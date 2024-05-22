#include <CDCHandler.h>
#include "initialisation.h"
#include "USB.h"
#include "WaveTable.h"
#include "Filter.h"
#include "lcd.h"
#include "ExtFlash.h"
#include "UI.h"

volatile uint32_t SysTickVal;
extern uint32_t SystemCoreClock;

// Store buffers that need to live in special memory areas
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;


/* TODO:
 * Adjust aliasing filters to cope with warp and tzfm
 * VCA on output ?
 *
 * Check drive strength on SPI pins
 */

extern "C" {
#include "interrupts.h"
}

float wavetableCalc = 0.0f;

uint32_t lastVal;
bool USBDebug = false;

int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	lcd.Init();
	extFlash.Init();

	filter.Init();					// Initialise filter coefficients, windows etc
	wavetable.Init();

	usb.Init(false);
	InitI2S();						// Initialise I2S which will start main sample interrupts

	while (1) {
		//filter.Update();			// Check if filter coefficients need to be updated
		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands
		ui.Update();
		fatTools.CheckCache();		// Check if any outstanding cache changes need to be written to Flash



#if (USB_DEBUG)
		if ((GPIOB->IDR & GPIO_IDR_ID4) == 0 && USBDebug) {
			USBDebug = false;
			usb.OutputDebug();
		}
#endif

	}
}

