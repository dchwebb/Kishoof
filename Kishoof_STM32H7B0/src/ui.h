#pragma once

#include "initialisation.h"
#include "GpioPin.h"
#include <vector>
#include <string_view>

class UI {
public:
	void DrawUI();
	void Update();

//	uint32_t SerialiseConfig(uint8_t** buff);
//	uint32_t StoreConfig(uint8_t* buff);

//	struct Config {
//		DispMode displayMode = DispMode::Oscilloscope;
//	} config;


	std::string_view FloatToString(float f, bool smartFormat);
	std::string_view IntToString(const int32_t v);
	uint16_t DarkenColour(const uint16_t& colour, uint16_t amount);

	bool bufferClear = true;				// Used to manage blanking draw buffers using DMA


private:
	void DrawMenu();
	void DrawWaveTable();

	uint32_t oldWavetable = 0xFFFFFFFF;

	uint32_t btnDown = 0;
	uint32_t btnReleased = 0;

	char charBuff[100];
	bool activeDrawBuffer = true;

	GpioPin encoderBtn{GPIOE, 4, GpioPin::Type::InputPullup};		// PE4: Encoder button
};

extern UI ui;
