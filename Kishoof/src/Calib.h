#pragma once

#include "initialisation.h"
#include "configManager.h"

class Calib {
public:
	void Calibrate(char key = 0);
	static void UpdateConfig();

	// 0v = 61200; 1v = 50110; 2v = 39020; 3v = 27910; 4v = 16790; 5v = 5670
	// C: 16.35 Hz 32.70 Hz; 65.41 Hz; 130.81 Hz; 261.63 Hz; 523.25 Hz; 1046.50 Hz; 2093.00 Hz; 4186.01 Hz
	// 61200 > 65.41 Hz; 50110 > 130.81 Hz; 39020 > 261.63 Hz

	// Increment = Hz * (2048 / 48000) = Hz * (wavetablesize / samplerate)
	// Pitch calculations - Increase pitchBase to increase pitch; Reduce ABS(cvMult) to increase spread

	// Calculations: 11330 is difference in cv between two octaves; 50050 is cv at 1v and 130.81 is desired Hz at 1v

	static constexpr float pitchBaseDef = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, -50050.0f / 11330.0f);
	static constexpr float pitchMultDef = -1.0f / 11330.0f;
	static constexpr uint16_t vcaNormalDef = 24000;

	struct {
		float pitchBase = pitchBaseDef;
		float pitchMult = pitchMultDef;
		uint16_t vcaNormal = vcaNormalDef;
	} cfg;

	ConfigSaver configSaver = {
		.settingsAddress = &cfg,
		.settingsSize = sizeof(cfg),
		.validateSettings = UpdateConfig
	};

	bool calibrating;			// Triggered by serial console
	enum class State {Waiting0, Waiting1, Octave0, Octave1, PendingSave};
	State state;

	float adcOctave0;
	float adcOctave1;
	float adcVCA;
	uint32_t calibCount = 0;
};

extern Calib calib;
