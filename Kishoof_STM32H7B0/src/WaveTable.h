#pragma once

#include <string_view>

#include "initialisation.h"
#include "Filter.h"
#include "GpioPin.h"
#include "FatTools.h"
#include "configManager.h"

struct WaveTable {
	friend class CDCHandler;					// Allow the serial handler access to private data for debug printing
	friend class Config;						// Allow the config access to private data to save settings
	friend class UI;
public:
	void CalcSample();							// Called by interrupt handler to generate next sample
	void Init();								// Initialise caches, buffers etc
	bool LoadWaveTable(uint32_t* startAddr);
	void Draw();
	bool UpdateWavetableList();
	void ChangeWaveTable(int32_t upDown);
	void ChannelBOctave(bool change = false);	// Called when channel B octave button is pressed
	static void UpdateConfig();

	struct {
		char wavetable[8];
		bool octaveChnB = false;
	} cfg;

	ConfigSaver configSaver = {
		.settingsAddress = &cfg,
		.settingsSize = sizeof(cfg),
		.validateSettings = &WaveTable::UpdateConfig
	};

	enum class Warp {none, squeeze, bend, mirror, reverse, tzfm, count} warpType = Warp::none;
	static constexpr std::string_view warpNames[] = {" NONE  ", "SQUEEZE", " BEND  ", "MIRROR ", "REVERSE", " TZFM  "};
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
		char name[8];
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
		uint32_t metadata;					// Serum metadata
		bool valid;							// false if header cannot be processed
	} wavList[maxWavetable];

	void OutputSample(uint8_t channel, float ratio);
	float FastTanh(float x);
	float CalcWarp();
	void AdditiveWave();
	int32_t ParseInt(const std::string_view cmd, const std::string_view precedingChar, const int32_t low, const int32_t high);
	bool GetWavInfo(Wav* sample);

	float defaultWavetable[4096];				// Built-in wavetable
	float outputSamples[2] = {0.0f, 0.0f};

	char longFileName[100];
	uint8_t lfnPosition = 0;

	uint32_t activeWaveTable;
	uint32_t wavetableCount;

	struct {
		volatile uint16_t& adcPot;
		volatile uint16_t& adcCV;
		float pos;		// Smoothed ADC value

		float Val() {
			constexpr float scaleOutput = 0.01f / 65536.0f;		// scale constant for two 16 bit values and filter
			pos = (0.99f * pos) + std::clamp((adcPot + 61000.0f - adcCV), 0.0f, 65535.0f) * scaleOutput;	// Reduce to ensure can hit zero with noise
			return pos;
		}
	} wavetablePos[2] = {{adc.Wavetable_Pos_A_Pot, adc.WavetablePosA_CV, 0.0f}, {adc.Wavetable_Pos_B_Pot, adc.WavetablePosB_CV, 0.0f}};



	float smoothedInc = 0.0f;
	float pitchInc[2] = {0.0f, 0.0f};
	float readPos[2] = {0.0f, 0.0f};

	bool stepped = false;
	int32_t warpVal = 0;					// Used for setting hysteresis on warp type
	Warp oldWarpType = Warp::none;

	uint8_t drawData[240];

	GpioPin modeSwitch	{GPIOE, 2, GpioPin::Type::Input};			// PE2: Mode switch
	GpioPin octaveUp	{GPIOD, 0, GpioPin::Type::InputPulldown};	// PD0: Octave_Up
	GpioPin octaveDown	{GPIOD, 1, GpioPin::Type::InputPulldown};	// PD1: Octave_Down
	GpioPin chBMix		{GPIOD, 8, GpioPin::Type::InputPulldown};	// PD8: ChB_Mix
	GpioPin chBRingMod	{GPIOD, 9, GpioPin::Type::InputPulldown};	// PD9: ChB_RM

	GpioPin octaveLED	{GPIOD, 14, GpioPin::Type::Output};			// PD14: LED_Oct_B

	GpioPin debugMain	{GPIOD, 6, GpioPin::Type::Output};			// PD5: Debug
	GpioPin debugDraw	{GPIOD, 5, GpioPin::Type::Output};			// PD6: Debug
};

extern WaveTable wavetable;
