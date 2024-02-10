#include <CDCHandler.h>
#include "initialisation.h"
#include "USB.h"
#include "WaveTable.h"
#include "Filter.h"
#include "sdram.h"


volatile uint32_t SysTickVal;
extern uint32_t SystemCoreClock;

// Store buffers that need to live in special memory areas
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;
int32_t __attribute__((section (".sdramSection"))) samples[SAMPLE_BUFFER_LENGTH];	// Place delay sample buffers in external SDRAM


USB usb;

extern "C" {
#include "interrupts.h"
}

uint32_t lastVal;

int main(void) {

	SystemClock_Config();			// Configure the clock and PLL
	SystemCoreClockUpdate();		// Update SystemCoreClock (system clock frequency)
	InitHardware();

	filter.Init();					// Initialise filter coefficients, windows etc
	usb.Init(false);
	wavetable.Init();
	InitI2S();						// Initialise I2S which will start main sample interrupts

	while (1) {
		filter.Update();			// Check if filter coefficients need to be updated
		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands

#if (USB_DEBUG)
		if ((GPIOB->IDR & GPIO_IDR_ID4) == 0 && USBDebug) {
			USBDebug = false;
			usb.OutputDebug();
		}
#endif

	}
}

