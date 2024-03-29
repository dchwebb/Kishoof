#include "Snare.h"
#include "VoiceManager.h"

void Snare::Play(const uint8_t voice, const uint32_t noteOffset, uint32_t noteRange, const float velocity)
{
	playing = true;

	partialpos[0] = config.basePos;						// Create discontinuity to create initial click
	partialpos[1] = 0.0f;
	partialpos[2] = 0.0f;

	const float freq = (config.baseFreq * (static_cast<float>(ADC_array[ADC_SnareTuning]) / 65536.0f + 0.5f));

	for (uint8_t i = 0; i < partialCount; ++i) {
		partialLevel[i] = config.partialInitLevel[i];
		partialInc[i] = FreqToInc(freq * config.partialFreqOffset[i]);
	}
	noiseLevel = config.noiseInitLevel;
	velocityScale = velocity * (static_cast<float>(ADC_array[ADC_SnareLevel]) / 32768.0f);

	noteRange = noteRange == 0 ? 128 : noteRange;
	sustainRate = 0.0012f * sqrt((static_cast<float>(noteOffset) + 1.0f) / noteRange);		// note offset allows for longer sustained hits
}


void Snare::Play(const uint8_t voice, const uint32_t index)
{
	// Called when button is pressed
	Play(0, index, 0, 1.0f);
}


void Snare::CalcOutput()
{
	if (playing) {
		const float rand1 = intToFloatMult * static_cast<int32_t>(RNG->DR);		// Left channel random number used for noise

		noiseLevel *= config.noiseDecay + sustainRate;
		float maxLevel = noiseLevel;

		float partialOutput = 0.0f;
		for (uint8_t i = 0; i < partialCount; ++i) {
			partialpos[i] += partialInc[i];							// Set current poition in sine wave
			partialLevel[i] *= config.partialDecay + sustainRate;
			partialOutput += std::sin(partialpos[i]) * partialLevel[i];

			if (partialLevel[i] > maxLevel) {
				maxLevel = partialLevel[i];
			}
		}

		const float rand2 = intToFloatMult * static_cast<int32_t>(RNG->DR);		// Get right channel noise value: calc here to give time for peripheral to update

		outputLevel[left]  = velocityScale * filter.CalcFilter(partialOutput + (rand1 * noiseLevel), left);
		outputLevel[right] = velocityScale * filter.CalcFilter(partialOutput + (rand2 * noiseLevel), right);

		if (maxLevel < 0.00001f) {
			playing = false;
		}

		noteMapper->pwmLed.Level(maxLevel);
	}

}


void Snare::UpdateFilter()
{
	filter.Update(false);
}


uint32_t Snare::SerialiseConfig(uint8_t** buff, const uint8_t voiceIndex)
{
	*buff = reinterpret_cast<uint8_t*>(&config);
	return sizeof(config);
}


void Snare::StoreConfig(uint8_t* buff, const uint32_t len)
{
	if (buff != nullptr && len <= sizeof(config)) {
		memcpy(&config, buff, len);
	}
}


uint32_t Snare::ConfigSize()
{
	return sizeof(config);
}
