#pragma once

#include "initialisation.h"
#include "GpioPin.h"
#include "configManager.h"
#include "lcd.h"
#include <vector>
#include <string_view>


class UI {
public:
	void Update();
	void SetWavetable(int32_t index);

	std::string_view FloatToString(float f, bool smartFormat);
	std::string_view IntToString(const int32_t v);
	RGBColour DarkenColour(const RGBColour colour, uint16_t amount);
	RGBColour InterpolateColour(const RGBColour colour1, const RGBColour colour2, const float ratio);

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

	uint32_t oldWavetable = 0xFFFFFFFF;
	uint32_t activeWaveTable = 0;
	uint32_t fileinfoStart = 0;		// Timer to allow file info to be temporarily displayed on opening new wavetable
	bool fileinfo = false;			// Set to true to tell draw routine show/clear file info
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
