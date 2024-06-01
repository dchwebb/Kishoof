#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include "FatTools.h"
#include <cmath>
#include <cstring>

WaveTable wavetable;

// C:\Users\Dominic\Documents\Xfer\Serum Presets\Tables

volatile int susp = 0;
volatile float dbg[5000];
int idx = 0;
void WaveTable::CalcSample()
{
	GpioPin::SetHigh(GPIOD, 9);		// Debug
	StartDebugTimer();

	// 0v = 61200; 1v = 50110; 2v = 39020; 3v = 27910; 4v = 16790; 5v = 5670
	// C: 16.35 Hz 32.70 Hz; 65.41 Hz; 130.81 Hz; 261.63 Hz; 523.25 Hz; 1046.50 Hz; 2093.00 Hz; 4186.01 Hz
	// 61200 > 65.41 Hz; 50110 > 130.81 Hz; 39020 > 261.63 Hz

	// Increment = Hz * (2048 / 48000) = Hz * (wavetablesize / samplerate)
	// Pitch calculations - Increase pitchBase to increase pitch; Reduce ABS(cvMult) to increase spread

	// Calculations: 11090 is difference in cv between two octaves; 50110 is cv at 1v and 130.81 is desired Hz at 1v
	constexpr float pitchBase = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, -50120.0f / 11090.0f);
	constexpr float cvMult = -1.0f / 11090.0f;
	float newInc = pitchBase * std::pow(2.0f, (float)adc.ADC_Tempo * cvMult);			// for cycle length matching sample rate (48k)
	pitchInc = 0.99 * pitchInc + 0.01 * newInc;

	// Set filter for recalculation
	filter.cutoff = 1.0f / pitchInc;

	readPos += pitchInc;
	if (readPos >= 2048) { readPos -= 2048; }

	// Get wavetable position
	float wtPos = ((float)(wavFile.tableCount - 1) / 43000.0f) * (65536 - adc.ADC_HiHatDecay);
	wavetableIdx = std::clamp((float)(0.99f * wavetableIdx + 0.01f * wtPos), 0.0f, (float)(wavFile.tableCount - 1));
	uint32_t sampleOffset = 2048 * std::floor(wavetableIdx);

	float outputSample;
	float ratio = readPos - (uint32_t)readPos;
	if (ratio > 0.00001f) {
		outputSample = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset, ratio);
	} else {
		outputSample = filter.CalcFilter((uint32_t)readPos, activeWaveTable + sampleOffset);
	}

	// Interpolate between wavetables
	ratio = wavetableIdx - (uint32_t)wavetableIdx;
	if (ratio > 0.0001f) {
		float outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, activeWaveTable + sampleOffset + 2048, ratio);
		outputSample = std::lerp(outputSample, outputSample2, ratio);
	}

	SPI2->TXDR = (int32_t)(outputSample * floatToIntMult);
	SPI2->TXDR = (int32_t)(outputSample * floatToIntMult);;

	debugTiming = StopDebugTimer();

	GpioPin::SetLow(GPIOD, 9);			// Debug off
}


void WaveTable::Init()
{
	if (wavetableType == TestData::wavetable) {

		//memcpy((uint8_t*)0x24000000, (uint8_t*)0x08100000, 131208);		// Copy wavetable to ram
		StartDebugTimer();
		//memcpy((uint8_t*)0x24000000, (uint8_t*)0x90843000, 467080);		// Copy wavetable to ram
		memcpy((uint8_t*)0x24000000, (uint8_t*)0x90843000, 467080);		// Copy wavetable to ram
		memLoad = StopDebugTimer();
		//LoadWaveTable((uint32_t*)0x08100000);
		LoadWaveTable((uint32_t*)0x90843000);
		//LoadWaveTable((uint32_t*)0x24000000);
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

bool WaveTable::UpdateSampleList()
{
	// Updates list of samples from FAT root directory
	FATFileInfo* dirEntry = fatTools.rootDirectory;

	uint32_t pos = 0;
	bool changed = false;

	while (dirEntry->name[0] != 0) {

		if (dirEntry->name[0] != FATFileInfo::fileDeleted && dirEntry->attr == FATFileInfo::LONG_NAME) {
			// Store long file name in temporary buffer as this may contain volume and panning information
			const FATLongFilename* lfn = (FATLongFilename*)dirEntry;
			const char tempFileName[13] {lfn->name1[0], lfn->name1[2], lfn->name1[4], lfn->name1[6], lfn->name1[8],
				lfn->name2[0], lfn->name2[2], lfn->name2[4], lfn->name2[6], lfn->name2[8], lfn->name2[10],
				lfn->name3[0], lfn->name3[2]};

			const uint32_t pos = ((lfn->order & 0x3F) - 1) * 13;
			if (pos + 13 < sizeof(longFileName)) {
				memcpy(&longFileName[pos], tempFileName, 13);		// strip 0x40 marker from first LFN entry order field
			}
			lfnPosition += 13;

		// Valid sample: not LFN, not deleted, not directory, extension = WAV
		} else if (dirEntry->name[0] != FATFileInfo::fileDeleted && (dirEntry->attr & AM_DIR) == 0 && strncmp(&(dirEntry->name[8]), "WAV", 3) == 0) {
			Sample* sample = &(sampleList[pos++]);

			// If directory entry preceeded by long file name use that to check for volume/panning information
			float newVolume = 1.0f;
			if (lfnPosition > 0) {
				longFileName[lfnPosition] = '\0';
				const int32_t vol = ParseInt(longFileName, ".v", 0, 200);
				if (vol > 0) {
					newVolume = static_cast<float>(vol) / 100.0f;
				}
				lfnPosition = 0;
			}

			// Check if any fields have changed
			if (sample->cluster != dirEntry->firstClusterLow || sample->size != dirEntry->fileSize ||
					strncmp(sample->name, dirEntry->name, 11) != 0) {
				changed = true;
				strncpy(sample->name, dirEntry->name, 11);
				sample->cluster = dirEntry->firstClusterLow;
				sample->size = dirEntry->fileSize;

				//sample->valid = GetSampleInfo(sample);
			}
		} else {
			lfnPosition = 0;
		}
		dirEntry++;
	}

	// Blank next sample (if exists) to show end of list
	Sample* sample = &(sampleList[pos++]);
	sample->name[0] = 0;


	return changed;
}


int32_t WaveTable::ParseInt(const std::string_view cmd, const std::string_view precedingChar, const int32_t low, const int32_t high)
{
	int32_t val = -1;
	const int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(&cmd[pos + precedingChar.size()], "0123456789-") > 0) {
		val = std::stoi(&cmd[pos + precedingChar.size()]);
	}
	if (high > low && (val > high || val < low)) {
		return low - 1;
	}
	return val;
}

