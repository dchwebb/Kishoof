#include "initialisation.h"
//#include "FatTools.h"
#include "fatfs.h"

volatile uint32_t SysTickVal;		// 1 ms resolution
extern uint32_t SystemCoreClock;


extern "C" {
#include "interrupts.h"
}


int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();

	uint8_t retSD = FATFS_LinkDriver(&SD_Driver, SDPath);
	f_mount(&SDFatFS, (TCHAR const*)SDPath, 0);

	while (1) {

#if (USB_DEBUG)
		if (uart.commandReady) {
			uart.ProcessCommand();
		}
#endif

	}
}

