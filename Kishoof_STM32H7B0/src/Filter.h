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
	float CalcFilter(int32_t pos, float* waveTable, uint32_t lutPos);
	float CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio, float pitchInc);

	float windowBeta = 4;					// between 0.0 and 10.0 - trade-off between stop band attenuation and filter transistion width

private:
	static constexpr uint32_t firTaps = 31;
	static constexpr uint32_t foldedFirTaps = (firTaps + 1) / 2;
	static constexpr uint32_t lutSize = 90;
	static constexpr uint32_t lutRange = 7;
	static constexpr float lutLookupMult = (float)lutSize / (float)lutRange;

	void BuildLUT();
	float Sinc(float x);
	void FIRFilterWindow();
	float Bessel(float x);

	float winCoeff[firTaps];

	struct {
		float log;
		float inc;
		float coeff[foldedFirTaps];
	} filterLUT[lutSize];

};


extern Filter filter;


