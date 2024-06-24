#pragma once

/*
 * Much of the filter code gratefully taken from Iowa Hills Software
 * http://www.iowahills.com/
 */

#include "initialisation.h"
#include <cmath>
#include <complex>
#include <array>

struct Filter {
	friend class SerialHandler;				// Allow the serial handler access to private data for printing
	friend class Config;					// Allow access to config to store values
public:

	void Init();

	template<typename sampleType>
	float CalcInterpolatedFilter(int32_t pos, sampleType* waveTable, const float ratio, const float pitchInc)
	{
		// FIR Convolution routine using folded FIR structure.
		// Samples are interpolated according to ratio and filtering is carried out to minimise multiplications

		// Calculate filter coefficient lookup table (converts pitch increment from exponential to linear scale)
		const uint32_t lutPos = std::min(uint32_t(std::round(std::log2(pitchInc) * lutLookupMult)), lutSize - 1);
		auto& filterCoeff = filterLUT[lutPos].coeff;

		if (ratio < 0.00001f) {								// To avoid a divide by zero and for better performance
			return CalcFilter(pos, waveTable, lutPos);
		}

		float outputSample = 0.0f;

		const float invertedRatio = 1.0f / ratio - 1.0f;	// half the samples are interpolated during filtering, and then the total normalised

		// pos = position of sample N, N-1, N-2 etc
		int32_t revpos = (pos - firTaps + 1) & 0x7FF;		// position of sample 1, 2, 3 etc

		for (uint8_t i = 0; i < firTaps / 2; ++i) {
			// Folded FIR structure - as coefficients are symmetrical we can multiple the sample 1 + sample N by the 1st coefficient, sample 2 + sample N - 1 by 2nd coefficient etc
			const int32_t pos2 = (pos + 1) & 0x7FF;
			const int32_t revpos2 = (revpos + 1) & 0x7FF;

			outputSample += filterCoeff[i] * ((invertedRatio * (waveTable[revpos] + waveTable[pos]) +
					(waveTable[revpos2] + waveTable[pos2])));

			revpos = revpos2;
			pos = (pos - 1) & 0x7FF;
		}

		outputSample += filterCoeff[firTaps / 2] * (invertedRatio * waveTable[revpos] + waveTable[(revpos + 1) & 0x7FF]);

		return outputSample * ratio;
	}


	template<typename sampleType>
	float CalcFilter(int32_t pos, sampleType* waveTable, const uint32_t lutPos)
	{
		// FIR Convolution routine using folded FIR structure
		float outputSample = 0.0f;
		auto& filterCoeff = filterLUT[lutPos].coeff;

		// pos = position of sample N, N-1, N-2 etc
		int32_t revpos = (pos - firTaps + 1) & 0x7FF;		// position of sample 1, 2, 3 etc

		for (uint8_t i = 0; i < firTaps / 2; ++i) {
			// Folded FIR structure - as coefficients are symmetrical we can multiple the sample 1 + sample N by the 1st coefficient, sample 2 + sample N - 1 by 2nd coefficient etc
			outputSample += filterCoeff[i] * (waveTable[revpos] + waveTable[pos]);
			revpos = (revpos + 1) & 0x7FF;
			pos = (pos - 1) & 0x7FF;
		}

		outputSample += filterCoeff[firTaps / 2] * waveTable[revpos];

		return outputSample;
	}


	float windowBeta = 4;					// between 0.0 and 10.0 - trade-off between stop band attenuation and filter transistion width

private:
	static constexpr uint32_t firTaps = 31;
	static constexpr uint32_t foldedFirTaps = (firTaps + 1) / 2;
	static constexpr uint32_t lutSize = 90;
	static constexpr uint32_t lutRange = 7;
	static constexpr float lutLookupMult = (float)lutSize / (float)lutRange;

	void BuildLUT();
	float Sinc(const float x);
	void FIRFilterWindow();
	float Bessel(const float x);

	float winCoeff[firTaps];

	struct {
		float log;
		float inc;
		float coeff[foldedFirTaps];
	} filterLUT[lutSize];

};


extern Filter filter;


