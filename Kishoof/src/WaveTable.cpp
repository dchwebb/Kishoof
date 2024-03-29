#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include "LCD.h"

#include <cmath>
#include <cstring>

WaveTable wavetable;

// C:\Users\Dominic\Documents\Xfer\Serum Presets\Tables

/* Currently used adcs:
adc.FeedbackCV 	> 1V/Oct CV
adc.DelayCV_L 	> Channel A wavetable position
adc.FilterPot 	> Channel B wavetable position
*/

// Create sine look up table as constexpr so will be stored in flash (create one extra entry to simplify interpolation)
constexpr std::array<float, WaveTable::sinLUTSize + 1> sineLUT = wavetable.CreateSinLUT();


void WaveTable::CalcSample()
{
	debugMain.SetHigh();		// Debug

	SPI2->TXDR = outputSamples[0];
	SPI2->TXDR = outputSamples[1];

	// 0v = 61200; 1v = 50110; 2v = 39020; 3v = 27910; 4v = 16790; 5v = 5670
	// C: 16.35 Hz 32.70 Hz; 65.41 Hz; 130.81 Hz; 261.63 Hz; 523.25 Hz; 1046.50 Hz; 2093.00 Hz; 4186.01 Hz
	// 61200 > 65.41 Hz; 50110 > 130.81 Hz; 39020 > 261.63 Hz

	// Increment = Hz * (2048 / 48000) = Hz * (wavetablesize / samplerate)
	// Pitch calculations - Increase pitchBase to increase pitch; Reduce ABS(cvMult) to increase spread

	// Calculations: 11090 is difference in cv between two octaves; 50110 is cv at 1v and 130.81 is desired Hz at 1v
	constexpr float pitchBase = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, -50120.0f / 11090.0f);
	constexpr float cvMult = -1.0f / 11090.0f;
	float newInc = pitchBase * std::pow(2.0f, (float)adc.FeedbackCV * cvMult);			// for cycle length matching sample rate (48k)
	pitchInc = 0.99 * pitchInc + 0.01 * newInc;

	// Set filter for recalculation
	filter.cutoff = 1.0f / pitchInc;

	readPos += pitchInc;
	if (readPos >= 2048) { readPos -= 2048; }

	// Get wavetable position
	float wtPos = ((float)(wavFile.tableCount - 1) / 43000.0f) * (65536 - adc.DelayCV_L);
	wavetableIdxA = std::clamp((float)(0.99f * wavetableIdxA + 0.01f * wtPos), 0.0f, (float)(wavFile.tableCount - 1));
	uint32_t sampleOffset = 2048 * std::floor(wavetableIdxA);

	float outputSampleA, outputSampleB;
	float ratio = readPos - (uint32_t)readPos;
	if (ratio > 0.00001f) {
		outputSampleA = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset, ratio);
	} else {
		outputSampleA = filter.CalcFilter((uint32_t)readPos, activeWaveTable + sampleOffset);
	}

	if (stepped) {
		// Get wavetable position
		wtPos = ((float)(wavFile.tableCount - 1) / 43000.0f) * (65536 - adc.FilterPot);
		wavetableIdxB = std::clamp((float)(0.99f * wavetableIdxB + 0.01f * wtPos), 0.0f, (float)(wavFile.tableCount - 1));
		sampleOffset = 2048 * std::floor(wavetableIdxB);
		outputSampleB = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset, ratio);

	} else {
		// Interpolate between wavetables
		ratio = wavetableIdxA - (uint32_t)wavetableIdxA;
		if (ratio > 0.0001f) {
			float outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset + 2048, ratio);
			outputSampleA = std::lerp(outputSampleA, outputSample2, ratio);
		}

		outputSampleB = AdditiveWave();						// Calculate channel B as additive wave
	}

//	SPI2->TXDR = (int32_t)(outputSampleA * floatToIntMult);
//	SPI2->TXDR = (int32_t)(outputSampleB * floatToIntMult);

	outputSamples[0] = (int32_t)(outputSampleA * floatToIntMult);
	outputSamples[1] = (int32_t)(outputSampleB * floatToIntMult);

	// Enter sample in draw table to enable LCD update
	constexpr float widthMult = (float)LCD::width / 2048.0f;		// Scale to width of the LCD
	const uint8_t drawPos = readPos * widthMult;		// convert position from position in 2048 sample wavetable to 240 wide screen

	// NB smoothed version is slightly slower than than non-smoothed (~10ns)
	drawData[drawPos] = (uint8_t)(((1.0f - outputSampleA) * 60.0f * 0.1f) + (0.9f * drawData[drawPos]));
	//drawData[drawPos] = (uint8_t)((1.0f + outputSample) * 60.0f);		// Change + to - to invert waveform

	debugMain.SetLow();			// Debug off
}


float WaveTable::AdditiveWave()
{
	// Calculate which pair of harmonic sets to interpolate between
	float harmonicPos = (float)((harmonicSets - 1) * adc.FilterPot) / 65536.0f;
	uint32_t harmonicLow = (uint32_t)harmonicPos;
	float ratio = harmonicPos - harmonicLow;

	float sample = 0.0f;
	float pos = 0.0f;
	for (uint32_t i = 0; i < harmonicCount; ++i) {
		pos += readPos;
		while (pos >= 2048.0f) {
			pos -= 2048.0f;
		}
		//outputSampleB += additiveHarmonics[i] * std::lerp(sineLUT[(uint32_t)pos], sineLUT[std::ceil(pos)], pos - std::floor(pos));
		float harmonicLevel = std::lerp(additiveHarmonics[harmonicLow][i], additiveHarmonics[harmonicLow + 1][i], ratio);
		sample += harmonicLevel * sineLUT[(uint32_t)pos];
	}
	return sample;
}


int32_t WaveTable::OutputMix(float wetSample)
{
	// Output mix level
	float dryLevel = static_cast<float>(adc.Mix) / 65536.0f;		// Convert 16 bit int to float 0 -> 1

	DAC1->DHR12R2 = (1.0f - dryLevel) * 4095.0f;		// Wet level
	DAC1->DHR12R1 = dryLevel * 4095.0f;					// Dry level

	int16_t outputSample = std::clamp(static_cast<int32_t>(wetSample), -32767L, 32767L);

	return outputSample;
}



void WaveTable::Init()
{
	if (wavetableType == TestData::wavetable) {

		//memcpy((uint8_t*)0x24000000, (uint8_t*)0x08100000, 131208);		// Copy wavetable to ram

		LoadWaveTable((uint32_t*)0x08100000);
		activeWaveTable = (float*)wavFile.startAddr;
	} else {
		// Generate test waves
		for (uint32_t i = 0; i < 2048; ++i) {
			switch (wavetableType) {
			case TestData::noise:
				while ((RNG->SR & RNG_SR_DRDY) == 0) {};
				testWavetable[i] = intToFloatMult * static_cast<int32_t>(RNG->DR);
				activeWaveTable = testWavetable;
				break;
			case TestData::twintone:
				testWavetable[i] = 0.5f * (std::sin((float)i * M_PI * 2.0f / 2048.0f) +
						std::sin(200.0f * (float)i * M_PI * 2.0f / 2048.0f));
				activeWaveTable = testWavetable;
				break;
			default:
				break;
			}
		}

		// Populate wavFile info
		wavFile.dataFormat = 3;
		wavFile.channels   = 1;
		wavFile.byteDepth  = 4;
		wavFile.sampleCount = 2048;
		wavFile.tableCount = 1;
		wavFile.startAddr = (uint8_t*)&testWavetable;
	}

	DAC1->DHR12R2 = 4095;				// Wet level
	DAC1->DHR12R1 = 0;					// Dry level
}


// Algorithm source: https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
float WaveTable::FastTanh(float x)
{
	float x2 = x * x;
	float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
	float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
	return a / b;
}


bool WaveTable::LoadWaveTable(uint32_t* startAddr)
{
	// populate the sample object with sample rate, number of channels etc
	// Parsing the .wav format is a pain because the header is split into a variable number of chunks and sections are not word aligned

	const uint8_t* wavHeader = (uint8_t*)startAddr;

	// Check validity
	if (*(uint32_t*)wavHeader != 0x46464952) {					// wav file should start with letters 'RIFF'
		return false;
	}

	// Jump through chunks looking for 'fmt' chunk
	uint32_t pos = 12;											// First chunk ID at 12 byte (4 word) offset
	while (*(uint32_t*)&(wavHeader[pos]) != 0x20746D66) {		// Look for string 'fmt '
		pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));			// Each chunk title is followed by the size of that chunk which can be used to locate the next one
		if  (pos > 1000) {
			return false;
		}
	}

	wavFile.dataFormat = *(uint16_t*)&(wavHeader[pos + 8]);
	wavFile.sampleRate = *(uint32_t*)&(wavHeader[pos + 12]);
	wavFile.channels   = *(uint16_t*)&(wavHeader[pos + 10]);
	wavFile.byteDepth  = *(uint16_t*)&(wavHeader[pos + 22]) / 8;

	// Navigate forward to find the start of the data area
	while (*(uint32_t*)&(wavHeader[pos]) != 0x61746164) {		// Look for string 'data'
		pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));
		if (pos > 1200) {
			return false;
		}
	}

	wavFile.dataSize = *(uint32_t*)&(wavHeader[pos + 4]);		// Num Samples * Num Channels * Bits per Sample / 8
	wavFile.sampleCount = wavFile.dataSize / (wavFile.channels * wavFile.byteDepth);
	wavFile.tableCount = wavFile.sampleCount / 2048;
	wavFile.startAddr = (uint8_t*)&(wavHeader[pos + 8]);
	wavFile.fileSize = pos + 8 + wavFile.dataSize;		// header size + data size
	return true;
}

/*
void WaveTable::Draw()
{
	// Populate a frame buffer to display the wavetable values (half screen refresh)

	if (!bufferClear) {		// Wait until the framebuffer has been cleared by the DMA blanking process
		return;
	}

	debugDraw.SetHigh();			// Debug

	const uint8_t startPos = activeDrawBuffer ? 0 : 120;
	uint8_t oldHeight = drawData[activeDrawBuffer ? 0 : 119];
	for (uint8_t i = 0; i < 120; ++i) {
		// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
		uint8_t currHeight = drawData[i + startPos];
		do {
			const uint32_t pos = currHeight * 120 + i;
			lcd.drawBuffer[activeDrawBuffer][pos] = LCD_GREEN;
			currHeight += currHeight > oldHeight ? -1 : 1;
		} while (currHeight != oldHeight);

		oldHeight = drawData[i + startPos];
	}

	lcd.PatternFill(startPos, 60, startPos + 119, 179, lcd.drawBuffer[activeDrawBuffer]);
	activeDrawBuffer = !activeDrawBuffer;

	// Trigger MDMA frame buffer blanking (memset too slow)
	MDMATransfer(lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 4);
	bufferClear = false;


	debugDraw.SetLow();			// Debug off
}
*/


void WaveTable::Draw()
{
	// Populate a frame buffer to display the wavetable values (full screen refresh)
	//debugDraw.SetHigh();			// Debug

	uint8_t oldHeight = drawData[0];
	for (uint8_t i = 0; i < 240; ++i) {

		// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
		uint8_t currHeight = drawData[i];
		do {
			const uint32_t pos = currHeight * LCD::height + i;
			lcd.drawBuffer[activeDrawBuffer][pos] = LCD_GREEN;
			currHeight += currHeight > oldHeight ? -1 : 1;
		} while (currHeight != oldHeight);

		oldHeight = drawData[i];
	}

	lcd.PatternFill(0, 60, LCD::width - 1, 179, lcd.drawBuffer[activeDrawBuffer]);
	activeDrawBuffer = !activeDrawBuffer;

	// Trigger MDMA frame buffer blanking
	MDMATransfer(lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 2);
	bufferClear = false;

	//debugDraw.SetLow();			// Debug off

}
