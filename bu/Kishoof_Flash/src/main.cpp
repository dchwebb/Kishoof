#include "initialisation.h"
#include "USB.h"
#include "WaveTable.h"
#include "FatTools.h"

volatile uint32_t SysTickVal;		// 1 ms resolution
uint32_t i2sUnderrun = 0;			// Debug counter for I2S underruns
extern uint32_t SystemCoreClock;

// Create DMA buffer that need to live in non-cached memory area
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;

//float __attribute__((section (".ram_d1_data"))) reverbMixBuffer[94000];

extern "C" {
#include "interrupts.h"
}


int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	extFlash.Init();				// Initialise external QSPI Flash
	filter.Init();					// Initialise filter coefficients, windows etc
	usb.Init(false);				// Pass false to indicate hard reset
	wavetable.Init();
	InitI2S();						// Initialise I2S which will start main sample interrupts

	while (1) {
		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands
		fatTools.CheckCache();		// Check if any outstanding cache changes need to be written to Flash
		filter.Update();			// Check if filter coefficients need to be updated

#if (USB_DEBUG)
		if (uart.commandReady) {
			uart.ProcessCommand();
		}
#endif

	}
}

