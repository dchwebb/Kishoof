#include <CDCHandler.h>
#include "initialisation.h"
#include "USB.h"
#include "WaveTable.h"
#include "Filter.h"
#include "lcd.h"

volatile uint32_t SysTickVal;
extern uint32_t SystemCoreClock;

// Store buffers that need to live in special memory areas
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;


extern "C" {
#include "interrupts.h"
}

uint32_t lastVal;

int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	lcd.Init();

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

