#pragma once

#include "initialisation.h"
#include "GpioPin.h"
#include "configManager.h"
#include <vector>
#include <string_view>

class UI {
public:
	void Update();
	void SetWavetable(int32_t index);

	std::string_view FloatToString(float f, bool smartFormat);
	std::string_view IntToString(const int32_t v);
	uint16_t DarkenColour(const uint16_t& colour, uint16_t amount);

	bool bufferClear = true;				// Used to manage blanking draw buffers using DMA
	static constexpr uint16_t waveDrawWidth = 200;
	static constexpr uint16_t waveDrawHeight = 120;

	enum class DisplayWave {channelA, channelB, Both};
	struct {
		DisplayWave displayWave = DisplayWave::Both;
	} cfg;

	ConfigSaver configSaver = {
		.settingsAddress = &cfg,
		.settingsSize = sizeof(cfg),
		.validateSettings = nullptr
	};

private:
	uint32_t WavetablePicker(int32_t upDown);
	void DrawWaveTable();


//	DisplayWave displayWave = DisplayWave::Both;

	uint32_t oldWavetable = 0xFFFFFFFF;
	uint32_t activeWaveTable = 0;
	uint32_t pickerOpened = 0;
	bool pickerDir = false;			// True if picker is currently selecting a directory

	bool oldWarpBtn = false;

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
		Btn warp;
	} buttons = {{{GPIOE, 4, GpioPin::Type::InputPullup}, 0, 0},
				 {{GPIOD, 10, GpioPin::Type::InputPullup}, 0, 0},
				 {{GPIOE, 3, GpioPin::Type::InputPullup}, 0, 0}};


};

extern UI ui;
