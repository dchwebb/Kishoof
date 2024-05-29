#include "initialisation.h"
#include "CDCHandler.h"
#include "USB.h"
#include "WaveTable.h"
#include "configManager.h"
#include "Calib.h"
#include "Filter.h"
#include "lcd.h"
#include "ExtFlash.h"
#include "UI.h"

volatile uint32_t SysTickVal;
extern uint32_t SystemCoreClock;

// Store buffers that need to live in special memory areas
volatile ADCValues __attribute__((section (".dma_buffer"))) adc;

Config config{&wavetable.configSaver, &calib.configSaver};		// Construct config handler with list of configSavers

/* TODO:
 * Switch warp type on zero crossing
 * Cross-fade switching wavetables
 * Implement warp type button

 * Check drive strength on SPI pins
 * Allow multiple flash sectors to be used for config storage
 */

extern "C" {
#include "interrupts.h"
}

bool USBDebug = false;

int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();
	lcd.Init();
	extFlash.Init();
	filter.Init();					// Initialise filter coefficients, windows etc
	config.RestoreConfig();
	wavetable.Init();
	usb.Init(false);
	InitI2S();						// Initialise I2S which will start main sample interrupts

	while (1) {
		usb.cdc.ProcessCommand();	// Check for incoming USB serial commands
		ui.Update();
		fatTools.CheckCache();		// Check if any outstanding cache changes need to be written to Flash
		config.SaveConfig();		// Save any scheduled changes
		CheckVCA();					// Bodge to check if VCA is normalled to 3.3v
		calib.Calibrate();
#if (USB_DEBUG)
		if ((GPIOB->IDR & GPIO_IDR_ID4) == 0 && USBDebug) {
			USBDebug = false;
			usb.OutputDebug();
		}
#endif

	}
}

