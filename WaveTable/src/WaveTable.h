#pragma once

#include "initialisation.h"
#include "SerialHandler.h"
#include "Filter.h"


struct WaveTable {
	friend class SerialHandler;				// Allow the serial handler access to private data for debug printing
	friend class Config;					// Allow the config access to private data to save settings
public:
	void CalcSample();						// Called by interrupt handler to generate next sample
	void Init();							// Initialise caches, buffers etc

	int32_t wavetable[2048];

private:
	uint32_t readPos = 0;
	int32_t oldReadPos;

	// Private class functions
	int32_t OutputMix(float wetSample);
	float FastTanh(float x);
};

extern WaveTable wavetable;
