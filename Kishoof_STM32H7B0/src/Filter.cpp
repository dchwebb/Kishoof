#include "Filter.h"


Filter filter;

void Filter::Init()
{
	FIRFilterWindow();
	BuildLUT();
}


void Filter::BuildLUT()
{
	// Build a lookup table of coefficients

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



