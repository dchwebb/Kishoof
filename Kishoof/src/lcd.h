#pragma once

#include "initialisation.h"
#include "fontData.h"
#include "GpioPin.h"
#include <string_view>


// RGB565 colours
#define LCD_WHITE		0xFFFF
#define LCD_BLACK		0x0000
#define LCD_GREY		0x528A
#define LCD_RED			0xF800
#define LCD_GREEN		0x07E0
#define LCD_DULLGREEN	0x02E0
#define LCD_BLUE		0x001F
#define LCD_LIGHTBLUE	0x051D
#define LCD_DULLBLUE	0x0293
#define LCD_YELLOW		0xFFE0
#define LCD_ORANGE		0xFBE4
#define LCD_DULLORANGE	0xA960
#define LCD_CYAN		0x07FF
#define LCD_MAGENTA		0xA254
#define LCD_GRAY		0x7BEF
#define LCD_BROWN		0xBBCA


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




// Define LCD DMA and SPI registers
#define LCD_DMA_STREAM			DMA1_Stream5
#define LCD_SPI 				SPI3
#define LCD_CLEAR_DMA_FLAGS		DMA1->HIFCR = DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTCIF5 | DMA_HIFCR_CTEIF5;

// Define macros for setting and clearing GPIO SPI pins
//#define LCD_RST_RESET	GPIOC->BSRR |= GPIO_BSRR_BR_14
//#define LCD_RST_SET 	GPIOC->BSRR |= GPIO_BSRR_BS_14
//#define LCD_DCX_RESET	GPIOC->BSRR |= GPIO_BSRR_BR_13
//#define LCD_DCX_SET		GPIOC->BSRR |= GPIO_BSRR_BS_13

// Macro for creating arguments to CommandData function
typedef std::vector<uint8_t> cdArgs_t;

// Macros to check if DMA or SPI are busy - shouldn't need to check Stream5 as this is receive
//#define SPI_DMA_Working	LCD_DMA_STREAM->NDTR || ((LCD_SPI->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || (LCD_SPI->SR & SPI_SR_BSY))
#define SPI_DMA_Working ((SPI3->SR & SPI_SR_TXP) == 0 || (SPI3->SR & SPI_SR_TXC) == 0)

struct FontData {
	const uint8_t Width;   // Font width in pixels
	const uint8_t Height;  // Font height in pixels
	const uint16_t *data;	// Pointer to data font data array
};


class LCD {
public:
	uint16_t width = 240;
	uint16_t height = 240;

	static constexpr uint16_t drawWidth = 320;
	static constexpr uint16_t drawHeight = 216;
	static constexpr uint16_t drawBufferWidth = 106;		// Maximum width of draw buffer (3 * 106 = 318 which is two short of full width but used in FFT for convenience

	static constexpr FontData Font_Small {7, 10, Font7x10};
	static constexpr FontData Font_Large {11, 18, Font11x18};
	static constexpr FontData Font_XLarge {16, 26, Font16x26};

	uint16_t drawBuffer[2][(drawHeight + 1) * drawBufferWidth];

	void Init();
	void ScreenFill(const uint16_t colour);
	void ColourFill(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1, const uint16_t colour);
	void PatternFill(const uint16_t x0, const uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t* PixelData);
	void DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour);
	void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour);
	void DrawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t colour);
	void DrawChar(uint16_t x, uint16_t y, char c, const FontData *font, const uint32_t& foreground, const uint16_t background);
	void DrawCharMem(uint16_t x, uint16_t y, uint16_t memWidth, uint16_t* memBuffer, char c, const FontData *font, const uint16_t foreground, const uint16_t background);
	void DrawString(uint16_t x0, const uint16_t y0, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background);
	void DrawStringMem(uint16_t x0, const uint16_t y0, uint16_t memWidth, uint16_t* memBuffer, std::string_view s, const FontData *font, const uint16_t foreground, const uint16_t background);

private:
	enum LCD_Orientation_t { LCD_Portrait, LCD_Portrait_Flipped, LCD_Landscape, LCD_Landscape_Flipped } ;
	enum SPIDataSize_t { SPIDataSize_8b, SPIDataSize_16b };			// SPI in 8-bits mode/16-bits mode

	LCD_Orientation_t orientation = LCD_Portrait;
	uint16_t charBuffer[2][Font_XLarge.Width * Font_XLarge.Height];
	uint8_t currentCharBuffer = 0;
	uint32_t dmaInt16;										// Used to buffer data for DMA transfer during colour fills

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
	void SPI_DMA_SendHalfWord(const uint16_t& value, const uint16_t& count);

	GpioPin DCPin {GPIOC, 11, GpioPin::Type::Output};
	GpioPin CSPin {GPIOE, 3, GpioPin::Type::Output};
};


extern LCD lcd;
