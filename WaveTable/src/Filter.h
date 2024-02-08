#pragma once

/*
 * Much of the filter code gratefully taken from Iowa Hills Software
 * http://www.iowahills.com/
 */

#include "initialisation.h"
#include <cmath>
#include <complex>
#include <array>


#define MAX_FIR_TAPS 93

typedef std::complex<double> complex_t;

struct Filter {
	friend class SerialHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow access to config to store values
public:

	void Init();
	void Update(bool reset);
	float CalcFilter(int32_t pos, float* wavetable);
private:
	uint8_t firTaps = 93;	// value must be divisble by four + 1 (eg 93 = 4*23 + 1) or will cause phase reversal when switching between LP and HP

	bool activateFilter = true;				// For debug
	bool activeFilter = 0;					// choose which set of coefficients to use (so coefficients can be calculated without interfering with current filtering)
	float cutoff;
	float currentCutoff;

	// FIR Settings
	float firCoeff[2][MAX_FIR_TAPS];
	float winCoeff[MAX_FIR_TAPS];

	void InitFIRFilter(float tone);
	float Sinc(float x);
	void FIRFilterWindow();
	float Bessel(float x);
	void SwitchFilter();
};


extern Filter filter;


