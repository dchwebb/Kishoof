#include "WaveTable.h"
#include "Filter.h"
#include "GpioPin.h"
#include <cmath>

WaveTable wavetable;


void WaveTable::CalcSample()
{

	GpioPin::SetHigh(GPIOC, 10);

	float newInc = static_cast<float>(adc.FeedbackPot) / 65536.0f;		// Convert 16 bit int to float 0 -> 1000
	pitchInc = 0.99 * pitchInc + 0.01 * newInc;
	float pitch = std::max(pitchInc * 20.0f, 1.0f);

	readPos += pitch;
	if (readPos >= 2048) { readPos -= 2048; }
	uint32_t readPos2 = (uint32_t)readPos + 1;
	if (readPos2 >= 2048) { readPos2 -= 2048; }

	float sample1 = wavetable[(uint32_t)readPos];
//	float sample2 = wavetable[readPos2];
//	float outputSample = std::lerp(sample1, sample2, readPos - (uint32_t)readPos);

	float filtered1 = filter.CalcFilter((uint32_t)readPos, wavetable);
	float filtered2 = filter.CalcFilter((uint32_t)(readPos + 1) & 0x7FF, wavetable);
	float outputSample = std::lerp(filtered1, filtered2, readPos - (uint32_t)readPos);


	SPI2->TXDR = (int32_t)(outputSample * floatToIntMult);
	SPI2->TXDR = sample1 * floatToIntMult;//outputSample;

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





