#include "initialisation.h"
#include "FatTools.h"
#include "SDCard.h"
#include "USB.h"

volatile uint32_t SysTickVal;		// 1 ms resolution
extern uint32_t SystemCoreClock;


extern "C" {
#include "interrupts.h"
}



int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	fatTools.InitFatFS();
	usb.Init(false);				// Pass false to indicate hard reset


	while (1) {

		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands

#if (USB_DEBUG)
		if (uart.commandReady) {
			uart.ProcessCommand();
		}
#endif

	}
}

