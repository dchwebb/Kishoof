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

struct Filter {
	friend class SerialHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow access to config to store values
public:

	void Init();
	void Update(bool reset = false);
	float CalcFilter(int32_t pos, float* wavetable);
	float CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio);
	float CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio, float pitchInc);

	float cutoff;
	float windowBeta = 4;					// between 0.0 and 10.0 - trade-off between stop band attenuation and filter transistion width

private:
	static constexpr uint32_t firTaps = 29;
	static constexpr uint32_t lutSize = 90;
	static constexpr uint32_t lutRange = 7;
	static constexpr float lutLookupMult = (float)lutSize / (float)lutRange;

	bool activateFilter = true;				// For debug
	bool activeFilter = 0;					// For double-buffering coefficients (so coefficient calculation won't interfere with current filter)
	float currentCutoff;

	// FIR Settings
	float firCoeff[2][firTaps];
	float winCoeff[firTaps];

	void BuildLUT();
	void InitFIRFilter(float tone);
	float Sinc(float x);
	void FIRFilterWindow();
	float Bessel(float x);
	void SwitchFilter();


	struct {
		float log;
		float inc;
		float coeff[(firTaps + 1) / 2];
	} filterLUT[lutSize];

	//GpioPin debugFilter{GPIOE, 3, GpioPin::Type::Output};		// PE3: Debug
};


extern Filter filter;


