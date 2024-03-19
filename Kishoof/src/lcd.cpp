#include "lcd.h"

//LCD lcd;

LCD __attribute__((section (".ram_d1_data"))) lcd {};

void LCD::Init()
{
	Command(cmdGC9A01A::SWRESET);			// Software reset
	Delay(100000);

	Command(cmdGC9A01A::INREGEN2);
	CommandData(0xEB, cdArgs_t {0x14});
	Command(cmdGC9A01A::INREGEN1);
	Command(cmdGC9A01A::INREGEN2);
	CommandData(0xEB, cdArgs_t {0x14});
	CommandData(0x84, cdArgs_t {0x40});
	CommandData(0x85, cdArgs_t {0xFF});
	CommandData(0x86, cdArgs_t {0xFF});
	CommandData(0x87, cdArgs_t {0xFF});
	CommandData(0x88, cdArgs_t {0x0A});
	CommandData(0x89, cdArgs_t {0x21});
	CommandData(0x8A, cdArgs_t {0x00});
	CommandData(0x8B, cdArgs_t {0x80});
	CommandData(0x8C, cdArgs_t {0x01});
	CommandData(0x8D, cdArgs_t {0x01});
	CommandData(0x8E, cdArgs_t {0xFF});
	CommandData(0x8F, cdArgs_t {0xFF});
	CommandData(0xB6, cdArgs_t {0x00, 0x00});
	CommandData(cmdGC9A01A::MADCTL, cdArgs_t {MADCTL_MX | MADCTL_BGR});
	CommandData(cmdGC9A01A::COLMOD, cdArgs_t {0x05});
	CommandData(0x90, cdArgs_t {0x08, 0x08, 0x08, 0x08});
	CommandData(0xBD, cdArgs_t {0x06});
	CommandData(0xBC, cdArgs_t {0x00});
	CommandData(0xFF, cdArgs_t {0x60, 0x01, 0x04});
	CommandData(cmdGC9A01A::POWER2, cdArgs_t {0x13});
	CommandData(cmdGC9A01A::POWER3, cdArgs_t {0x13});
	CommandData(cmdGC9A01A::POWER4, cdArgs_t {0x22});
	CommandData(0xBE, cdArgs_t {0x11});
	CommandData(0xE1, cdArgs_t {0x10, 0x0E});
	CommandData(0xDF, cdArgs_t {0x21, 0x0c, 0x02});
	CommandData(cmdGC9A01A::GAMMA1, cdArgs_t {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A});
	CommandData(cmdGC9A01A::GAMMA2, cdArgs_t {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F});
	CommandData(cmdGC9A01A::GAMMA3, cdArgs_t {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A});
	CommandData(cmdGC9A01A::GAMMA4, cdArgs_t {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F});
	CommandData(0xED, cdArgs_t {0x1B, 0x0B});
	CommandData(0xAE, cdArgs_t {0x77});
	CommandData(0xCD, cdArgs_t {0x63});
	// Unsure what this (from manufacturer's boilerplate code) is meant to do, seems to work OK without:
	//CommandData(0x70, cdArgs_t {00x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x030});
	CommandData(cmdGC9A01A::FRAMERATE, cdArgs_t {0x34});
	CommandData(0x62, cdArgs_t {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70});
	CommandData(0x63, cdArgs_t {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70});
	CommandData(0x64, cdArgs_t {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07});
	CommandData(0x66, cdArgs_t {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00});
	CommandData(0x67, cdArgs_t {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98});
	CommandData(0x74, cdArgs_t {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00});
	CommandData(0x98, cdArgs_t {0x3e, 0x07});
	Command(cmdGC9A01A::TEON);
	Command(cmdGC9A01A::INVON);
	Command(cmdGC9A01A::SLPOUT);		// Exit sleep
	Delay(10000);
	Command(cmdGC9A01A::DISPON);		// Display on
	Delay(10000);

	ScreenFill(LCD_BLACK);

	Rotate(LCD_Portrait_Flipped);
/*
	ColourFill(50, 50, 57, 57, LCD_YELLOW);
	ColourFill(90, 90, 97, 97, LCD_RED);
	ColourFill(130, 130, 137, 137, LCD_BLUE);
	ColourFill(170, 170, 177, 177, LCD_GREEN);

*/
};


void LCD::SetCursorPosition(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2)
{
	Command(cmdGC9A01A::CASET);
	Data16b(x1);
	Data16b(x2);

	Command(cmdGC9A01A::RASET);
	Data16b(y1);
	Data16b(y2);

	Command(cmdGC9A01A::RAMWR);			// Write to RAM
}


void LCD::Rotate(const LCD_Orientation_t o)
{
	Command(cmdGC9A01A::MADCTL);
	switch (o) {
		case LCD_Portrait :				Data(MADCTL_MX | MADCTL_BGR); break;
		case LCD_Portrait_Flipped : 	Data(MADCTL_MV | MADCTL_BGR); break;
		case LCD_Landscape : 			Data(MADCTL_MY | MADCTL_BGR); break;
		case LCD_Landscape_Flipped :	Data(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR); break;
	}

	orientation = o;
}


void LCD::ScreenFill(const uint16_t colour)
{
	ColourFill(0, 0, width - 1, height - 1, colour);
}


void LCD::DMASend(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t* pixelData, bool memInc)
{
	const uint32_t pixelCount = (x1 - x0 + 1) * (y1 - y0 + 1);
	SetCursorPosition(x0, y0, x1, y1);

	while (SPI_DMA_Working);

	DCPin.SetHigh();

	SPI3->CR1 &= ~SPI_CR1_SPE;						// Disable SPI
	SPI3->IFCR |= SPI_IFCR_TXTFC;					// Clear Transmission Transfer Filled flag

	DMA1_Stream0->CR &= ~DMA_SxCR_EN;				// Disable DMA
	DMA1->LIFCR |= DMA_LIFCR_CFEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0;	// Clear DMA errors and transfer complete status flags

	if (memInc) {
		DMA1_Stream0->CR |= DMA_SxCR_MINC;			// Memory in increment mode
	} else {
		DMA1_Stream0->CR &= ~DMA_SxCR_MINC;			// Memory not in increment mode
	}
	DMA1_Stream0->M0AR = (uint32_t)pixelData;		// Configure the memory data register address
	DMA1_Stream0->NDTR = pixelCount;				// Number of data items to transfer
	DMA1_Stream0->CR |= DMA_SxCR_EN;				// Enable DMA and wait

	SPI3->CFG1 |= 15;								// Set SPI to 16 bit mode
	SPI3->CFG1 |= SPI_CFG1_TXDMAEN;					// Tx DMA stream enable
	SPI3->CR1 |= SPI_CR1_SPE;						// Enable SPI
	SPI3->CR1 |= SPI_CR1_CSTART;					// Start SPI
}



void LCD::ColourFill(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t colour)
{
	while (SPI_DMA_Working);			// Workaround to prevent compiler optimisations altering dmaInt16 value during send
	dmaInt16 = colour;
	DMASend(x0, y0, x1, y1, &dmaInt16, false);
/*
	const uint32_t pixelCount = (x1 - x0 + 1) * (y1 - y0 + 1);

	SetCursorPosition(x0, y0, x1, y1);
	dmaInt16 = colour;

	while (SPI_DMA_Working);

	DCPin.SetHigh();

	SPI3->CR1 &= ~SPI_CR1_SPE;						// Disable SPI
	SPI3->IFCR |= SPI_IFCR_TXTFC;

	DMA1_Stream0->CR &= ~DMA_SxCR_EN;				// Disable DMA
	DMA1->LIFCR |= DMA_LIFCR_CFEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0;

	DMA1_Stream0->CR &= ~DMA_SxCR_MINC;				// Memory not in increment mode
	DMA1_Stream0->M0AR = (uint32_t)&dmaInt16;		// Configure the memory data register address

	DMA1_Stream0->NDTR = pixelCount;				// Number of data items to transfer
	DMA1_Stream0->CR |= DMA_SxCR_EN;				// Enable DMA and wait

	SPI3->CFG1 |= 15;								// Set SPI to 16 bit mode
	SPI3->CFG1 |= SPI_CFG1_TXDMAEN;					// Tx DMA stream enable
	SPI3->CR1 |= SPI_CR1_SPE;						// Enable SPI
	SPI3->CR1 |= SPI_CR1_CSTART;					// Start SPI
*/
}


void LCD::PatternFill(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t* pixelData)
{
	DMASend(x0, y0, x1, y1, pixelData, true);
}


void LCD::DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour)
{
	SetCursorPosition(x, y, x, y);
	Data16b(colour);
}


void LCD::DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour)
{
	// Check lines are not too long
	if (x0 >= width)	x0 = width - 1;
	if (x1 >= width)	x1 = width - 1;
	if (y0 >= height)	y0 = height - 1;
	if (y1 >= height)	y1 = height - 1;
	if (y0 < 0)			y0 = 0;
	if (y1 < 0)			y1 = 0;

	// Flip co-ordinates if wrong way round
	if (x0 > x1) 		std::swap(x0, x1);
	if (y0 > y1) 		std::swap(y0, y1);

	int16_t dx = x1 - x0;
	int16_t dy = y1 - y0;

	// Vertical or horizontal line
	if (dx == 0 || dy == 0) {
		ColourFill(x0, y0, x1, y1, colour);
		return;
	}

	int16_t err = ((dx > dy) ? dx : -dy) / 2;

	while (1) {
		DrawPixel(x0, y0, colour);
		if (x0 == x1 && y0 == y1) {
			break;
		}

		// Work out whether to increment the x, y or both according to the slope
		if (err > -dx) {
			err -= dy;
			x0 += 1;
		}
		if (err < dy) {
			err += dx;
			y0 += 1;
		}
	}

}


void LCD::DrawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour)
{
	// Check lines are not too long
	if (x0 >= width)	x0 = width - 1;
	if (x1 >= width)	x1 = width - 1;
	if (y0 >= height)	y0 = height - 1;
	if (y1 >= height)	y1 = height - 1;
	if (y0 < 0)			y0 = 0;
	if (y1 < 0)			y1 = 0;

	// Flip co-ordinates if wrong way round
	if (x0 > x1) 		std::swap(x0, x1);
	if (y0 > y1) 		std::swap(y0, y1);

	ColourFill(x0, y0, x1, y0, colour);
	ColourFill(x0, y0, x0, y1, colour);
	ColourFill(x0, y1, x1, y1, colour);
	ColourFill(x1, y0, x1, y1, colour);
}


void LCD::DrawChar(uint16_t x, uint16_t y, char c, const FontData *font, const uint32_t& foreground, const uint16_t background)
{
	// If at the end of a line of display, go to new line and set x to 0 position
	if ((x + font->Width) > width) {
		y += font->Height;
		x = 0;
	}

	// Write character colour data to array
	uint16_t px, py, i = 0;
	for (py = 0; py < font->Height; ++py) {
		const uint16_t fontRow = font->data[(c - 32) * font->Height + py];
		for (px = 0; px < font->Width; ++px) {
			if ((fontRow << (px)) & 0x8000) {			// for one byte characters use if ((fontRow << px) & 0x200) {
				charBuffer[currentCharBuffer][i] = foreground;
			} else {
				charBuffer[currentCharBuffer][i] = background;
			}
			++i;
		}
	}

	// Send array of data to SPI/DMA to draw
	while (SPI_DMA_Working);
	PatternFill(x, y, x + font->Width - 1, y + font->Height - 1, charBuffer[currentCharBuffer]);

	// alternate between the two character buffers so that the next character can be prepared whilst the last one is being copied to the LCD
	currentCharBuffer = currentCharBuffer == 1 ? 0 : 1;
}


// writes a character to an existing display array
void LCD::DrawCharMem(const uint16_t x, const uint16_t y, const uint16_t memWidth, uint16_t* memBuffer, char c, const FontData *font, const uint16_t foreground, const uint16_t background)
{
	// Write character colour data to array
	uint16_t px, py, i;

	for (py = 0; py < font->Height; py++) {
		i = (memWidth * (y + py)) + x;

		const uint16_t fontRow = font->data[(c - 32) * font->Height + py];
		for (px = 0; px < font->Width; ++px) {
			if ((fontRow << px) & 0x8000) {
				memBuffer[i] = foreground;
			} else {
				memBuffer[i] = background;
			}
			++i;
		}
	}
}


void LCD::DrawString(uint16_t x0, const uint16_t y0, const std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background)
{
	for (const char& c : s) {
		DrawChar(x0, y0, c, font, foreground, background);
		x0 += font->Width;
	}
}


void LCD::DrawStringMem(uint16_t x0, const uint16_t y0, uint16_t const memWidth, uint16_t* memBuffer, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background) {
	for (const char& c : s) {
		DrawCharMem(x0, y0, memWidth, memBuffer, c, font, foreground, background);
		x0 += font->Width;
	}
}


void LCD::CommandData(const cmdGC9A01A cmd, const cdArgs_t data)
{
	Command(cmd);
	while (SPI_DMA_Working);
	DCPin.SetHigh();
	for (uint8_t d : data) {
		SPISendByte(d);
	}
}


void LCD::CommandData(const uint8_t cmd, const cdArgs_t data)
{
	Command(cmd);
	while (SPI_DMA_Working);
	DCPin.SetHigh();
	for (uint8_t d : data) {
		SPISendByte(d);
	}
}


void LCD::Command(const uint8_t cmd)
{
	while (SPI_DMA_Working);
	DCPin.SetLow();
	SPISendByte(cmd);
}


void LCD::Command(const cmdGC9A01A cmd)
{
	while (SPI_DMA_Working);
//	SPI3->CFG1 &= ~SPI_CFG1_TXDMAEN;
	DCPin.SetLow();
	SPISendByte(static_cast<uint8_t>(cmd));
}


void LCD::Data(const uint8_t data)
{
	DCPin.SetHigh();
	SPISendByte(data);
}


void LCD::Data16b(const uint16_t data)
{
	DCPin.SetHigh();
	SPISendByte(data >> 8);
	SPISendByte(data & 0xFF);
}


void LCD::SPISetDataSize(const SPIDataSize_t& Mode)
{
	SPI3->CR1 &= ~SPI_CR1_SPE;
	SPI3->CFG1 &= ~SPI_CFG1_DSIZE;
	if (Mode == SPIDataSize_16b) {
		SPI3->CFG1 |= 15 << SPI_CFG1_DSIZE_Pos;				// Data size (N-1 bits)
	} else {
		SPI3->CFG1 |= 7 << SPI_CFG1_DSIZE_Pos;				// Data size (N-1 bits)
	}
	SPI3->CR1 |= SPI_CR1_SPE;
}


inline void LCD::SPISendByte(const uint8_t data)
{
	while (SPI_DMA_Working);

	if (((SPI3->CFG1 & SPI_CFG1_DSIZE) >> SPI_CFG1_DSIZE_Pos) != 7) {	// check if in 16 bit mode
		SPISetDataSize(SPIDataSize_8b);
	}

	spiTX8bit = data;								// Data must be written as 8 bit or will transfer 32 bit word
	SPI3->CR1 |= SPI_CR1_CSTART;

	while (SPI_DMA_Working);						// Wait for transmission to complete
}


void LCD::Delay(volatile uint32_t delay)		// delay must be declared volatile or will be optimised away
{
	for (; delay != 0; delay--);
}


