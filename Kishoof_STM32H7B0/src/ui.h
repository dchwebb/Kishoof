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

	char charBuff[100];
	bool activeDrawBuffer = true;

	// Struct to manage buttons with debounce and GPIO management
	struct Btn {
		GpioPin pin;
		uint32_t down = 0;
		uint32_t up = 0;

		// Debounce button press mechanism
		bool Pressed() {
			if (pin.IsHigh() && down && SysTickVal > down + 100) {
				down = 0;
				up = SysTickVal;
			}

			if (pin.IsLow() && down == 0 && SysTickVal > up + 100) {
				down = SysTickVal;
				return true;
			}
			return false;
		}
	};
	struct {
		Btn encoder;
		Btn octave;
	} buttons = {{{GPIOE, 4, GpioPin::Type::InputPullup}, 0, 0}, {{GPIOD, 10, GpioPin::Type::InputPullup}, 0, 0}};


};

extern UI ui;
