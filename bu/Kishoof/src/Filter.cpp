#include "Filter.h"


Filter filter;

void Filter::Init()
{
	filter.FIRFilterWindow();
	currentCutoff = (float)adc.FilterPot / 65536.0f;
	cutoff = currentCutoff;
	filter.Update(true);
}


void Filter::Update(bool reset)
{
	//cutoff = 0.999f * cutoff + 0.001 * (float)adc.FilterPot / 65536.0f;

	if (reset || std::abs(cutoff - currentCutoff) > 0.001) {
		debugFilter.SetHigh();

		// round cutoff to 3dp
		float omega = std::round(cutoff * 1000.0f) / 1000.0f;
		InitFIRFilter(omega);
		activeFilter = !activeFilter;

		debugFilter.SetLow();
	}

}



// Rectangular FIR
void Filter::InitFIRFilter(float omega)
{
	// Eg is sampling freq = 20 kHz, then Nyquist = 10 kHz. LPF with 3 dB corner at 1 kHz set omega = 0.1 (1 kHz / 10 kHz)

	// cycle between two sets of coefficients so one can be changed without affecting the other
	const bool inactiveFilter = !activeFilter;

	for (int8_t j = 0; j < firTaps / 2 + 1; ++j) {
		int8_t  arg = j - (firTaps - 1) / 2;
		firCoeff[inactiveFilter][j] = omega * Sinc(omega * arg * M_PI) * winCoeff[j];
	}

	currentCutoff = omega;
}


float Filter::CalcInterpolatedFilter(int32_t pos, float* waveTable, float ratio)
{
	// FIR Convolution routine using folded FIR structure.
	// Samples are interpolated according to ratio and filtering is carried out to minimise multiplications

	if (ratio < 0.00001f) {								// To avoid a divide by zero and for better performance
		return CalcFilter(pos, waveTable);
	}

	float outputSample = 0.0f;

	const float invertedRatio = 1.0f / ratio - 1.0f;	// half the samples are interpolated during filtering, and then the total normalised

	// pos = position of sample N, N-1, N-2 etc
	int32_t revpos = (pos - firTaps + 1) & 0x7FF;		// position of sample 1, 2, 3 etc

	for (uint8_t i = 0; i < firTaps / 2; ++i) {
		// Folded FIR structure - as coefficients are symmetrical we can multiple the sample 1 + sample N by the 1st coefficient, sample 2 + sample N - 1 by 2nd coefficient etc
		const int32_t pos2 = (pos + 1) & 0x7FF;
		const int32_t revpos2 = (revpos + 1) & 0x7FF;

		outputSample += firCoeff[activeFilter][i] * ((invertedRatio * (waveTable[revpos] + waveTable[pos]) +
				(waveTable[revpos2] + waveTable[pos2])));

		revpos = revpos2;
		pos = (pos - 1) & 0x7FF;
	}

	outputSample += firCoeff[activeFilter][firTaps / 2] * (invertedRatio * waveTable[revpos] + waveTable[(revpos + 1) & 0x7FF]);

	return outputSample * ratio;
}


float Filter::CalcFilter(int32_t pos, float* waveTable)
{
	// FIR Convolution routine using folded FIR structure
	float outputSample = 0.0f;

	// pos = position of sample N, N-1, N-2 etc
	int32_t revpos = (pos - firTaps + 1) & 0x7FF;		// position of sample 1, 2, 3 etc

	for (uint8_t i = 0; i < firTaps / 2; ++i) {
		// Folded FIR structure - as coefficients are symmetrical we can multiple the sample 1 + sample N by the 1st coefficient, sample 2 + sample N - 1 by 2nd coefficient etc
		outputSample += firCoeff[activeFilter][i] * (waveTable[revpos] + waveTable[pos]);
		revpos = (revpos + 1) & 0x7FF;
		pos = (pos - 1) & 0x7FF;
	}

	outputSample += firCoeff[activeFilter][firTaps / 2] * waveTable[revpos];

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
		float arg = windowBeta * sqrt(1.0f - pow( (static_cast<float>(2 * j) + 1 - firTaps) / (firTaps + 1), 2.0) );
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



