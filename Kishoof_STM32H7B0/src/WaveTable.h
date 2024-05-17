#pragma once

#include "initialisation.h"
#include "Filter.h"
#include "GpioPIn.h"
#include "FatTools.h"
#include <string_view>

struct WaveTable {
	friend class CDCHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow the config access to private data to save settings
	friend class UI;
public:
	void CalcSample();						// Called by interrupt handler to generate next sample
	void Init();							// Initialise caches, buffers etc
	bool LoadWaveTable(uint32_t* startAddr);
	void Draw();
	bool UpdateWavetableList();
	void ChangeWaveTable(int32_t upDown);

	float testWavetable[4096];

	float outputSamples[2] = {0.0f, 0.0f};

	enum class Warp {none, squeeze, bend, mirror, reverse, tzfm, count} warpType = Warp::none;
	static constexpr std::string_view warpNames[] = {" NONE  ", "SQUEEZE", " BEND  ", "MIRROR ", "REVERSE", " TZFM  "};

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

	static constexpr uint32_t harmonicSets = 3;
	static constexpr uint32_t harmonicCount = 10;
	float additiveHarmonics[harmonicSets][harmonicCount] = {
		{ 0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },								// Sine
		{ 0.9f, 0.0f, 0.9f / 3.0f, 0.0f, 0.9f / 5.0f, 0.0f, 0.9f / 7.0f, 0.0f, 0.9f / 9.0f, 0.0f },	// Square
		{ 0.6f, -0.6f / 2.0f, 0.6f / 3.0f, -0.6f / 4.0f, 0.6f / 5.0f, -0.6f / 6.0f, 0.6f / 7.0f, -0.6f / 8.0f, 0.6f / 9.0f, -0.6f / 10.0f } 		// Sawtooth
	};

	static constexpr uint32_t sinLUTSize = 2048;
	constexpr auto CreateSinLUT()		// constexpr function to generate LUT in Flash
	{
		std::array<float, sinLUTSize + 1> array {};		// Create one extra entry to simplify interpolation
		for (uint32_t s = 0; s < sinLUTSize + 1; ++s){
			array[s] = std::sin(s * 2.0f * std::numbers::pi / sinLUTSize);
		}
		return array;
	}

private:
	static constexpr uint32_t maxWavetable = 128;
	static constexpr float scaleOutput = -std::pow(2.0f, 31.0f);	// Multiple to convert -1.0 - 1.0 float to 32 bit int and invert

	struct Wav {
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
		uint16_t tableCount;				// Number of 2048 sample wavetables in file
		bool valid;							// false if header cannot be processed
	} wavList[maxWavetable];

	void OutputSample(uint8_t channel, float ratio);
	float FastTanh(float x);
	float CalcWarp();
	void AdditiveWave();
	int32_t ParseInt(const std::string_view cmd, const std::string_view precedingChar, const int32_t low, const int32_t high);
	bool GetWavInfo(Wav* sample);

	enum class TestData {noise, twintone, testwaves, wavetable} wavetableType = TestData::wavetable;

	char longFileName[100];
	uint8_t lfnPosition = 0;

	uint32_t activeWaveTable;
	char waveTableName[11];			// Store name of active wavetable so can be relocated after disk activity
	uint32_t wavetableCount;

	struct {
		volatile uint16_t& adcControl;
		float pos;		// Smoothed ADC value
	} channel[2] = {{adc.Wavetable_Pos_A_Pot, 0.0f}, {adc.Wavetable_Pos_B_Pot, 0.0f}};



	float pitchInc = 0.0f;
	float readPos = 0;
	int32_t oldReadPos;

	bool stepped = false;
	int32_t warpVal = 0;					// Used for setting hysteresis on warp type
	Warp oldWarpType = Warp::none;

	float debugTiming;

	uint8_t drawData[240];


	GpioPin modeSwitch{GPIOE, 2, GpioPin::Type::Input};			// PE2: Mode switch
	GpioPin octaveUp{GPIOD, 0, GpioPin::Type::InputPulldown};			// PD0: Octave_Up
	GpioPin octaveDown{GPIOD, 1, GpioPin::Type::InputPulldown};			// PD1: Octave_Down

	GpioPin debugMain{GPIOD, 6, GpioPin::Type::Output};			// PD5: Debug
	GpioPin debugDraw{GPIOD, 5, GpioPin::Type::Output};			// PD6: Debug
};

extern WaveTable wavetable;
