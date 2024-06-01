#pragma once

#include "initialisation.h"
#include "Filter.h"
#include "GpioPIn.h"
#include <string_view>

struct WaveTable {
	friend class CDCHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow the config access to private data to save settings
public:
	void CalcSample();						// Called by interrupt handler to generate next sample
	void Init();							// Initialise caches, buffers etc
	bool LoadWaveTable(uint32_t* startAddr);
	void Draw();

	float testWavetable[2048];
	bool bufferClear = true;				// Used to manage blanking draw buffers using DMA
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
	int32_t OutputMix(float wetSample);
	void OutputSample(uint8_t channel, float ratio);
	float FastTanh(float x);
	float CalcWarp();
	void AdditiveWave();

	enum class TestData {noise, twintone, wavetable} wavetableType = TestData::wavetable;

	float* activeWaveTable;
	struct {
		volatile uint16_t& adcControl;
		float pos;		// Smoothed ADC value
	} channel[2] = {{adc.DelayCV_L, 0.0f}, {adc.FilterPot, 0.0f}};

	float pitchInc = 0.0f;
	float readPos = 0;
	int32_t oldReadPos;

	bool stepped = false;
	int32_t warpVal = 0;					// Used for setting hysteresis on warp type
	Warp oldWarpType = Warp::none;

	float debugTiming;

	uint8_t drawData[240];
	bool activeDrawBuffer = true;


	GpioPin debugMain{GPIOE, 2, GpioPin::Type::Output};		// PE2: Debug
	GpioPin debugDraw{GPIOE, 3, GpioPin::Type::Output};		// PE3: Debug
};

extern WaveTable wavetable;
