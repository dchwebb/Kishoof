#pragma once

#include "initialisation.h"
#include "fontData.h"
#include <string_view>
#include <vector>


union RGBColour  {
	uint16_t colour;
	struct __attribute__((packed)) {
		uint16_t blue : 5;
		uint16_t green : 6;
		uint16_t red : 5;
	};


	constexpr RGBColour(uint8_t r, uint8_t g, uint8_t b) {
		colour = (r << 11) + (g << 5) + b;
	}
	constexpr RGBColour(int col) : colour {static_cast<uint16_t>(col)} {};

	static RGBColour InterpolateColour(const RGBColour colour1, const RGBColour colour2, const float ratio)
	{
		//	Interpolate between two RGB 565 colours
		const uint8_t r = std::lerp(colour1.red, colour2.red, ratio);
		const uint8_t g = std::lerp(colour1.green, colour2.green, ratio);
		const uint8_t b = std::lerp(colour1.blue, colour2.blue, ratio);
		return RGBColour{r, g, b};
	}


	RGBColour DarkenColour(const uint16_t amount) const
	{
		//	Darken an RGB colour by the specified amount (apply bit offset so that all colours are treated as 6 bit values)
		uint8_t r = (red << 1) - std::min(amount, red);
		uint8_t g = green - std::min(amount, green);
		uint8_t b = (blue << 1) - std::min(amount, blue);
		return RGBColour{uint8_t(r >> 1), g, uint8_t(b >> 1)};
	}

	enum colours : uint16_t {
		White = 0xFFFF,
		Black = 0x0000,
		Grey = 0x528A,
		LightGrey = 0x8C71,
		Red = 0xF800,
		Green = 0x07E0,
		DullGreen = 0x02E0,
		Blue = 0x001F,
		LightBlue = 0x051D,
		DullBlue = 0x0293,
		DarkBlue = 0x01cd,
		Yellow = 0xFFE0,
		Orange = 0xFB44,
		DullOrange = 0xA960,
		Cyan = 0x07FF,
		Magenta = 0xA254,
		Gray = 0x7BEF,
		Brown = 0xBBCA};
};


#define MADCTL_MY 0x80 				// Bottom to top
#define MADCTL_MX 0x40 				// Right to left
#define MADCTL_MV 0x20 				// Reverse Mode
#define MADCTL_ML 0x10 				// LCD refresh Bottom to top
#define MADCTL_RGB 0x00				// Red-Green-Blue pixel order
#define MADCTL_BGR 0x08				// Blue-Green-Red pixel order
#define MADCTL_MH 0x04 				// LCD refresh right to left

// LCD command definitions
enum class cmdGC9A01A : uint8_t {
	SWRESET 	= 0x01,				// Software Reset (maybe, not documented)
	RDDID 		= 0x04,				// Read display identification information
	RDDST 		= 0x09,				// Read Display Status
	SLPIN 		= 0x10,				// Enter Sleep Mode
	SLPOUT 		= 0x11,				// Sleep Out
	PTLON 		= 0x12,				// Partial Mode ON
	NORON 		= 0x13,				// Normal Display Mode ON
	INVOFF 		= 0x20,				// Display Inversion OFF
	INVON 		= 0x21,				// Display Inversion ON
	DISPOFF 	= 0x28,				// Display OFF
	DISPON 		= 0x29,				// Display ON
	CASET 		= 0x2A,				// Column Address Set
	RASET 		= 0x2B,				// Row Address Set
	RAMWR 		= 0x2C,				// Memory Write
	PTLAR 		= 0x30,				// Partial Area
	VSCRDEF 	= 0x33,				// Vertical Scrolling Definition
	TEOFF 		= 0x34,				// Tearing Effect Line OFF
	TEON 		= 0x35,				// Tearing Effect Line ON
	MADCTL 		= 0x36,				// Memory Access Control
	VSCRSADD 	= 0x37,				// Vertical Scrolling Start Address
	IDLEOFF 	= 0x38,				// Idle mode OFF
	IDLEON 		= 0x39,				// Idle mode ON
	COLMOD 		= 0x3A,				// Pixel Format Set
	CONTINUE 	= 0x3C,				// Write Memory Continue
	TEARSET 	= 0x44,				// Set Tear Scanline
	GETLINE 	= 0x45,				// Get Scanline
	SETBRIGHT 	= 0x51,				// Write Display Brightness
	SETCTRL 	= 0x53,				// Write CTRL Display
	POWER7 		= 0xA7,				// Power Control 7
	TEWC 		= 0xBA,				// Tearing effect width control
	POWER1 		= 0xC1,				// Power Control 1
	POWER2 		= 0xC3,				// Power Control 2
	POWER3 		= 0xC4,				// Power Control 3
	POWER4 		= 0xC9,				// Power Control 4
	RDID1 		= 0xDA,				// Read ID 1
	RDID2 		= 0xDB,				// Read ID 2
	RDID3 		= 0xDC,				// Read ID 3
	FRAMERATE 	= 0xE8,				// Frame rate control
	SPI2DATA 	= 0xE9,				// SPI 2DATA control
	INREGEN2 	= 0xEF,				// Inter register enable 2
	GAMMA1 		= 0xF0,				// Set gamma 1
	GAMMA2 		= 0xF1,				// Set gamma 2
	GAMMA3 		= 0xF2,				// Set gamma 3
	GAMMA4 		= 0xF3,				// Set gamma 4
	IFACE 		= 0xF6,				// Interface control
	INREGEN1 	= 0xFE,				// Inter register enable 1
};


// Macro for creating arguments to CommandData function
typedef std::vector<uint8_t> cdArgs_t;

// Macro to check if DMA or SPI are busy
#define SPI_DMA_Working ((SPI3->SR & SPI_SR_TXP) == 0 || (SPI3->SR & SPI_SR_TXC) == 0)

struct FontData {
	const uint8_t Width;   // Font width in pixels
	const uint8_t Height;  // Font height in pixels
	const uint16_t *data;	// Pointer to data font data array
};


class LCD {
public:
	static constexpr uint16_t width = 240;
	static constexpr uint16_t height = 240;

	static constexpr FontData Font_Small {7, 10, Font7x10};
	static constexpr FontData Font_Large {11, 18, Font11x18};
	static constexpr FontData Font_XLarge {16, 26, Font16x26};

	static uint16_t drawBuffer[2][width * height];		// declared static to allow placement in non chached RAM

	void Init();
	void ScreenFill(const uint16_t colour);
	void ColourFill(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t colour);
	void PatternFill(const uint16_t x0, const uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t* pixelData);
	void DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour);
	void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour);
	void DrawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour);
	void DrawChar(uint16_t x, uint16_t y, char c, const FontData *font, const uint32_t& foreground, const uint16_t background);
	void DrawCharMem(uint16_t x, uint16_t y, uint16_t memWidth, uint16_t* memBuffer, char c, const FontData *font, const uint16_t foreground, const uint16_t background);
	void DrawString(uint16_t x0, const uint16_t y0, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background);
	void DrawStringMem(uint16_t x0, const uint16_t y0, uint16_t memWidth, uint16_t* memBuffer, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background);
	void DrawStringMemCenter(uint16_t x0, const uint16_t y0, const size_t width, uint16_t* memBuffer, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background);
private:
	enum LCD_Orientation_t { LCD_Portrait, LCD_Portrait_Flipped, LCD_Landscape, LCD_Landscape_Flipped } ;
	enum SPIDataSize_t { SPIDataSize_8b, SPIDataSize_16b };			// SPI in 8-bits mode/16-bits mode

	LCD_Orientation_t orientation = LCD_Portrait;
	static uint16_t charBuffer[2][Font_XLarge.Width * Font_XLarge.Height];
	uint8_t currentCharBuffer = 0;
	static uint16_t dmaInt16;								// Used to buffer data for DMA transfer during colour fills (static for DMA non-buffered declaration)
	uint8_t& spiTX8bit = (uint8_t&)(SPI3->TXDR);			// Byte data must be written as 8 bit or will transfer 32 bit word

	void Data(const uint8_t data);
	void Data16b(const uint16_t data);
	void Command(const cmdGC9A01A data);
	void CommandData(const cmdGC9A01A cmd, cdArgs_t data);
	void Command(const uint8_t cmd);
	void CommandData(const uint8_t cmd, const cdArgs_t data);
	void Rotate(LCD_Orientation_t orientation);
	void SetCursorPosition(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2);
	void Delay(volatile uint32_t delay);

	inline void SPISendByte(const uint8_t data);
	void SPISetDataSize(const SPIDataSize_t& Mode);
	void DMASend(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t* pixelData, bool memInc);

	GpioPin DCPin {GPIOB, 6, GpioPin::Type::Output, 0, GpioPin::DriveStrength::High};
	GpioPin ResetPin {GPIOB, 7, GpioPin::Type::Output};
	GpioPin CSPin {GPIOA, 15, GpioPin::Type::Output};
};


extern LCD lcd;
