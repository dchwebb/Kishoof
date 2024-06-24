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
bool SafeMode = false;				// Disables file system mounting, USB MSC drive is disabled, don't load config

volatile ADCValues __attribute__((section (".dma_buffer"))) adc;	// Store adc buffer in non-cached memory area

Config config{&wavetable.configSaver, &calib.configSaver, &ui.configSaver};		// Construct config handler with list of configSavers

extern "C" {
#include "interrupts.h"
}

bool USBDebug = false;

int main(void) {

	InitClocks();					// Configure the clock and PLL
	InitHardware();

	if (GpioPin::IsLow(GPIOE, 4)) {	// If encoder button is pressed enter 'safe-mode'
		SafeMode = true;
	}

	lcd.Init();
	extFlash.Init();
	filter.Init();					// Initialise look up table of filter coefficients, windows etc
	if (!SafeMode) {
		config.RestoreConfig();
	}
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

