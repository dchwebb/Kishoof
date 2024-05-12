#include <CDCHandler.h>
#include "initialisation.h"
#include "USB.h"
#include "WaveTable.h"
#include "Filter.h"
#include "lcd.h"
#include "ExtFlash.h"

volatile uint32_t SysTickVal;
extern uint32_t SystemCoreClock;

// Store buffers that need to live in special memory areas
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;


/* TODO:
 * Adjust aliasing filters to cope with warp and tzfm
 * Add channel B octave
 * VCA on output ?
 * channel B ring mod and mix
 *
 * Check drive strength on SPI pins
 */

extern "C" {
#include "interrupts.h"
}

float filterInterval = 0.0f;

uint32_t lastVal;
bool USBDebug = false;

int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	lcd.Init();
	extFlash.Init();

	filter.Init();					// Initialise filter coefficients, windows etc
	usb.Init(false);
	wavetable.Init();
	InitI2S();						// Initialise I2S which will start main sample interrupts

	while (1) {
		filter.Update();			// Check if filter coefficients need to be updated


		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands

		if (!(SPI_DMA_Working)) {
			//StartDebugTimer();
			wavetable.Draw();
			//filterInterval = StopDebugTimer();
			fatTools.CheckCache();		// Check if any outstanding cache changes need to be written to Flash
		}

#if (USB_DEBUG)
		if ((GPIOB->IDR & GPIO_IDR_ID4) == 0 && USBDebug) {
			USBDebug = false;
			usb.OutputDebug();
		}
#endif

	}
}

