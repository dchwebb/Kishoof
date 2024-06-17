#include "ui.h"
#include "lcd.h"
//#include "config.h"
#include "WaveTable.h"
#include  <cstdio>
#include <cstring>

UI ui;
uint32_t blankData;			// Used to transfer zeros into frame buffer by MDMA


void UI::DrawWaveTable()
{
	// Populate a frame buffer to display the wavetable values (full screen refresh)
	debugPin2.SetHigh();			// Debug

	if (wavetable.warpType != wavetable.oldWarpType) {
		wavetable.oldWarpType = wavetable.warpType;
		std::string_view s = wavetable.warpNames[(uint8_t)wavetable.warpType];

		constexpr uint32_t textLeft = 80;
		constexpr uint32_t textWidth = LCD::width - (2 * textLeft);			// Allows for 12 chars wide
		constexpr uint32_t textTop = 192;
		lcd.DrawStringMemCenter(0, 0, textWidth, lcd.drawBuffer[activeDrawBuffer], s, &lcd.Font_Large, wavetable.warpType == WaveTable::Warp::none ? LCD_GREY : LCD_WHITE, LCD_BLACK);
		lcd.PatternFill(textLeft, textTop, textLeft - 1 + textWidth, textTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if (activeWaveTable != oldWavetable) {
		oldWavetable = activeWaveTable;
		std::string_view s;
		if (wavetable.wavList[activeWaveTable].lfn[0]) {
			s = std::string_view(wavetable.wavList[activeWaveTable].lfn);
		} else {
			uint32_t space = (std::string_view(wavetable.wavList[activeWaveTable].name)).find(" ");
			s = std::string_view(wavetable.wavList[activeWaveTable].name, std::min(8UL, space));		// Replace spaces with 0 for length finding
		}

		constexpr uint32_t textLeft = 54;
		constexpr uint32_t textWidth = LCD::width - (2 * textLeft);			// Allows for 12 chars wide
		constexpr uint32_t textTop = 35;
		const uint16_t colour = pickerDir ? LCD_YELLOW : wavetable.wavList[activeWaveTable].valid ? LCD_WHITE : LCD_GREY;
		lcd.DrawStringMemCenter(0, 0, textWidth, lcd.drawBuffer[activeDrawBuffer], s, &lcd.Font_Large, colour, LCD_BLACK);
		lcd.PatternFill(textLeft, textTop, textLeft - 1 + textWidth, textTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if (wavetable.cfg.warpButton != oldWarpBtn) {
		oldWarpBtn = wavetable.cfg.warpButton;
		lcd.DrawChar(55, 192, oldWarpBtn ? '<' : ' ', &lcd.Font_Large, LCD_ORANGE, LCD_BLACK);
		lcd.DrawChar(172, 192, oldWarpBtn ? '<' : ' ', &lcd.Font_Large, LCD_ORANGE, LCD_BLACK);

	} else {
		if (cfg.displayWave == DisplayWave::Both) {

			for (uint32_t channel = 0; channel < 2; ++channel) {
				uint8_t oldHeight = (channel ? 60 : 0) + wavetable.drawData[channel][0] / 2;
				for (uint8_t i = 0; i < waveDrawWidth; ++i) {

					// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
					uint8_t currHeight = (channel ? 60 : 0) + wavetable.drawData[channel][i] / 2;
					do {
						const uint32_t pos = currHeight * waveDrawWidth + i;		// Pixel order is across then down
						lcd.drawBuffer[activeDrawBuffer][pos] = channel ? LCD_ORANGE : LCD_LIGHTBLUE;
						currHeight += currHeight > oldHeight ? -1 : 1;
					} while (currHeight != oldHeight);

					oldHeight = (channel ? 60 : 0) + wavetable.drawData[channel][i] / 2;
				}
			}

		} else {
			const uint32_t channel = (cfg.displayWave == DisplayWave::channelA ? 0 : 1);
			const uint32_t wavetablePos = wavetable.CurrentWavetable(channel);
			const uint16_t drawColour = (cfg.displayWave == DisplayWave::channelA) ? LCD_LIGHTBLUE : LCD_ORANGE;

			// Draw a line representing the quantised wavetable position
			const wchar_t colour = (cfg.displayWave == DisplayWave::channelA) ? (LCD_DULLBLUE << 16) + LCD_DULLBLUE : (LCD_DULLORANGE << 16) + LCD_DULLORANGE;
			wmemset((wchar_t*)lcd.drawBuffer[activeDrawBuffer], colour, wavetablePos / 2);

			uint8_t oldHeight = wavetable.drawData[channel][0];
			for (uint8_t i = 0; i < waveDrawWidth; ++i) {
				// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
				uint8_t currHeight = wavetable.drawData[channel][i];
				do {
					const uint32_t pos = currHeight * waveDrawWidth + i;		// Pixel order is across then down
					lcd.drawBuffer[activeDrawBuffer][pos] = drawColour;
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

	// Trigger MDMA frame buffer blanking
	blankData = 0;
	MDMATransfer(MDMA_Channel0, (const uint8_t*)&blankData, (const uint8_t*)lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 2);
	bufferClear = false;


	debugPin2.SetLow();			// Debug off

}


void UI::SetWavetable(int32_t index)
{
	// Allows wavetable class to set current wavetable at Init
	activeWaveTable = index;
	pickerDir = wavetable.wavList[index].isDir;

}


uint32_t UI::WavetablePicker(int32_t upDown)
{
	// Selects next/previous wavetable, unless directory in which case it can be opened
	const int32_t nextWavetable = (int32_t)activeWaveTable + upDown;
	auto& currentWT = wavetable.wavList[activeWaveTable];
	auto& nextWT = wavetable.wavList[nextWavetable];

	if (nextWavetable >= 0 && nextWavetable < (int32_t)wavetable.wavetableCount && nextWT.dir == currentWT.dir) {
		pickerOpened = SysTickVal;			// Store time opened to check if drawing picker
		activeWaveTable = nextWavetable;
		pickerDir = nextWT.isDir;
		if (!pickerDir && nextWT.valid) {
			wavetable.ChangeWaveTable(nextWavetable);
		}
	}
	return 0;
}


void UI::Update()
{
	// encoders count in fours with the zero point set to 32000; test for values greater than 3 as sometimes miscounts
	if (std::abs((int16_t)32000 - (int16_t)TIM8->CNT) > 2) {
		int8_t v = TIM8->CNT > 32000 ? 1 : -1;
		WavetablePicker(v);

		TIM8->CNT -= TIM8->CNT > 32000 ? 4 : -4;
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

	if (!(SPI_DMA_Working)) {
		DrawWaveTable();
	}
}


std::string_view UI::FloatToString(float f, bool smartFormat)
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


//	Takes an RGB colour and darkens by the specified amount
uint16_t UI::DarkenColour(const uint16_t& colour, uint16_t amount) {
	uint16_t r = (colour >> 11) << 1;
	uint16_t g = (colour >> 5) & 0b111111;
	uint16_t b = (colour & 0b11111) << 1;
	r -= std::min(amount, r);
	g -= std::min(amount, g);
	b -= std::min(amount, b);;
	return ((r >> 1) << 11) + (g << 5) + (b >> 1);
}

/*
uint32_t UI::SerialiseConfig(uint8_t** buff)
{
	*buff = reinterpret_cast<uint8_t*>(&config);
	return sizeof(config);
}


uint32_t UI::StoreConfig(uint8_t* buff)
{
	if (buff != nullptr) {
		memcpy(&config, buff, sizeof(config));
	}
	return sizeof(config);
}
*/
