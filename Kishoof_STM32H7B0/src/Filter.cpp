#include "Filter.h"


Filter filter;

void Filter::Init()
{
	FIRFilterWindow();
	BuildLUT();
}


void Filter::BuildLUT()
{
	// Build a lookup table

	GpioPin::SetHigh(GPIOD, 6);
	// Eg is sampling freq = 20 kHz, then Nyquist = 10 kHz. LPF with 3 dB corner at 1 kHz set omega = 0.1 (1 kHz / 10 kHz)
	// Each LUT is for an integer pitch increment from 1 to 90 (maximum pitch increment with 5V and octave up
	const float inc = (float)lutRange / (float)lutSize;
	for (uint32_t i = 0; i < lutSize; ++i) {
		float pitchInc = std::pow(2.0f, inc * i);
		const float omega = 1.0f / pitchInc;
		filterLUT[i].inc = pitchInc;
		for (int8_t j = 0; j < (int8_t)foldedFirTaps; ++j) {
			const int8_t  arg = j - (firTaps - 1) / 2;
			filterLUT[i].coeff[j] = omega * Sinc(omega * arg * M_PI) * winCoeff[j];
		}
	}
	GpioPin::SetLow(GPIOD, 6);
}


float Filter::CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio, float pitchInc)
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


float Filter::CalcFilter(int32_t pos, float* waveTable, uint32_t lutPos)
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


float Filter::Sinc(float x)
{
	if (x > -1.0E-5 && x < 1.0E-5) {
		return 1.0f;
	}
	return (std::sin(x) / x);
}


void Filter::FIRFilterWindow()
{
	// Kaiser window
	for (uint8_t j = 0; j < firTaps; j++) {
		const float arg = windowBeta * sqrt(1.0f - pow( (static_cast<float>(2 * j) + 1 - firTaps) / (firTaps + 1), 2.0) );
		winCoeff[j] = Bessel(arg) / Bessel(windowBeta);
	}
}


// Used for Kaiser window calculations
float Filter::Bessel(float x)
{
	float sum = 0.0f;

	for (uint8_t i = 1; i < 10; ++i) {
		float xPower = pow(x / 2.0f, static_cast<float>(i));
		int factorial = 1;
		for (uint8_t j = 1; j <= i; ++j) {
			factorial *= j;
		}
		sum += pow(xPower / static_cast<float>(factorial), 2.0f);
	}
	return 1.0f + sum;
}



