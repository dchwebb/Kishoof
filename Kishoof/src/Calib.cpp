#include <Calib.h>
#include <cstdio>

Calib calib;

void Calib::Calibrate(char key)
{
	// Can be called either from main (key = 0) to update calibration values or from serial console (key != 0) to update state
	if (key == 0 && calibrating) {
		if (state == State::Octave0) {
			adcOctave0 += (float)adc.Pitch_CV;
			adcVCA += (float)adc.VcaCV;
			if (++calibCount == 2000) {
				printf("Pitch reading: %d (expected around 61000)\r\n"
						"VCA reading: %d (expected around 24000)\r\n"
						"Apply 1V to Pitch input\r\n"
						"Enter 'y' to continue, 'x' to cancel\r\n",
						(int)(adcOctave0 / 2000.0f),
						(int)(adcVCA / 2000.0f)
						);
				state = State::Waiting1;
			}
		}
		if (state == State::Octave1) {
			adcOctave1 += (float)adc.Pitch_CV;
			if (++calibCount == 2000) {
				printf("Pitch reading: %d (expected around 50000)\r\n"
						"Enter 'y' to save calibration, 'x' to cancel\r\n",
						(int)(adcOctave1 / 2000.0f)
						);
				state = State::PendingSave;
			}
		}
	}

	// Start instruction
	if (key == 's') {
		calibrating = true;
		printf("Calibrating\r\n"
				"Disconnect VCA and apply 0V (lowest C) to Pitch input\r\n"
				"Enter 'y' to continue, 'x' to cancel\r\n");
		state = State::Waiting0;
	}

	if (key == 'y') {
		switch (state) {
		case State::Waiting0:
			state = State::Octave0;
			adcOctave0 = (float)adc.Pitch_CV;
			adcVCA = (float)adc.VcaCV;
			calibCount = 1;
			break;
		case State::Waiting1:
			state = State::Octave1;
			adcOctave1 = (float)adc.VcaCV;
			calibCount = 1;
			break;
		case State::PendingSave: {
			state = State::Waiting0;
			const float voltSpread = (adcOctave0 - adcOctave1) / 2000.0f;

			cfg.pitchBase = (65.41f * (2048.0f / sampleRate)) / std::pow(2.0, (-adcOctave1 / 2000.0f) / voltSpread);
			cfg.pitchMult = -1.0f / voltSpread;
			cfg.vcaNormal = (uint16_t)std::round(adcVCA / 2000.0f);

			printf("Calibration saved\r\n");
			config.SaveConfig(true);
			calibrating = false;
			}
			break;
		default:
			break;
		}
	}

	if (key == 'x') {
		printf("Cancelled calibration\r\n");
		calibrating = false;
	}

}


