#pragma once

#include "initialisation.h"
#include "Filter.h"


struct WaveTable {
	friend class CDCHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow the config access to private data to save settings
public:
	void CalcSample();						// Called by interrupt handler to generate next sample
	void Init();							// Initialise caches, buffers etc
	bool LoadWaveTable(uint32_t* startAddr);
	bool UpdateSampleList();

	float testWavetable[2048];

	struct Sample {
		char name[11];
		uint32_t size;						// Size of file in bytes
		uint32_t cluster;					// Starting cluster
		uint32_t lastCluster;				// If file spans multiple clusters store last cluster before jump - if 0xFFFFFFFF then clusters are contiguous
		const uint8_t* startAddr;			// Address of data section
		const uint8_t* endAddr;				// End Address of data section
		uint32_t dataSize;					// Size of data section in bytes
		uint32_t sampleCount;				// Number of samples (stereo samples only counted once)
		uint32_t sampleRate;
		uint8_t byteDepth;
		uint16_t dataFormat;				// 1 = PCM; 3 = Float
		uint8_t channels;					// 1 = mono, 2 = stereo
		bool valid;							// false if header cannot be processed
	} sampleList[128];

	struct WavFile {
		const uint8_t* startAddr;			// Address of data section
		uint32_t fileSize;
		uint32_t dataSize;					// Size of data section in bytes
		uint32_t sampleCount;				// Number of samples (stereo samples only counted once)
		uint32_t sampleRate;
		uint8_t byteDepth;
		uint16_t dataFormat;				// 1 = PCM; 3 = Float
		uint8_t channels;					// 1 = mono, 2 = stereo
		uint16_t tableCount;				// Number of 2048 sample wavetables in file
		bool valid;							// false if header cannot be processed
	} wavFile;

private:
	enum class TestData {noise, twintone, wavetable};
	TestData wavetableType = TestData::wavetable;

	char longFileName[100];
	uint8_t lfnPosition = 0;

	float* activeWaveTable;
	float wavetableIdx;

	float pitchInc = 0.0f;
	float readPos = 0;
	int32_t oldReadPos;

	float debugTiming;

	// Private class functions
	int32_t OutputMix(float wetSample);
	float FastTanh(float x);
	int32_t ParseInt(const std::string_view cmd, const std::string_view precedingChar, const int32_t low, const int32_t high);
};

extern WaveTable wavetable;
