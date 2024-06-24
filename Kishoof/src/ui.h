#pragma once

#include "initialisation.h"
#include "configManager.h"
#include "lcd.h"
#include <vector>
#include <string_view>


class UI {
public:
	void Update();
	void SetWavetable(const int32_t index);

	std::string_view FloatToString(const float f, const bool smartFormat);
	std::string_view IntToString(const int32_t v);
	RGBColour DarkenColour(const RGBColour colour, const uint16_t amount);
	RGBColour InterpolateColour(const RGBColour colour1, const RGBColour colour2, const float ratio);

	bool bufferClear = true;				// Used to manage blanking draw buffers using DMA
	static constexpr uint16_t waveDrawWidth = 200;
	static constexpr uint16_t waveDrawHeight = 120;

	enum class DisplayWave : uint8_t {channelA, channelB, Both};
	enum class DisplayPos : uint8_t {off, line, pointer};
	struct {
		DisplayWave displayWave = DisplayWave::Both;
		DisplayPos displayPos = DisplayPos::line;
	} cfg;



	ConfigSaver configSaver = {
		.settingsAddress = &cfg,
		.settingsSize = sizeof(cfg),
		.validateSettings = nullptr
	};

private:
	uint32_t WavetablePicker(const int32_t upDown);
	void DrawWaveTable();
	void DrawPositionMarker(uint32_t yPos, bool dirUp, float xPos, RGBColour drawColour);

	// Variables to handle info display changes
	uint32_t oldWavetable = 0xFFFFFFFF;
	uint32_t activeWaveTable = 0;
	bool pickerDir = false;					// True if picker is currently selecting a directory

	uint32_t fileinfoStart = 0;				// Timer to allow file info to be temporarily displayed on opening new wavetable
	enum class TimedInfo {none, showFileInfo, clearFileInfo};
	TimedInfo timedInfo = TimedInfo::none;	// Temporary info about files

	uint32_t oldWarpType = 0xFFFFFFFF;
	bool oldWarpBtn = false;				// Shows if channel B is reversed

	char charBuff[100];
	bool activeDrawBuffer = true;

	// text drawing positions for upper/wide (wavetable name) and lower/narrow (warp type/file info) text
	static constexpr uint32_t narrowTextLeft = 80;
	static constexpr uint32_t narrowTextWidth = LCD::width - (2 * narrowTextLeft);			// Allows for 7 chars wide
	static constexpr uint32_t lowerTextTop = 192;
	static constexpr uint32_t wideTextLeft = 54;
	static constexpr uint32_t wideTextWidth = LCD::width - (2 * wideTextLeft);			// Allows for 12 chars wide
	static constexpr uint32_t uppertextTop = 35;


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
