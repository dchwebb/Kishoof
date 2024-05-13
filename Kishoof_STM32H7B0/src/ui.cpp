#include "ui.h"
#include "lcd.h"
//#include "config.h"
#include "WaveTable.h"
#include  <cstdio>

UI ui;
uint32_t blankData;			// Used to transfer zeros into frame buffer by MDMA

void UI::DrawUI()
{


}



void UI::DrawWaveTable()
{
	// Populate a frame buffer to display the wavetable values (full screen refresh)
	//debugDraw.SetHigh();			// Debug

	if (wavetable.warpType != wavetable.oldWarpType) {
		wavetable.oldWarpType = wavetable.warpType;
		std::string_view s = wavetable.warpNames[(uint8_t)wavetable.warpType];
		//lcd.DrawString(60, 180, s, &lcd.Font_Small, LCD_WHITE, LCD_BLACK);

		const uint32_t textLeft = 75;
		const uint32_t textTop = 190;
		lcd.DrawStringMem(0, 0, lcd.Font_Large.Width * s.length(), lcd.drawBuffer[activeDrawBuffer], s, &lcd.Font_Large, LCD_WHITE, LCD_BLACK);
		lcd.PatternFill(textLeft, textTop, textLeft - 1 + lcd.Font_Large.Width * s.length(), textTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else if (wavetable.activeWaveTable != oldWavetable) {
		oldWavetable = wavetable.activeWaveTable;
		std::string_view s = std::string_view(wavetable.wavList[wavetable.activeWaveTable].name, 8);

		const uint32_t textLeft = 75;
		const uint32_t textTop = 40;
		lcd.DrawStringMem(0, 0, lcd.Font_Large.Width * s.length(), lcd.drawBuffer[activeDrawBuffer], s, &lcd.Font_Large, LCD_WHITE, LCD_BLACK);
		lcd.PatternFill(textLeft, textTop, textLeft - 1 + lcd.Font_Large.Width * s.length(), textTop - 1 + lcd.Font_Large.Height, lcd.drawBuffer[activeDrawBuffer]);

	} else {
		uint8_t oldHeight = wavetable.drawData[0];
		for (uint8_t i = 0; i < 240; ++i) {

			// do while loop needed to draw vertical lines where adjacent samples are vertically spaced by more than a pixel
			uint8_t currHeight = wavetable.drawData[i];
			do {
				const uint32_t pos = currHeight * LCD::height + i;
				lcd.drawBuffer[activeDrawBuffer][pos] = LCD_GREEN;
				currHeight += currHeight > oldHeight ? -1 : 1;
			} while (currHeight != oldHeight);

			oldHeight = wavetable.drawData[i];
		}

		lcd.PatternFill(0, 60, LCD::width - 1, 179, lcd.drawBuffer[activeDrawBuffer]);
	}
	activeDrawBuffer = !activeDrawBuffer;

	// Trigger MDMA frame buffer blanking
	blankData = 0;
	MDMATransfer(MDMA_Channel0, (const uint8_t*)&blankData, (const uint8_t*)lcd.drawBuffer[activeDrawBuffer], sizeof(lcd.drawBuffer[0]) / 2);
	bufferClear = false;


	//debugDraw.SetLow();			// Debug off

}

void UI::DrawMenu()
{
//	lcd.DrawString(10, 6, "L", &lcd.Font_Large, LCD_WHITE, LCD_BLACK);
//	lcd.DrawString(80, 6, "Encoder Action", &lcd.Font_Large, LCD_ORANGE, LCD_BLACK);
//	lcd.DrawString(303, 6, "R", &lcd.Font_Large, LCD_WHITE, LCD_BLACK);
//	lcd.DrawRect(0, 1, 319, 239, LCD_WHITE);
//	lcd.DrawLine(0, 27, 319, 27, LCD_WHITE);
//	lcd.DrawLine(26, 1, 26, 27, LCD_WHITE);
//	lcd.DrawLine(294, 1, 294, 27, LCD_WHITE);
//	lcd.DrawLine(159, 27, 159, 239, LCD_WHITE);
//
//	const std::vector<MenuItem>* currentMenu =
//			config.displayMode == DispMode::Oscilloscope ? &oscMenu :
//			config.displayMode == DispMode::Tuner ? &tunerMenu :
//			config.displayMode == DispMode::Fourier || config.displayMode == DispMode::Waterfall ? &fftMenu : nullptr;
//
//	uint8_t pos = 0;
//	for (auto m = currentMenu->cbegin(); m != currentMenu->cend(); m++, pos++) {
//		lcd.DrawString(10, 32 + pos * 20, m->name, &lcd.Font_Large, (m->selected == encoderModeL) ? LCD_BLACK : LCD_WHITE, (m->selected == encoderModeL) ? LCD_WHITE : LCD_BLACK);
//		lcd.DrawString(170, 32 + pos * 20, m->name, &lcd.Font_Large, (m->selected == encoderModeR) ? LCD_BLACK : LCD_WHITE, (m->selected == encoderModeR) ? LCD_WHITE : LCD_BLACK);
//	}
}


void UI::Update()
{
	// encoders count in fours with the zero point set to 32000
	if (std::abs((int16_t)32000 - (int16_t)TIM8->CNT) > 3) {
		int8_t v = TIM8->CNT > 32000 ? 1 : -1;
		wavetable.ChangeWaveTable(v);

		TIM8->CNT -= TIM8->CNT > 32000 ? 4 : -4;
		//cfg.ScheduleSave();
	}



	// Check if encoder button is pressed with debounce
	if (btnDown && SysTickVal > btnDown + 100 && encoderBtn.IsHigh()) {
		btnDown = 0;
	}

	if (encoderBtn.IsLow() && btnDown == 0) {
		btnDown = SysTickVal;

		// Button action
	}

	if (!(SPI_DMA_Working)) {
		//StartDebugTimer();
		DrawWaveTable();
		//wavetableCalc = StopDebugTimer();
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
