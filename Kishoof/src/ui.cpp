#include "ui.h"
#include "WaveTable.h"
#include <cstdio>
#include <cstring>

UI ui;
uint32_t blankData;			// Used to transfer zeros into frame buffer by MDMA


void UI::DrawWaveTable()
{
	if (fatTools.Busy()) {
		return;
	}

	// Populate a frame buffer to display either text portion of screen or wavetables

	if (timedInfo == TimedInfo::showFileInfo) {		// Show frame count or error type when trying to load a wavetable
		timedInfo = TimedInfo::none;

		const char* buff;
		if (wavetable.wavList[activeWaveTable].invalid) {
			buff = wavetable.InvalidText[wavetable.wavList[activeWaveTable].invalid];
		} else {
			snprintf(charBuff, 13, "Frames %d", wavetable.wavList[activeWaveTable].tableCount);
			buff = charBuff;
		}

		lcd.DrawStringMemCenter(0, 0, wideTextWidth, lcd.drawBuffer[activeDrawBuffer], buff, lcd.Font_Large, RGBColour::LightGrey, RGBColour::Black);
		lcd.PatternFill(wideTextLeft, lowerTextTop, wideTextLeft - 1 + wideTextWidth, lowerTextTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if (timedInfo == TimedInfo::clearFileInfo) {			// Clear wavetable loading info after timeout
		oldWarpType = 0xFFFFFFFF;								// Force redraw of warp type
		oldWarpBtn = !oldWarpBtn;								// Force redraw of channel B reverse
		timedInfo = TimedInfo::none;

		// Will just blank the screen
		lcd.PatternFill(wideTextLeft, lowerTextTop, wideTextLeft - 1 + wideTextWidth, lowerTextTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if ((uint32_t)wavetable.warpType != oldWarpType) {
		oldWarpType = (uint32_t)wavetable.warpType;
		std::string_view s = wavetable.warpNames[(uint8_t)wavetable.warpType];

		lcd.DrawStringMemCenter(0, 0, narrowTextWidth, lcd.drawBuffer[activeDrawBuffer], s, lcd.Font_Large, wavetable.warpType == WaveTable::Warp::none ? RGBColour::Grey : RGBColour::White, RGBColour::Black);
		lcd.PatternFill(narrowTextLeft, lowerTextTop, narrowTextLeft - 1 + narrowTextWidth, lowerTextTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if (activeWaveTable != oldWavetable) {
		oldWavetable = activeWaveTable;

		std::string_view s;
		if (wavetable.wavList[activeWaveTable].lfn[0]) {
			s = std::string_view(wavetable.wavList[activeWaveTable].lfn);
		} else {
			uint32_t space = (std::string_view(wavetable.wavList[activeWaveTable].name)).find(" ");
			s = std::string_view(wavetable.wavList[activeWaveTable].name, std::min(8UL, space));		// Replace spaces with 0 for length finding
		}

		const uint16_t colour = pickerDir ? RGBColour::Yellow : wavetable.wavList[activeWaveTable].invalid ? RGBColour::LightGrey : RGBColour::White;
		lcd.DrawStringMemCenter(0, 0, wideTextWidth, lcd.drawBuffer[activeDrawBuffer], s, lcd.Font_Large, colour, RGBColour::Black);
		lcd.PatternFill(wideTextLeft, uppertextTop, wideTextLeft - 1 + wideTextWidth, uppertextTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);


	} else if (wavetable.cfg.warpButton != oldWarpBtn) {
		oldWarpBtn = wavetable.cfg.warpButton;
		lcd.DrawChar(55, 192, oldWarpBtn ? '<' : ' ', lcd.Font_Large, RGBColour::Orange, RGBColour::Black);
		lcd.DrawChar(172, 192, oldWarpBtn ? '<' : ' ', lcd.Font_Large, RGBColour::Orange, RGBColour::Black);

	} else {

		// Set up positions for drawing markers representing the quantised wavetable position and warp amount
		const float warpPos = (float)wavetable.warpAmt * (1.0f / 65535.0f);
		const uint32_t bottomLine = (waveDrawHeight - 1);		// Pixel order is across then down
		const uint32_t middleLine = ((waveDrawHeight / 2) - 1);
		DrawPositionMarker(bottomLine, true, warpPos, RGBColour::LightGrey);

		if (cfg.displayWave == DisplayWave::Both) {

			for (uint32_t channel = 0; channel < 2; ++channel) {
				const RGBColour drawColour = (channel == 0) ? RGBColour::LightBlue : RGBColour::Orange;
				DrawPositionMarker(channel == 0 ? 0 : middleLine, false, wavetable.QuantisedWavetablePos(channel), drawColour);

				uint8_t oldHeight = (channel ? 60 : 0) + wavetable.drawData[channel][0] / 2;
				for (uint8_t i = 0; i < waveDrawWidth; ++i) {

					// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
					uint8_t currHeight = (channel ? 60 : 0) + wavetable.drawData[channel][i] / 2;
					do {
						const uint32_t pos = currHeight * waveDrawWidth + i;		// Pixel order is across then down
						lcd.drawBuffer[activeDrawBuffer][pos] = channel ? RGBColour::Orange : RGBColour::LightBlue;
						currHeight += currHeight > oldHeight ? -1 : 1;
					} while (currHeight != oldHeight);

					oldHeight = (channel ? 60 : 0) + wavetable.drawData[channel][i] / 2;
				}
			}

		} else {
			const uint32_t channel = (cfg.displayWave == DisplayWave::channelA ? 0 : 1);
			const RGBColour drawColour = (channel == 0) ? RGBColour::LightBlue : RGBColour::Orange;

			DrawPositionMarker(0, false, wavetable.QuantisedWavetablePos(channel), drawColour);

			uint8_t oldHeight = wavetable.drawData[channel][0];
			for (uint8_t i = 0; i < waveDrawWidth; ++i) {
				// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
				uint8_t currHeight = wavetable.drawData[channel][i];
				do {
					const uint32_t pos = currHeight * waveDrawWidth + i;		// Pixel order is across then down
					lcd.drawBuffer[activeDrawBuffer][pos] = drawColour.colour;
					currHeight += currHeight > oldHeight ? -1 : 1;
				} while (currHeight != oldHeight);

				oldHeight = wavetable.drawData[channel][i];
			}
		}

		constexpr uint32_t drawL = (LCD::width - waveDrawWidth) / 2;
		constexpr uint32_t drawR = drawL + waveDrawWidth - 1;
		constexpr uint32_t drawT = (LCD::height - waveDrawHeight) / 2;
		constexpr uint32_t drawB = drawT + waveDrawHeight - 1;
		lcd.PatternFill(drawL, drawT, drawR, drawB, lcd.drawBuffer[activeDrawBuffer]);

	}
	activeDrawBuffer = !activeDrawBuffer;

	// Trigger MDMA frame buffer blanking (memset very slow and MDMA does not require processor)
	blankData = 0;
	MDMATransfer(MDMA_Channel0, (const uint8_t*)&blankData, (const uint8_t*)lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 2);
	bufferClear = false;
}


void UI::DrawPositionMarker(uint32_t yPos, bool dirUp, float xPos, RGBColour drawColour)
{
	// Display the wavetable position and warp amount markers as lines or triangles according to config
	xPos *= waveDrawWidth;
	yPos *= waveDrawWidth;
	switch (cfg.displayPos) {
	case DisplayPos::line:
		// Draw gradient lines representing the marker position
		for (uint8_t i = 0; i < xPos; ++i) {
			lcd.drawBuffer[activeDrawBuffer][yPos + i] = RGBColour::InterpolateColour(RGBColour::Black, drawColour, (float)i / xPos).colour;
		}
		break;
	case DisplayPos::pointer:
		for (int i = - 2; i < 3; ++i) {
			const uint32_t top = xPos + yPos + (dirUp ? -2 : 0) * waveDrawWidth;

			if (i + xPos >= 0 && i + xPos < waveDrawWidth) {
				lcd.drawBuffer[activeDrawBuffer][i + top + (dirUp ? (2 * waveDrawWidth) : 0)] = drawColour.colour;
				if (i > -2 && i < 2) {
					lcd.drawBuffer[activeDrawBuffer][i + top + waveDrawWidth] = drawColour.colour;
				}
				if (i == 0) {
					lcd.drawBuffer[activeDrawBuffer][i + top + (dirUp ? 0 : (2 * waveDrawWidth))] = drawColour.colour;
				}
			}
		}
		break;
	default:
		break;
	}
}


void UI::SetWavetable(const int32_t index)
{
	// Allows wavetable class to set current wavetable at Init
	oldWavetable = 0xFFFFFFFF;
	activeWaveTable = index;
	pickerDir = wavetable.wavList[index].isDir;

}


uint32_t UI::WavetablePicker(const int32_t upDown)
{
	// Selects next/previous wavetable, unless directory in which case it can be opened
	const int32_t nextWavetable = (int32_t)activeWaveTable + upDown;
	auto& currentWT = wavetable.wavList[activeWaveTable];
	auto& nextWT = wavetable.wavList[nextWavetable];

	if (nextWavetable >= 0 && nextWavetable < (int32_t)wavetable.wavetableCount && nextWT.dir == currentWT.dir) {
		activeWaveTable = nextWavetable;
		pickerDir = nextWT.isDir;
		if (!pickerDir && !nextWT.invalid) {
			wavetable.ChangeWaveTable(nextWavetable);
		}
	}
	return 0;
}


void UI::Update()
{
	// encoders count in fours with the zero point set to 32000; test for values greater than 3 as sometimes miscounts
	if (std::abs((int16_t)32000 - (int16_t)TIM8->CNT) > 2) {
		const int8_t v = TIM8->CNT > 32000 ? 1 : -1;
		WavetablePicker(v);

		TIM8->CNT -= TIM8->CNT > 32000 ? 4 : -4;

		// If selected wavetable has changed, show frame count etc
		if (!pickerDir) {
			fileinfoStart = SysTickVal;											// Store time opened to display file info
			timedInfo = TimedInfo::showFileInfo;
		}
	}

	if (buttons.encoder.Pressed()) {
		// If directory selected by picker, click opens folder
		if (pickerDir) {
			activeWaveTable = wavetable.wavList[activeWaveTable].firstWav;
			WavetablePicker(0);
		} else {
			switch (cfg.displayWave) {
			case DisplayWave::channelA:
				cfg.displayWave = DisplayWave::channelB;
				break;
			case DisplayWave::channelB:
				cfg.displayWave = DisplayWave::Both;
				break;
			case DisplayWave::Both:
				cfg.displayWave = DisplayWave::channelA;
				break;
			}
			config.ScheduleSave();
		}
	}

	if (buttons.octave.Pressed()) {
		wavetable.ChannelBOctave(true);
	}

	if (buttons.warp.Pressed()) {
		wavetable.WarpButton(true);
	}

	// Clear timed file info data after 3 seconds or if warp type is changed
	if (fileinfoStart > 0 &&
			(SysTickVal > fileinfoStart + 3000 ||
			(uint32_t)wavetable.warpType != oldWarpType ||
			wavetable.cfg.warpButton != oldWarpBtn ||
			pickerDir)) {
		timedInfo = TimedInfo::clearFileInfo;
		fileinfoStart = 0;
	}

	if (!(SPI_DMA_Working)) {
		DrawWaveTable();
	}
}


std::string_view UI::FloatToString(const float f, const bool smartFormat)
{
	if (smartFormat && f > 10000) {
		sprintf(charBuff, "%.1fk", std::round(f / 100.0f) / 10.0f);
	} else if (smartFormat && f > 1000) {
		sprintf(charBuff, "%.0f", std::round(f));
	} else	{
		sprintf(charBuff, "%.1f", std::round(f * 10.0f) / 10.0f);
	}
	return charBuff;
}


std::string_view UI::IntToString(const int32_t v)
{
	sprintf(charBuff, "%ld", v);
	return charBuff;
}


RGBColour UI::InterpolateColour(const RGBColour colour1, const RGBColour colour2, const float ratio)
{
	//	Interpolate between two RGB 565 colours
	const uint8_t r = std::lerp(colour1.red, colour2.red, ratio);
	const uint8_t g = std::lerp(colour1.green, colour2.green, ratio);
	const uint8_t b = std::lerp(colour1.blue, colour2.blue, ratio);
	return RGBColour{r, g, b};
}


RGBColour UI::DarkenColour(const RGBColour colour, const uint16_t amount)
{
	//	Darken an RGB colour by the specified amount (apply bit offset so that all colours are treated as 6 bit values)
	uint8_t r = (colour.red << 1) - std::min(amount, colour.red);
	uint8_t g = colour.green - std::min(amount, colour.green);
	uint8_t b = (colour.blue << 1) - std::min(amount, colour.blue);
	return RGBColour{uint8_t(r >> 1), g, uint8_t(b >> 1)};
}
