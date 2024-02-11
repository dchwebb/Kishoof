#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include <cmath>

WaveTable wavetable;


volatile int susp = 0;
volatile float dbg[5000];
int idx = 0;
void WaveTable::CalcSample()
{
	GpioPin::SetHigh(GPIOC, 10);


	// 0v = 61200; 1v = 50110; 2v = 39020; 3v = 27910; 4v = 16790; 5v = 5670
	// C: 16.35 Hz 32.70 Hz; 65.41 Hz; 130.81 Hz; 261.63 Hz; 523.25 Hz; 1046.50 Hz; 2093.00 Hz; 4186.01 Hz
	// 61200 > 65.41 Hz; 50110 > 130.81 Hz; 39020 > 261.63 Hz

	// Increment = Hz * (2048 / 48000) = Hz * (wavetablesize / samplerate)
	// Pitch calculations - Increase pitchBase to increase pitch; Reduce ABS(cvMult) to increase spread

	// Calculations: 11090 is difference in cv between two octaves; 50110 is cv at 1v and 130.81 is desired Hz at 1v
	constexpr float pitchBase = (130.81f * (2048.0f / sampleRate)) / std::pow(2.0, -50120.0f / 11090.0f);
	constexpr float cvMult = -1.0f / 11090.0f;
	float newInc = pitchBase * std::pow(2.0f, (float)adc.FilterCV * cvMult);			// for cycle length matching sample rate (48k)
	pitchInc = 0.99 * pitchInc + 0.01 * newInc;

//	float newInc = static_cast<float>(adc.FeedbackPot) / 65536.0f;		// Convert 16 bit int to float 0 -> 1000
//	pitchInc = 0.99 * pitchInc + 0.01 * newInc;
//	float pitch = std::max(pitchInc * 20.0f, 1.0f);

	readPos += pitchInc;
	if (readPos >= 2048) { readPos -= 2048; }

	float outputSample1;
	float ratio = readPos - (uint32_t)readPos;
	if (ratio > 0.00001f) {
		outputSample1 = filter.CalcInterpolatedFilter((uint32_t)readPos, wavetable, ratio);
	} else {
		outputSample1 = filter.CalcFilter((uint32_t)readPos, wavetable);
	}

//	float filtered1 = filter.CalcFilter((uint32_t)readPos, wavetable);
//	float filtered2 = filter.CalcFilter((uint32_t)(readPos + 1) & 0x7FF, wavetable);
//	float outputSample2 = std::lerp(filtered1, filtered2, readPos - (uint32_t)readPos);

	if (std::isnan(outputSample1)) {
		++susp;
	}

	SPI2->TXDR = (int32_t)(outputSample1 * floatToIntMult);
	SPI2->TXDR = (int32_t)(outputSample1 * floatToIntMult);;

	GpioPin::SetLow(GPIOC, 10);			// Debug off
}



int32_t WaveTable::OutputMix(float wetSample)
{
	// Output mix level
	float dryLevel = static_cast<float>(adc.Mix) / 65536.0f;		// Convert 16 bit int to float 0 -> 1

	DAC1->DHR12R2 = (1.0f - dryLevel) * 4095.0f;		// Wet level
	DAC1->DHR12R1 = dryLevel * 4095.0f;					// Dry level

	int16_t outputSample = std::clamp(static_cast<int32_t>(wetSample), -32767L, 32767L);

	return outputSample;
}



void WaveTable::Init()
{
	for (uint32_t i = 0; i < 2048; ++i) {

		switch (wavetableType) {
		case TestData::noise:
			while ((RNG->SR & RNG_SR_DRDY) == 0) {};
			wavetable[i] = intToFloatMult * static_cast<int32_t>(RNG->DR);
			break;
		case TestData::twintone:
			wavetable[i] = 0.5f * (std::sin((float)i * M_PI * 2.0f / 2048.0f) +
					std::sin(430.0f * (float)i * M_PI * 2.0f / 2048.0f));
			break;
		}
	}

	DAC1->DHR12R2 = 4095;				// Wet level
	DAC1->DHR12R1 = 0;					// Dry level
}


// Algorithm source: https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
float WaveTable::FastTanh(float x)
{
	float x2 = x * x;
	float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
	float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
	return a / b;
}





