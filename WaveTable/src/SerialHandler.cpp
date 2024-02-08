#include "SerialHandler.h"

#include <stdio.h>


int32_t SerialHandler::ParseInt(const std::string cmd, const char precedingChar, int low = 0, int high = 0) {
	int32_t val = -1;
	int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(cmd.substr(pos + 1).c_str(), "0123456789-") > 0) {
		val = stoi(cmd.substr(pos + 1));
	}
	if (high > low && (val > high || val < low)) {
		usb->SendString("Must be a value between " + std::to_string(low) + " and " + std::to_string(high) + "\r\n");
		return low - 1;
	}
	return val;
}

float SerialHandler::ParseFloat(const std::string cmd, const char precedingChar, float low = 0.0, float high = 0.0) {
	float val = -1.0f;
	int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(cmd.substr(pos + 1).c_str(), "0123456789.") > 0) {
		val = stof(cmd.substr(pos + 1));
	}
	if (high > low && (val > high || val < low)) {
		usb->SendString("Must be a value between " + std::to_string(low) + " and " + std::to_string(high) + "\r\n");
		return low - 1.0f;
	}
	return val;
}

SerialHandler::SerialHandler(USB& usbObj)
{
	usb = &usbObj;

	// bind the usb's CDC caller to the CDC handler in this class
	usb->cdcDataHandler = std::bind(&SerialHandler::Handler, this, std::placeholders::_1, std::placeholders::_2);
}



// Check if a command has been received from USB, parse and action as required
bool SerialHandler::Command()
{
	char buf[50];

	if (!CmdPending) {
		return false;
	}



	if (ComCmd.compare("info\n") == 0) {		// Print diagnostic information

		usb->SendString("Mountjoy WaveTable v1.0 - Current Settings:\r\n\r\n");


	} else if (ComCmd.compare("help\n") == 0) {

		usb->SendString("Mountjoy Retrospector\r\n"
				"\r\nSupported commands:\r\n"
				"info        -  Show diagnostic information\r\n"
				"\r\n"
#if (USB_DEBUG)
				"usbdebug    -  Start USB debugging\r\n"
				"\r\n"
#endif
		);

#if (USB_DEBUG)
	} else if (ComCmd.compare("usbdebug\n") == 0) {				// Configure gate LED
		USBDebug = true;
		usb->SendString("Press link button to dump output\r\n");
#endif




	} else if (ComCmd.compare("mem16\n") == 0 || ComCmd.compare("mem32\n") == 0) {		// Memory test
		extern bool runMemTest;
		if (!runMemTest) {
			usb->SendString("Entering memory test mode - clears, writes and reads external RAM. Type 'mem' again to stop\r\n");
			CmdPending = false;
			MemoryTest(ComCmd.compare("mem16\n") == 0);
		} else {
			usb->SendString("Memory test complete\r\n");
			runMemTest = false;
		}




	} else if (ComCmd.compare("fir\n") == 0) {					// Dump FIR filter coefficients

		// NB to_string not working. Use sprintf with following: The float formatting support is not enabled, check your MCU Settings from "Project Properties > C/C++ Build > Settings > Tool Settings",
		// or add manually "-u _printf_float" in linker flags
		for (int f = 0; f < filter.firTaps; ++f) {
			if (f > filter.firTaps / 2) {						// Using a folded FIR structure so second half of coefficients is a reflection of the first
				sprintf(buf, "%0.10f", filter.firCoeff[filter.activeFilter][filter.firTaps - f]);		// 10dp
			} else {
				sprintf(buf, "%0.10f", filter.firCoeff[filter.activeFilter][f]);
			}
			usb->SendString(std::string(buf) + "\r\n");
		}


	} else if (ComCmd.compare("wd\n") == 0) {					// Dump filter window

		for (int f = 0; f < filter.firTaps; ++f) {
			sprintf(buf, "%0.10f", filter.winCoeff[f]);			// 10dp
			usb->SendString(std::string(buf) + "\r\n");
		}


	} else {
		usb->SendString("Unrecognised command: " + ComCmd + "Type 'help' for supported commands\r\n");
	}

	CmdPending = false;
	return true;
}


void SerialHandler::Handler(uint8_t* data, uint32_t length)
{
	static bool newCmd = true;
	if (newCmd) {
		ComCmd = std::string(reinterpret_cast<char*>(data), length);
		newCmd = false;
	} else {
		ComCmd.append(reinterpret_cast<char*>(data), length);
	}
	if (*ComCmd.rbegin() == '\r')
		*ComCmd.rbegin() = '\n';

	if (*ComCmd.rbegin() == '\n') {
		CmdPending = true;
		newCmd = true;
	}

}

