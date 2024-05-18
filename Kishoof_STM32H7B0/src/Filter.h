#pragma once

/*
 * Much of the filter code gratefully taken from Iowa Hills Software
 * http://www.iowahills.com/
 */

#include "initialisation.h"
#include "GpioPin.h"
#include <cmath>
#include <complex>
#include <array>

static constexpr int maxFIRTaps = 93;

struct Filter {
	friend class SerialHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow access to config to store values
public:

	void Init();
	void Update(bool reset = false);
	float CalcFilter(int32_t pos, float* wavetable);
	float CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio);

	float cutoff;
	float windowBeta = 4;					// between 0.0 and 10.0 - trade-off between stop band attenuation and filter transistion width

private:
	uint8_t firTaps = 29;

	bool activateFilter = true;				// For debug
	bool activeFilter = 0;					// For double-buffering coefficients (so coefficient calculation won't interfere with current filter)
	float currentCutoff;

	// FIR Settings
	float firCoeff[2][maxFIRTaps];
	float winCoeff[maxFIRTaps];

	void InitFIRFilter(float tone);
	float Sinc(float x);
	void FIRFilterWindow();
	float Bessel(float x);
	void SwitchFilter();

	//GpioPin debugFilter{GPIOE, 3, GpioPin::Type::Output};		// PE3: Debug
};


extern Filter filter;


