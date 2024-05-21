#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include "LCD.h"

#include <cmath>
#include <cstring>

WaveTable wavetable;

// C:\Users\Dominic\Documents\Xfer\Serum Presets\Tables

// Create sine look up table as constexpr so will be stored in flash (create one extra entry to simplify interpolation)
constexpr std::array<float, WaveTable::sinLUTSize + 1> sineLUT = wavetable.CreateSinLUT();

uint32_t flashBusy = 0;

void WaveTable::CalcSample()
{
	debugMain.SetHigh();		// Debug

	// Previously calculated samples output at beginning of interrupt to keep timing independent of calculation time
	SPI2->TXDR = (int32_t)(outputSamples[0] * scaleOutput);
	SPI2->TXDR = (int32_t)(outputSamples[1] * scaleOutput);

	if (fatTools.Busy() && activeWaveTable > 0) {			// If using built-in wavetable don't need flash memory
		++flashBusy;
		return;
	}

	stepped = modeSwitch.IsLow();
	float octave = octaveUp.IsHigh() ? 2.0f : octaveDown.IsHigh() ? 0.5 : 1.0f;

	// 0v = 61200; 1v = 50110; 2v = 39020; 3v = 27910; 4v = 16790; 5v = 5670
	// C: 16.35 Hz 32.70 Hz; 65.41 Hz; 130.81 Hz; 261.63 Hz; 523.25 Hz; 1046.50 Hz; 2093.00 Hz; 4186.01 Hz
	// 61200 > 65.41 Hz; 50110 > 130.81 Hz; 39020 > 261.63 Hz

	// Increment = Hz * (2048 / 48000) = Hz * (wavetablesize / samplerate)
	// Pitch calculations - Increase pitchBase to increase pitch; Reduce ABS(cvMult) to increase spread

	// Calculations: 11090 is difference in cv between two octaves; 50110 is cv at 1v and 130.81 is desired Hz at 1v
	//constexpr float pitchBase = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, -50120.0f / 11090.0f);
	constexpr float pitchBase = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, -50050.0f / 11330.0f);
	constexpr float cvMult = -1.0f / 11330.0f;
	float newInc = pitchBase * std::pow(2.0f, (float)adc.Pitch_CV * cvMult) * octave;			// for cycle length matching sample rate (48k)
	pitchInc[0] = 0.99 * pitchInc[0] + 0.01 * newInc;

	filter.cutoff = 1.0f / pitchInc[0];				// Set filter for recalculation

	readPos[0] += pitchInc[0];
	if (readPos[0] >= 2048) { readPos[0] -= 2048; }

	pitchInc[1] = pitchInc[0] * (octaveChnB ? 0.5f : 1.0f);
	readPos[1] += pitchInc[1];
	if (readPos[1] >= 2048) { readPos[1] -= 2048; }


	float adjReadPos = CalcWarp();

	if (stepped) {
		OutputSample(1, readPos[1]);
	} else {
		AdditiveWave();
	}


	OutputSample(0, adjReadPos);

	if (chBMix.IsHigh()) {
		outputSamples[1] = FastTanh(outputSamples[0] + outputSamples[1]);
	} else 	if (chBRingMod.IsHigh()) {
		outputSamples[1] = outputSamples[0] * outputSamples[1];
	}


	// Enter sample in draw table to enable LCD update
	constexpr float widthMult = (float)LCD::width / 2048.0f;		// Scale to width of the LCD
	const uint8_t drawPos = std::min((uint8_t)std::round(readPos[0] * widthMult), (uint8_t)(LCD::width - 1));		// convert position from position in 2048 sample wavetable to 240 wide screen
	//drawData[drawPos] = (uint8_t)((1.0f - outputSampleA) * 60.0f);
	drawData[drawPos] = (uint8_t)(((1.0f - outputSamples[0]) * 60.0f * 0.5f) + (0.5f * drawData[drawPos]));		// Smoothed and inverted

	debugMain.SetLow();			// Debug off
}


inline void WaveTable::OutputSample(uint8_t chn, float readPos)
{
	// Get location of current wavetable frame in wavetable
	//const float wtPos = ((float)(wavList[activeWaveTable].tableCount - 1) / 43000.0f) * channel[chn].adcPot;
	//channel[chn].pos = std::clamp((float)(0.99f * channel[chn].pos + 0.01f * wtPos), 0.0f, (float)(wavList[activeWaveTable].tableCount - 1));	// Smooth
	const float pos = std::clamp(channel[chn].Val() * (wavList[activeWaveTable].tableCount - 1), 0.0f, (float)(wavList[activeWaveTable].tableCount - 1));
	const uint32_t sampleOffset = 2048 * std::floor(pos);			// get sample position of wavetable frame

	// Interpolate between samples
	const float ratio = readPos - (uint32_t)readPos;
	outputSamples[chn] = filter.CalcInterpolatedFilter((uint32_t)readPos, (float*)wavList[activeWaveTable].startAddr + sampleOffset, ratio, pitchInc[chn]);

	// Interpolate between wavetables if channel A
	if (!stepped && chn == 0) {
		const float wtRatio = pos - (uint32_t)pos;
		if (wtRatio > 0.0001f) {
			float outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, (float*)wavList[activeWaveTable].startAddr + sampleOffset + 2048, ratio, pitchInc[chn]);
			outputSamples[0] = std::lerp(outputSamples[0], outputSample2, wtRatio);
		}
	}
}


inline float WaveTable::CalcWarp()
{
	// Set warp type from knob with hysteresis
	if (abs(warpVal - adc.Warp_Type_Pot) > 100) {
		warpVal = adc.Warp_Type_Pot;
		warpType = (Warp)((uint32_t)Warp::count * warpVal  / 65536);
	}

	float adjReadPos;					// Read position after warp applied
	float pitchMult = 1.0f;				// Multiply to adjust the pitch increment for calculating ant-aliasing filter cutoff
	switch (warpType) {
	case Warp::bend: {
		// Waveform is stretched to one side (or squeezed to the other side) [Like 'Asym' in Serum]
		// https://www.desmos.com/calculator/u9aphhyiqm
		const float a = std::clamp((float)adc.Warp_Amt_Pot / 32768.0f, 0.1f, 1.9f);
		if (readPos[0] < 1024.0f * a) {
			pitchMult = 1.0f / a;
			adjReadPos = readPos[0] / a;
		} else {
			pitchMult = 1.0f / (2.0f - a);
			adjReadPos = (readPos[0] + 2048.0f - 2048.0f * a) / (2.0f - a);
		}
	}
	break;

	case Warp::squeeze: {
		// Pinched: waveform squashed from sides to center; Stretched: from center to sides [Like 'Bend' in Serum]
		// Deforms readPos using sine wave
		float bendAmt = 1.0f / 96.0f;		// Increase to extend bend amount range
		if (adc.Warp_Amt_Pot > 32767) {
			float sinWarp = sineLUT[((uint32_t)readPos[0] + 1024) & 0x7ff];			// Apply a 180 degree phase shift and limit to 2047
			adjReadPos = readPos[0] + sinWarp * (adc.Warp_Amt_Pot - 32767) * bendAmt;

		} else {
			float sinWarp = sineLUT[(uint32_t)readPos[0]];			// Get amount of warp
			adjReadPos = readPos[0] + sinWarp * (32767 - adc.Warp_Amt_Pot) * bendAmt;
		}

	}
	break;

	case Warp::mirror: {
		// Like bend but flips direction in center: https://www.desmos.com/calculator/8jtheoca0l
		const float a = std::clamp((float)adc.Warp_Amt_Pot / 32768.0f, 0.1f, 1.9f);
		if (readPos[0] < 512.0f * a) {
			adjReadPos = 2.0f * readPos[0] / a;
		} else if (readPos[0] < 1024.0f) {
			adjReadPos = (readPos[0] * 2.0f) / (2.0f - a) + 2048.0f * (1.0f - a) / (2.0f - a);
		} else if (readPos[0] < 2048.0f * (4.0f - a) / 4) {
			adjReadPos = (readPos[0] * -2.0f) / (2.0f - a) + 2048.0f * (3.0f - a) / (2.0f - a);
		} else {
			adjReadPos = (4096.0f - readPos[0] * 2.0f) / a;
		}
	}
	break;

	case Warp::reverse: {
		adjReadPos = 2048.0f - readPos[0];
	}
	break;


	case Warp::tzfm: {
		// Through Zero FM: Phase distorts channel A using scaled bipolar version of channel B's waveform
		float bendAmt = 1.0f / 48.0f;		// Increase to extend bend amount range
		adjReadPos = readPos[0] + outputSamples[1] * (adc.Warp_Amt_Pot - 32767) * bendAmt;
		if (adjReadPos >= 2048) { adjReadPos -= 2048; }
		if (adjReadPos < 0) { adjReadPos += 2048; }
	}
	break;

	default:
		adjReadPos = readPos[0];
		break;
	}
	return adjReadPos;
}


inline void WaveTable::AdditiveWave()
{
	// Calculate which pair of harmonic sets to interpolate between
	float harmonicPos = (float)((harmonicSets - 1) * adc.Wavetable_Pos_B_Pot) / 65536.0f;
	uint32_t harmonicLow = (uint32_t)harmonicPos;
	float ratio = harmonicPos - harmonicLow;

	float sample = 0.0f;
	float pos = 0.0f;
	for (uint32_t i = 0; i < harmonicCount; ++i) {
		pos += readPos[1];
		while (pos >= 2048.0f) {
			pos -= 2048.0f;
		}
		//outputSampleB += additiveHarmonics[i] * std::lerp(sineLUT[(uint32_t)pos], sineLUT[std::ceil(pos)], pos - std::floor(pos));
		float harmonicLevel = std::lerp(additiveHarmonics[harmonicLow][i], additiveHarmonics[harmonicLow + 1][i], ratio);
		sample += harmonicLevel * sineLUT[(uint32_t)pos];
	}
	outputSamples[1] = sample;
}


void WaveTable::ChangeWaveTable(int32_t upDown)
{
	int32_t nextWavetable = (int32_t)activeWaveTable + upDown;
	if (nextWavetable >= 0 && nextWavetable < (int32_t)wavetableCount) {
		activeWaveTable = nextWavetable;
		strncpy(waveTableName, wavList[activeWaveTable].name, 11);
	}
}


void WaveTable::Init()
{
	wavetable.UpdateWavetableList();						// Updated list of samples on flash

	// Locate valid wavetable
	uint32_t pos = 0;
	for (auto& wav : wavList) {
		if (wav.valid) {
			activeWaveTable = pos;
			strncpy(waveTableName, wav.name, 11);
			return;
		}
		++pos;
	}
}



bool WaveTable::GetWavInfo(Wav* wav)
{
	// populate the sample object with sample rate, number of channels etc
	// Parsing the .wav format is a pain because the header is split into a variable number of chunks and sections are not word aligned

	const uint8_t* wavHeader = fatTools.GetClusterAddr(wav->cluster, true);

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

	wav->dataFormat = *(uint16_t*)&(wavHeader[pos + 8]);
	wav->sampleRate = *(uint32_t*)&(wavHeader[pos + 12]);
	wav->channels   = *(uint16_t*)&(wavHeader[pos + 10]);
	wav->byteDepth  = *(uint16_t*)&(wavHeader[pos + 22]) / 8;

	// Check if there is a clm chunk with Serum metadata
	pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));
	if (*(uint32_t*)&(wavHeader[pos]) == 0x206d6c63) {		// Look for string 'clm '
		const char* metadata = (char*)&(wavHeader[pos + 16]);
		if (std::strspn(metadata, "0123456789") == 8) {
			wav->metadata = std::stoi(metadata);
		}


	}

	// Navigate forward to find the start of the data area
	while (*(uint32_t*)&(wavHeader[pos]) != 0x61746164) {		// Look for string 'data'
		pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));
		if (pos > 1200) {
			return false;
		}
	}

	wav->dataSize = *(uint32_t*)&(wavHeader[pos + 4]);		// Num Samples * Num Channels * Bits per Sample / 8
	wav->sampleCount = wav->dataSize / (wav->channels * wav->byteDepth);
	wav->startAddr = &(wavHeader[pos + 8]);
	wav->tableCount = wav->sampleCount / 2048;

	// Follow cluster chain and store last cluster if not contiguous to tell playback engine when to do a fresh address lookup
	uint32_t cluster = wav->cluster;
	wav->lastCluster = 0xFFFFFFFF;

	while (fatTools.clusterChain[cluster] != 0xFFFF) {
		if (fatTools.clusterChain[cluster] != cluster + 1 && wav->lastCluster == 0xFFFFFFFF) {		// Store cluster at first discontinuity of chain
			wav->lastCluster = cluster;
		}
		cluster = fatTools.clusterChain[cluster];
	}
	wav->endAddr = fatTools.GetClusterAddr(cluster);
	return true;
}


bool WaveTable::UpdateWavetableList()
{
	// Create a test wavetable with a couple of waveforms - also used in case of no file system
	strncpy(wavList[0].name, "default    ", 11);
	wavList[0].dataFormat = 3;
	wavList[0].channels   = 1;
	wavList[0].byteDepth  = 4;
	wavList[0].sampleCount = 4096;
	wavList[0].tableCount = 2;
	wavList[0].startAddr = (uint8_t*)&testWavetable;

	// Generate test waves
	for (uint32_t i = 0; i < 2048; ++i) {
		testWavetable[i] = std::sin((float)i * M_PI * 2.0f / 2048.0f);
		testWavetable[i + 2048] = (2.0f * i / 2048.0f) - 1.0f;
	}

	// Updates list of wavetables from FAT root directory
	FATFileInfo* dirEntry = fatTools.rootDirectory;

	uint32_t pos = 1;
	bool changed = false;

	while (dirEntry->name[0] != 0 && pos < maxWavetable) {

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

		// Valid wavetable: not LFN, not deleted, not directory, extension = WAV
		} else if (dirEntry->name[0] != FATFileInfo::fileDeleted && (dirEntry->attr & AM_DIR) == 0 && strncmp(&(dirEntry->name[8]), "WAV", 3) == 0) {
			Wav* wav = &(wavList[pos++]);

			// Check if any fields have changed
			if (wav->cluster != dirEntry->firstClusterLow || wav->size != dirEntry->fileSize ||
					strncmp(wav->name, dirEntry->name, 11) != 0) {
				changed = true;
				strncpy(wav->name, dirEntry->name, 8);
				wav->cluster = dirEntry->firstClusterLow;
				wav->size = dirEntry->fileSize;

				wav->valid = GetWavInfo(wav);
			}
		} else {
			lfnPosition = 0;
		}
		dirEntry++;
	}
	wavetableCount = pos;

	// Blank next sample (if exists) to show end of list
	Wav* wav = &(wavList[pos++]);
	wav->name[0] = 0;

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



// Algorithm source: https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
float WaveTable::FastTanh(float x)
{
	float x2 = x * x;
	float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
	float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
	return a / b;
}
