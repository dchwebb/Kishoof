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
adc.DelayPot_R	> Warp amount
*/

// Create sine look up table as constexpr so will be stored in flash (create one extra entry to simplify interpolation)
constexpr std::array<float, WaveTable::sinLUTSize + 1> sineLUT = wavetable.CreateSinLUT();


void WaveTable::CalcSample()
{
	debugMain.SetHigh();		// Debug

	// Previously calculated samples output at beginning of interrupt to keep timing independent of calculation time
	SPI2->TXDR = (int32_t)(outputSamples[0] * floatToIntMult);
	SPI2->TXDR = (int32_t)(outputSamples[1] * floatToIntMult);

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

	filter.cutoff = 1.0f / pitchInc;				// Set filter for recalculation

	readPos += pitchInc;
	if (readPos >= 2048) { readPos -= 2048; }

	float adjReadPos = CalcWarp();

	if (stepped) {
		OutputSample(1, adjReadPos);
	} else {
		AdditiveWave();
	}

	if (warpType == Warp::tzfm) {
		// Through Zero FM: Phase distorts channel A using scaled bipolar version of channel B's waveform
		float bendAmt = 1.0f / 48.0f;		// Increase to extend bend amount range
		if (adc.DelayPot_R > 32767) {
			adjReadPos = readPos + outputSamples[1] * (adc.DelayPot_R - 32767) * bendAmt;
		} else {
			adjReadPos = readPos - outputSamples[1] * (32767 - adc.DelayPot_R) * bendAmt;
		}
		if (adjReadPos >= 2048) { adjReadPos -= 2048; }
		if (adjReadPos < 0) { adjReadPos += 2048; }
	}

	OutputSample(0, adjReadPos);


	// Enter sample in draw table to enable LCD update
	constexpr float widthMult = (float)LCD::width / 2048.0f;		// Scale to width of the LCD
	const uint8_t drawPos = std::min((uint8_t)std::round(readPos * widthMult), (uint8_t)(LCD::width - 1));		// convert position from position in 2048 sample wavetable to 240 wide screen
	//drawData[drawPos] = (uint8_t)((1.0f - outputSampleA) * 60.0f);
	drawData[drawPos] = (uint8_t)(((1.0f - outputSamples[0]) * 60.0f * 0.5f) + (0.5f * drawData[drawPos]));		// Smoothed and inverted

	debugMain.SetLow();			// Debug off
}


inline void WaveTable::OutputSample(uint8_t chn, float readPos)
{
	// Get location of current wavetable frame in wavetable
	const float wtPos = ((float)(wavFile.tableCount - 1) / 43000.0f) * (65536 - channel[chn].adcControl);
	channel[chn].pos = std::clamp((float)(0.99f * channel[chn].pos + 0.01f * wtPos), 0.0f, (float)(wavFile.tableCount - 1));	// Smooth
	const uint32_t sampleOffset = 2048 * std::floor(channel[chn].pos);			// get sample position of wavetable frame

	// Interpolate between samples
	const float ratio = readPos - (uint32_t)readPos;
	outputSamples[chn] = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset, ratio);

	// Interpolate between wavetables if channel A
	if (!stepped && chn == 0) {
		const float wtRatio = channel[chn].pos - (uint32_t)channel[chn].pos;
		if (wtRatio > 0.0001f) {
			float outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset + 2048, ratio);
			outputSamples[0] = std::lerp(outputSamples[0], outputSample2, wtRatio);
		}
	}
}


inline float WaveTable::CalcWarp()
{
	// Set warp type from knob with hysteresis
	if (abs(warpVal - adc.DelayPot_L) > 100) {
		warpVal = adc.DelayPot_L;
		warpType = (Warp)((uint32_t)Warp::count * warpVal  / 65536);
	}

	float adjReadPos;					// Read position after warp applied
	switch (warpType) {
	case Warp::bend: {
		// Waveform is stretched to one side (or squeezed to the other side) [Like 'Asym' in Serum]
		// https://www.desmos.com/calculator/u9aphhyiqm
		const float a = std::clamp((float)adc.DelayPot_R / 32768.0f, 0.1f, 1.9f);
		if (readPos < 1024.0f * a) {
			adjReadPos = readPos / a;
		} else {
			adjReadPos = (readPos + 2048.0f - 2048.0f * a) / (2.0f - a);
		}
	}
	break;

	case Warp::squeeze: {
		// Pinched: waveform squashed from sides to center; Stretched: from center to sides [Like 'Bend' in Serum]
		// Deforms readPos using sine wave
		float bendAmt = 1.0f / 96.0f;		// Increase to extend bend amount range
		if (adc.DelayPot_R > 32767) {
			float sinWarp = sineLUT[((uint32_t)readPos + 1024) & 0x7ff];			// Apply a 180 degree phase shift and limit to 2047
			adjReadPos = readPos + sinWarp * (adc.DelayPot_R - 32767) * bendAmt;

		} else {
			float sinWarp = sineLUT[(uint32_t)readPos];			// Get amount of warp
			adjReadPos = readPos + sinWarp * (32767 - adc.DelayPot_R) * bendAmt;
		}

//		if (adjReadPos >= 2048) { adjReadPos -= 2048; }
//		if (adjReadPos < 0) { adjReadPos += 2048; }
	}
	break;

	case Warp::mirror: {
		// Like bend but flips direction in center: https://www.desmos.com/calculator/8jtheoca0l
		const float a = std::clamp((float)adc.DelayPot_R / 32768.0f, 0.1f, 1.9f);
		if (readPos < 512.0f * a) {
			adjReadPos = 2.0f * readPos / a;
		} else if (readPos < 1024.0f) {
			adjReadPos = (readPos * 2.0f) / (2.0f - a) + 2048.0f * (1.0f - a) / (2.0f - a);
		} else if (readPos < 2048.0f * (4.0f - a) / 4) {
			adjReadPos = (readPos * -2.0f) / (2.0f - a) + 2048.0f * (3.0f - a) / (2.0f - a);
		} else {
			adjReadPos = (4096.0f - readPos * 2.0f) / a;
		}
	}
	break;

	case Warp::reverse: {
		adjReadPos = 2048.0f - readPos;
	}
	break;
	default:
		adjReadPos = readPos;
		break;
	}
	return adjReadPos;
}


inline void WaveTable::AdditiveWave()
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
	outputSamples[1] = sample;
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

		LoadWaveTable((uint32_t*)0x08100000);			// 4088.wav
		//LoadWaveTable((uint32_t*)0x08130000);			// Basic Shapes.wav
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

	if (warpType != oldWarpType) {
		oldWarpType = warpType;
		std::string_view s = warpNames[(uint8_t)warpType];
		//lcd.DrawString(60, 180, s, &lcd.Font_Small, LCD_WHITE, LCD_BLACK);

		const uint32_t textLeft = 75;
		const uint32_t textTop = 190;
		lcd.DrawStringMem(0, 0, lcd.Font_Large.Width * s.length(), lcd.drawBuffer[activeDrawBuffer], s, &lcd.Font_Large, LCD_WHITE, LCD_BLACK);
		lcd.PatternFill(textLeft, textTop, textLeft - 1 + lcd.Font_Large.Width * s.length(), textTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else {
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
	}
	activeDrawBuffer = !activeDrawBuffer;

	// Trigger MDMA frame buffer blanking
	MDMATransfer(lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 2);
	bufferClear = false;


	//debugDraw.SetLow();			// Debug off

}
