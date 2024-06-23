#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include "Calib.h"

#include <cmath>
#include <cstring>

WaveTable wavetable;


// Create sine look up table as constexpr so will be stored in flash (create one extra entry to simplify interpolation)
constexpr std::array<float, WaveTable::sinLUTSize + 1> sineLUT = wavetable.CreateSinLUT();

uint32_t flashBusy = 0;

extern bool vcaConnected;			// Temporary hack as current hardware does not have VCA normalled correctly


void WaveTable::CalcSample()
{
	// If crossfading (when switching warp type) blend from old sample to new sample
	if (crossfade > 0.0f) {
		outputSamples[0] = crossfade * oldOutputSamples[0] + (1.0f - crossfade) * outputSamples[0];
		outputSamples[1] = crossfade * oldOutputSamples[1] + (1.0f - crossfade) * outputSamples[1];
		crossfade -= 0.001f;
	}


	// Previously calculated samples output at beginning of interrupt to keep timing independent of calculation time
	if (vcaConnected) {
		const float vcaMult = std::max(60000.0f - adc.VcaCV, 0.0f);
		SPI2->TXDR = (int32_t)(outputSamples[0] * scaleVCAOutput * vcaMult);
		SPI2->TXDR = (int32_t)(outputSamples[1] * scaleVCAOutput * vcaMult);
	} else {
		SPI2->TXDR = (int32_t)(outputSamples[0] * scaleOutput);
		SPI2->TXDR = (int32_t)(outputSamples[1] * scaleOutput);
	}

	if (fatTools.Busy()) {
		++flashBusy;
		debugPin1.SetLow();		// Debug
		debugPin2.SetHigh();	// Debug
		return;
	}
	debugPin1.SetHigh();		// Debug
	debugPin2.SetLow();			// Debug


	// Pitch calculations
	const float octave = octaveUp.IsHigh() ? 2.0f : octaveDown.IsHigh() ? 0.5 : 1.0f;
	const float newInc = calib.cfg.pitchBase * std::pow(2.0f, (float)adc.Pitch_CV * calib.cfg.pitchMult) * octave;			// for cycle length matching sample rate (48k)
	smoothedInc = 0.99 * smoothedInc + 0.01 * newInc;

	// Increment the read position for each channel; pitch inc will be used in filter to set anti-aliasing cutoff frequency
	pitchInc[0] = smoothedInc;
	readPos[0] += smoothedInc;
	if (readPos[0] >= 2048.0f) { readPos[0] -= 2048.0f; }

	pitchInc[1] = smoothedInc * (cfg.octaveChnB ? 0.5f : 1.0f) * (cfg.warpButton ? -1.0f : 1.0f);
	readPos[1] += pitchInc[1];
	if (readPos[1] >= 2048.0f) { readPos[1] -= 2048.0f; }
	if (readPos[1] < 0.0f) { readPos[1] += 2048.0f; }


	// Generate channel B output first as used in TZFM to alter channel A read position
	stepped = modeSwitch.IsLow();
	if (stepped) {
		OutputSample(1, readPos[1]);
	} else {
		AdditiveWave();
	}
	const float adjReadPos = CalcWarp();
	OutputSample(0, adjReadPos);


	// Apply mix/ring mod to channel B
	if (chBMix.IsHigh()) {
		outputSamples[1] = FastTanh(outputSamples[0] + outputSamples[1]);
	} else 	if (chBRingMod.IsHigh()) {
		outputSamples[1] = FastTanh(outputSamples[0] * outputSamples[1]);
	}

	if (crossfade <= 0.0f) {
		oldOutputSamples[0] = FastTanh(outputSamples[0]);
		oldOutputSamples[1] = FastTanh(outputSamples[1]);
	}


	// Enter sample in draw table to enable LCD update: If channel A is affected by channel B (TZFM with octave down) Use channel B's position to draw waveform
	const uint32_t drawPosChn = (warpType == Warp::tzfm && cfg.octaveChnB) ? 1 : 0;

	const uint8_t drawPos0 = (uint8_t)std::round(readPos[drawPosChn] * drawWidthMult);		// convert from position in 2048 sample wavetable to draw width
	drawData[0][drawPos0] = (uint8_t)((1.0f - outputSamples[0]) * drawHeightMult);

	uint8_t drawPos1 = (uint8_t)std::round(readPos[1] * drawWidthMult);
	if (cfg.warpButton) {
		drawPos1 = 199 - drawPos1;				// Invert channel B
	}
	drawData[1][drawPos1] = (uint8_t)((1.0f - outputSamples[1]) * drawHeightMult);

	debugPin1.SetLow();			// Debug off
}


float WaveTable::QuantisedWavetablePos(uint8_t chn)
{
	// For drawing wavetable position: return quantised x position of current wavetable
	if (!stepped) {
		return wavetablePos[chn].pos;
	} else if (chn == 1) {
		return std::round(wavetablePos[chn].pos * harmonicSets) / (float)harmonicSets;
	} else {
		return std::round(wavetablePos[chn].pos * wavList[activeWaveTable].tableCount) / (float)wavList[activeWaveTable].tableCount;
	}
}


inline void WaveTable::OutputSample(uint8_t chn, float readPos)
{
	// Get location of current wavetable frame in wavetable
	const Wav wav = wavList[activeWaveTable];
	float pos = std::clamp(wavetablePos[chn].Val() * (wavList[activeWaveTable].tableCount - 1), 0.0f, (float)(wavList[activeWaveTable].tableCount - 1));
	const uint32_t sampleOffset = 2048 * (stepped ? std::round(pos) : std::floor(pos));			// get sample position of wavetable frame

	// Interpolate between samples
	const float ratio = readPos - (uint32_t)readPos;
	if (wav.sampleType == SampleType::Float32) {
		outputSamples[chn] = filter.CalcInterpolatedFilter((uint32_t)readPos, (float*)wav.startAddr + sampleOffset, ratio, pitchInc[chn]);
	} else if (wav.sampleType == SampleType::PCM16) {
		outputSamples[chn] = filter.CalcInterpolatedFilter((uint32_t)readPos, (int16_t*)wav.startAddr + sampleOffset, ratio, pitchInc[chn]) * (1.0 / 32768.0f);
	}

	// Interpolate between wavetables if channel A
	if (!stepped && chn == 0) {
		const float wtRatio = pos - (uint32_t)pos;
		if (wtRatio > 0.0001f) {
			float outputSample2;
			if (wav.sampleType == SampleType::Float32) {
				outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, (float*)wav.startAddr + sampleOffset + 2048, ratio, pitchInc[chn]);
			} else {
				outputSample2 = filter.CalcInterpolatedFilter((uint32_t)readPos, (int16_t*)wav.startAddr + sampleOffset + 2048, ratio, pitchInc[chn]) * (1.0 / 32768.0f);
			}
			outputSamples[0] = std::lerp(outputSamples[0], outputSample2, wtRatio);
		}
	}
}


inline float WaveTable::CalcWarp()
{
	// Set warp type from knob with hysteresis
	if (abs(warpTypeVal - adc.Warp_Type_Pot) > 1000) {
		warpTypeVal = adc.Warp_Type_Pot;
		const Warp newWarpType = (Warp)((uint32_t)Warp::count * warpTypeVal / 65536);
		if (newWarpType != warpType) {
			warpType = newWarpType;
			crossfade = 1.0f;
		}
	}

	// Calculate smoothed warp amount from pot and CV with trimmer controlling range of CV
	const float cv = std::max(61300.0f - adc.WarpCV, 0.0f);		// Reduce to ensure can hit zero with noise
	warpAmt = (0.99f * warpAmt) +
			  (0.01f * std::clamp((adc.Warp_Amt_Pot + NormaliseADC(adc.Warp_Amt_Trm) * cv), 0.0f, 65535.0f));

	float adjReadPos;					// Read position after warp applied
	float pitchAdj;						// To adjust the pitch increment for calculating ant-aliasing filter cutoff

	switch (warpType) {
	case Warp::bend: {
		// Waveform is stretched to one side (or squeezed to the other side) [Like 'Asym' in Serum]
		// https://www.desmos.com/calculator/u9aphhyiqm
		const float a = std::clamp(warpAmt / 32768.0f, 0.1f, 1.9f);
		if (readPos[0] < 1024.0f * a) {
			pitchAdj = 1.0f / a;
			adjReadPos = readPos[0] * pitchAdj;
		} else {
			pitchAdj = 1.0f / (2.0f - a);
			adjReadPos = (readPos[0] + 2048.0f - 2048.0f * a) / (2.0f - a);
		}
		pitchInc[0] *= pitchAdj;
	}
	break;

	case Warp::squeeze: {
		// Pinched: waveform squashed from sides to center; Stretched: from center to sides [Like 'Bend' in Serum]
		// Deforms readPos using sine wave
		const float bendAmt = 1.0f / 96.0f;		// Increase to extend bend amount range
		if (warpAmt > 32767.0f) {
			const float sinWarp = sineLUT[((uint32_t)readPos[0] + 1024) & 0x7ff];			// Apply a 180 degree phase shift and limit to 2047
			pitchAdj = sinWarp * (warpAmt - 32767.0f) * bendAmt;
		} else {
			const float sinWarp = sineLUT[(uint32_t)readPos[0]];			// Get amount of warp
			pitchAdj = sinWarp * (32767.0f - warpAmt) * bendAmt;
		}
		adjReadPos = readPos[0] + pitchAdj;
		pitchInc[0] *= 1.5f;			// Adding pitchAdj creates odd effects around bend point - sounds better multiplying by average
	}
	break;

	case Warp::mirror: {
		// Like bend but flips direction in center: https://www.desmos.com/calculator/8jtheoca0l
		const float a = std::clamp(warpAmt / 32768.0f, 0.1f, 1.9f);
		if (readPos[0] < 512.0f * a) {
			pitchAdj = 2.0f / a;
			adjReadPos = pitchAdj * readPos[0];
		} else if (readPos[0] < 1024.0f) {
			pitchAdj = 2.0f / (2.0f - a);
			adjReadPos = (readPos[0] * pitchAdj) + 2048.0f * (1.0f - a) / (2.0f - a);
		} else if (readPos[0] < 2048.0f * (4.0f - a) / 4) {
			pitchAdj = 2.0f / (2.0f - a);
			adjReadPos = (readPos[0] * -pitchAdj) + 2048.0f * (3.0f - a) / (2.0f - a);
		} else {
			pitchAdj = 2.0f / a;
			adjReadPos = (4096.0f - readPos[0] * 2.0f) / a;
		}
		pitchInc[0] *= pitchAdj;
	}
	break;

	case Warp::tzfm: {
		// Through Zero FM: Phase distorts channel A using scaled bipolar version of channel B's waveform
		float bendAmt = 1.0f / 48.0f;		// Increase to extend bend amount range
		pitchAdj = outputSamples[1] * (warpAmt - 32767.0f) * bendAmt;
		adjReadPos = readPos[0] + pitchAdj;
		pitchInc[0] *= 1.5f;			// Adding pitchAdj creates odd effects around bend point - sounds better multiplying by average
	}
	break;

	default:
		adjReadPos = readPos[0];
		break;
	}

	if (adjReadPos >= 2048) { adjReadPos -= 2048; }
	if (adjReadPos < 0) { adjReadPos += 2048; }

	return adjReadPos;
}


inline void WaveTable::AdditiveWave()
{
	// Calculate which pair of harmonic sets to interpolate between
	const float harmonicPos = wavetablePos[1].Val() * (harmonicSets - 1);
	const uint32_t harmonicLow = (uint32_t)harmonicPos;
	const float ratio = harmonicPos - harmonicLow;

	float sample = 0.0f;
	float pos = 0.0f;
	float revPos = 0.0f;
	for (uint32_t i = 0; i < harmonicCount; ++i) {
		pos += readPos[1];
		revPos -= readPos[1];
		while (pos >= 2048.0f) {
			pos -= 2048.0f;
		}
		while (revPos < 0.0f) {
			revPos += 2048.0f;
		}

		float harmonicLevel = std::lerp(additiveHarmonics[harmonicLow][i], additiveHarmonics[harmonicLow + 1][i], ratio);
		sample += harmonicLevel * sineLUT[(uint32_t)pos];
	}
	outputSamples[1] = sample;
}


void WaveTable::CalcAdditive()
{
	// Build list of additive waves
	// none = 0, sine1 = 1, sine2 = 2, sine3 = 3, sine4 = 4, sine5 = 5, sine6 = 6, square = 7, saw = 8, triangle = 9
	harmonicSets = 0;
	memset(additiveHarmonics, 0, sizeof(additiveHarmonics));
	for (int8_t i = 7; i > -1; --i) {
		const uint8_t additiveType = (uint8_t)((cfg.additiveWaves >> (i * 4)) & 0xF);

		if (harmonicSets > 0 || (additiveType > 0 && additiveType < 10)) {
			++harmonicSets;
			if (additiveType > 0 && additiveType < 7) {
				additiveHarmonics[harmonicSets - 1][additiveType - 1] = 0.9f;
			} else if ((AdditiveType)additiveType == AdditiveType::saw) {
				for (uint8_t h = 0; h < harmonicCount; ++h) {
					additiveHarmonics[harmonicSets - 1][h] = 0.6f / (h + 1);
				}
			} else if ((AdditiveType)additiveType == AdditiveType::square) {
				for (uint8_t h = 0; h < harmonicCount; h += 2) {
					additiveHarmonics[harmonicSets - 1][h] = 0.9f / (h + 1);
				}
			} else if ((AdditiveType)additiveType == AdditiveType::triangle) {
				float mult = 0.8f;
				for (uint8_t h = 0; h < harmonicCount; h += 2) {
					additiveHarmonics[harmonicSets - 1][h] = mult / std::pow(h + 1, 2);
					mult *= -1.0f;
				}
			}
		}
	}

	if (harmonicSets == 0) {			// If no harmonics configured  create a single sine wave
		harmonicSets = 1;
		additiveHarmonics[0][0] = 0.9f;
	}
}


void WaveTable::Init()
{
	CalcAdditive();
	wavetable.UpdateWavetableList();							// Updated list of samples on flash
}


void WaveTable::ChangeWaveTable(int32_t index)
{
	// Called by UI when changing wavetable
	activeWaveTable = index;
	strncpy(cfg.wavetable, wavList[index].name, 8);
	crossfade = 1.0f;

	config.ScheduleSave();
}


void WaveTable::UpdateWavetableList()
{
	// Create a test wavetable with a couple of waveforms - also used in case of no file system
	strncpy(wavList[0].name, "Default ", 8);
	wavList[0].dataFormat = 3;
	wavList[0].channels   = 1;
	wavList[0].byteDepth  = 4;
	wavList[0].sampleCount = 4096;
	wavList[0].tableCount = 2;
	wavList[0].startAddr = (uint8_t*)&defaultWavetable;
	wavList[0].sampleType = SampleType::Float32;
	wavList[0].invalid = Invalid::OK;

	// Generate test waves
	for (uint32_t i = 0; i < 2048; ++i) {
		defaultWavetable[i] = std::sin((float)i * M_PI * 2.0f / 2048.0f);
		defaultWavetable[i + 2048] = (2.0f * i / 2048.0f) - 1.0f;
	}

	// Updates list of wavetables from FAT root directory
	wavetableCount = 1;
	ReadDir(fatTools.rootDirectory, 0);				// Reads all wavetables in file system and stores metadata in wavList
	std::sort(&wavList[1], &wavList[wavetableCount], &WavetableSorter);

	for (uint32_t i = 0; i < wavetableCount; ++i) {
		if (wavList[i].isDir && wavList[i].name[0] != '.') {

			// Create dummy folder for back navigation
			if (wavetableCount < maxWavetable) {
				Wav& wav = wavList[wavetableCount++];
				strncpy(wav.name, ".", 8);
				strncpy(wav.lfn, "<< Back", lfnSize);
				wav.firstWav = i;
				wav.dir = i;
				wav.isDir = true;
				wav.invalid = Invalid::OK;
			}

			const uint32_t oldWavetableCount = wavetableCount;
			ReadDir((FATFileInfo*)fatTools.GetClusterAddr(wavList[i].cluster), i);
			std::sort(&wavList[oldWavetableCount], &wavList[wavetableCount], &WavetableSorter);
			if (oldWavetableCount != wavetableCount) {		// Valid wav files found in directory
				wavList[i].invalid = Invalid::OK;
			}
		}
	}

	// Blank next sample (if exists) to show end of list
	Wav& wav = wavList[wavetableCount];
	wav.name[0] = 0;

	// Attempt to locate active wavetable
	for (uint32_t i = 0; i < wavetableCount; ++i) {
		if (wavList[i].invalid == Invalid::OK && !wavList[i].isDir && strncmp(wavList[i].name, cfg.wavetable, 8) == 0) {
			activeWaveTable = i;
			break;
		}
	}
	ui.SetWavetable(activeWaveTable);
}


void WaveTable::ReadDir(FATFileInfo* dirEntry, uint32_t dirIndex)
{
	if (fatTools.noFileSystem) {
		return;
	}

	// Store contents of a directory into the wavetable and directory lists
	while (dirEntry->name[0] != 0 && wavetableCount < maxWavetable) {
		const bool isValidDir = dirEntry->name[0] != '.' &&	(dirEntry->attr & AM_DIR) && (dirEntry->attr & AM_HID) == 0 && (dirEntry->attr & AM_SYS) == 0;
		const bool isValidWav = (dirEntry->attr & AM_DIR) == 0 && strncmp(&(dirEntry->name[8]), "WAV", 3) == 0 && dirEntry->firstClusterLow;

		if (dirEntry->name[0] != FATFileInfo::fileDeleted && dirEntry->attr == FATFileInfo::LONG_NAME) {
			// Store long file name in temporary buffer
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
		} else if (dirEntry->name[0] != FATFileInfo::fileDeleted && (isValidWav || isValidDir)) {
			Wav& wav = wavList[wavetableCount];
			memset(&wav, 0, sizeof(wav));

			strncpy(wav.name, dirEntry->name, 8);
			if (lfnPosition > 0) {
				CleanLFN(wav.lfn);
			}
			wav.dir = dirIndex;
			wav.cluster = dirEntry->firstClusterLow;

			if (isValidWav) {
				wav.size = dirEntry->fileSize;
				GetWavInfo(wav);
			} else {
				wav.isDir = true;
				wav.invalid = Invalid::EmptyFolder;			// Will be set to OK if contains valid wavetables
			}

			// If storing the first file in a sub directory, store the index against the directory item
			if (dirIndex > 0 && wavList[dirIndex].firstWav == 0) {
				wavList[dirIndex].firstWav = wavetableCount;
			}

			wavetableCount++;
		} else {
			lfnPosition = 0;
		}
		dirEntry++;
	}
}


void WaveTable::GetWavInfo(Wav& wav)
{
	// populate the wavetable object with sample rate, number of channels etc
	const uint8_t* wavHeader = fatTools.GetClusterAddr(wav.cluster, true);

	// Check validity
	if (*(uint32_t*)wavHeader != 0x46464952) {					// wav file should start with letters 'RIFF'
		wav.invalid = Invalid::HeaderCorrupt;
		return;
	}

	// Jump through chunks looking for 'fmt' chunk
	uint32_t pos = 12;											// First chunk ID at 12 byte (4 word) offset
	while (*(uint32_t*)&(wavHeader[pos]) != 0x20746D66) {		// Look for string 'fmt '
		pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));			// Each chunk title is followed by the size of that chunk which can be used to locate the next one
		if  (pos > 1000) {
			wav.invalid = Invalid::HeaderCorrupt;
			return;
		}
	}

	wav.dataFormat = *(uint16_t*)&(wavHeader[pos + 8]);			// 1 = PCM integer; 3 = float
	wav.channels   = *(uint16_t*)&(wavHeader[pos + 10]);
	wav.byteDepth  = *(uint16_t*)&(wavHeader[pos + 22]) / 8;

	// Check if there is a clm chunk with Serum metadata
	pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));
	if (*(uint32_t*)&(wavHeader[pos]) == 0x206d6c63) {			// Look for string 'clm '
		const char* metadata = (char*)&(wavHeader[pos + 16]);
		if (std::strspn(metadata, "0123456789") == 8) {
			wav.metadata = std::stoi(metadata);
		}
	}

	// Navigate forward to find the start of the data area
	while (*(uint32_t*)&(wavHeader[pos]) != 0x61746164) {		// Look for string 'data'
		pos += (8 + *(uint32_t*)&(wavHeader[pos + 4]));
		if (pos > 1200) {
			wav.invalid = Invalid::HeaderCorrupt;
			return;
		}
	}

	const uint32_t startOffset = pos + 8;
	wav.dataSize = *(uint32_t*)&(wavHeader[pos + 4]);			// Num Samples * Num Channels * Bits per Sample / 8

	// Handle issue where dataSize is incorrectly reported (seems to be when exporting 16 bit wavetables from Serum)
	if (wav.dataSize > wav.size) {
		if (wav.size - startOffset == wav.dataSize / 2) {
			wav.dataSize = wav.dataSize / 2;
		}
	}

	wav.sampleCount = wav.dataSize / (wav.channels * wav.byteDepth);
	wav.startAddr = &(wavHeader[startOffset]);
	wav.tableCount = wav.sampleCount / 2048;

	// Currently support 32 bit floats and 16 bit PCM integer formats
	if (wav.byteDepth == 4 && wav.dataFormat == 3) {
		wav.sampleType = SampleType::Float32;
	} else if (wav.byteDepth == 2 && wav.dataFormat == 1) {
		wav.sampleType = SampleType::PCM16;
	} else {
		wav.sampleType = SampleType::Unsupported;
	}

	// Follow cluster chain and store last cluster if not contiguous to tell playback engine when to do a fresh address lookup
	uint32_t cluster = wav.cluster;
	wav.lastCluster = 0xFFFFFFFF;
	while (fatTools.clusterChain[cluster] != 0xFFFF && fatTools.clusterChain[cluster] != 0) {
		if (fatTools.clusterChain[cluster] != cluster + 1 && wav.lastCluster == 0xFFFFFFFF) {		// Store cluster at first discontinuity of chain
			wav.lastCluster = cluster;
			wav.fragmented = true;
			break;
		}
		cluster = fatTools.clusterChain[cluster];
	}

	if (wav.sampleType == SampleType::Unsupported) {
		wav.invalid = Invalid::SampleFormat;
	} else if (wav.channels != 1) {
		wav.invalid = Invalid::ChannelCount;
	} else if (wav.dataSize > wav.size) {
		wav.invalid = Invalid::HeaderCorrupt;
	} else if (wav.fragmented) {
		wav.invalid = Invalid::Fragmented;
	}
}


bool WaveTable::WavetableSorter(Wav const& lhs, Wav const& rhs) {
	// Sort wavetables with sub-folders first followed by short file name
	if (lhs.isDir != rhs.isDir) {
		return lhs.isDir;
	}
	for (uint8_t i = 0; i < 8; ++i) {
		if (lhs.name[i] != rhs.name[i]) {
			return lhs.name[i] < rhs.name[i];
		}
	}
	return true;
}


void WaveTable::CleanLFN(char* storeName)
{
	// Clean long file name and store in wavetable or directory list
	longFileName[lfnPosition] = '\0';
	std::string_view lfn {longFileName};
	size_t copyLen = std::min(lfn.find(".wav"), lfnSize);
	std::strncpy(storeName, longFileName, copyLen);
	lfnPosition = 0;
}


// Algorithm source: https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
float WaveTable::FastTanh(float x)
{
	const float x2 = x * x;
	const float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
	const float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
	return a / b;
}


void WaveTable::UpdateConfig()
{
	wavetable.ChannelBOctave();
}


void WaveTable::ChannelBOctave(bool change)
{
	if (change) {
		cfg.octaveChnB = !cfg.octaveChnB;
		config.ScheduleSave();
	}
	if (cfg.octaveChnB) {
		octaveLED.SetHigh();
	} else {
		octaveLED.SetLow();
	}

}


void WaveTable::WarpButton(bool change)
{
	if (change) {
		cfg.warpButton = !cfg.warpButton;
		config.ScheduleSave();
	}
	crossfade = 1.0f;
}


void WaveTable::FragChain()
{
	// Build lookup table of fragmented memory addresses
	Wav& wav = wavList[activeWaveTable];

	if (wav.fragmented) {
		fragChain[0].startAddr = wav.startAddr;
		uint32_t cluster = wav.lastCluster;
		uint32_t i = 1;
		while (i < 16 && fatTools.clusterChain[cluster] != 0xFFFF) {
			if (fatTools.clusterChain[cluster] != cluster + 1) {
				fragChain[i - 1].endAddr = fatTools.GetClusterAddr(cluster, true);
				fragChain[i].startAddr = fatTools.GetClusterAddr(fatTools.clusterChain[cluster], true);
				++i;
			}
			cluster = fatTools.clusterChain[cluster];
		}
	}
}
