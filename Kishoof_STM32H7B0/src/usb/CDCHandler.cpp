#include "USB.h"
#include "CDCHandler.h"
#include "Filter.h"
#include "WaveTable.h"
#include "ExtFlash.h"
#include <stdio.h>
#include <charconv>

uint32_t flashBuff[8192];

// Check if a command has been received from USB, parse and action as required
void CDCHandler::ProcessCommand()
{
	if (!cmdPending) {
		return;
	}

	std::string_view cmd {comCmd};

	if (cmd.compare("info") == 0) {		// Print diagnostic information

		usb->SendString("Mountjoy Kishoof v1.0 - Current Settings:\r\n\r\n");


	} else if (cmd.compare("help") == 0) {

		usb->SendString("Mountjoy Kishoof\r\n"
				"\r\nSupported commands:\r\n"
				"info        -  Show diagnostic information\r\n"
				"noise       -  Use noise wavetable\r\n"
				"sine        -  Use sine wavetable\r\n"
				"wav         -  Use wav file wavetable\r\n"
				"\r\n"
#if (USB_DEBUG)
				"usbdebug    -  Start USB debugging\r\n"
				"\r\n"
#endif
		);

#if (USB_DEBUG)
	} else if (cmd.compare("usbdebug") == 0) {				// Configure gate LED
		extern bool USBDebug;
		USBDebug = true;
		usb->SendString("Press link button to dump output\r\n");
#endif

	} else if (cmd.compare("octo") == 0) {						// Switch Flash to octal mode
		extFlash.SetOctoMode();
		printf("Changed to octal mode\r\n");

	} else if (cmd.compare("resetflash") == 0) {				// Reset Flash chip and reset to SPI mode
		extFlash.Reset();
		printf("Flash reset\r\n");

	} else if (cmd.compare("ido") == 0) {						// External RAM ID (force octal mode)
		uint32_t id = extFlash.GetID(true);
		printf("Flash ID: %#010lx\r\n", id);

	} else if (cmd.compare("mmap") == 0) {						// Memory mapped
		extFlash.MemoryMapped();
		printf("Memory Mapped mode\r\n");

	} else if (cmd.compare("flashid") == 0) {					// External RAM ID
		uint32_t id = extFlash.GetID();
		printf("Flash ID: %#010lx\r\n", id);

	} else if (cmd.compare("chkbusy") == 0) {					// Check Flash Busy
		extFlash.CheckBusy();

	} else if (cmd.compare("wren") == 0) {					// Check Flash Busy
		extFlash.WriteEnable();

	} else if (cmd.compare("sreg") == 0) {						// Flash Status Register
		uint32_t status = extFlash.ReadStatusReg();
		printf("Status Register: %#010lx \r\n", status);

	} else if (cmd.compare(0, 5, "mreg:") == 0) {				// Flash Register
		uint32_t addr;
		auto res = std::from_chars(cmd.data() + cmd.find(":") + 1, cmd.data() + cmd.size(), addr, 16);
		if (res.ec == std::errc()) {
			uint32_t id = extFlash.ReadReg(addr);
			printf("Flash Register: %#010lx Data: %#010lx\r\n", addr, id);
		} else {
			usb->SendString("Invalid register\r\n");
		}

	} else if (cmd.compare(0, 7, "cfgreg:") == 0) {				// Flash Cfg2 Register
		uint32_t addr;
		auto res = std::from_chars(cmd.data() + cmd.find(":") + 1, cmd.data() + cmd.size(), addr, 16);
		if (res.ec == std::errc()) {
			uint32_t id = extFlash.ReadCfgReg(addr);
			printf("Flash Register: %#010lx Data: %#010lx\r\n", addr, id);
		} else {
			usb->SendString("Invalid register\r\n");
		}

	} else if (cmd.compare(0, 5, "rmem:") == 0) {				// Read Memory
		uint32_t addr;
		auto res = std::from_chars(cmd.data() + cmd.find(":") + 1, cmd.data() + cmd.size(), addr, 16);
		if (res.ec == std::errc()) {
			uint32_t val = extFlash.Read(addr);
			printf("Flash Data Address: %#010lx Data: %#010lx\r\n", addr, val);
		} else {
			usb->SendString("Invalid address\r\n");
		}

	} else if (cmd.compare(0, 5, "wmem:") == 0) {				// Write Memory Page
		uint32_t addr;
		auto res = std::from_chars(cmd.data() + cmd.find(":") + 1, cmd.data() + cmd.size(), addr, 16);
		if (res.ec == std::errc()) {			// no error
			// Create test buffer
			uint32_t testBuffer[64];
			for (uint32_t i = 0; i < 64; ++i) {
				testBuffer[i] = i;
			}
			extFlash.WriteData(addr, testBuffer, 64);
			printf("Write Address: %#010lx\r\n", addr);
		} else {
			usb->SendString("Invalid address\r\n");
		}

	} else if (cmd.compare(0, 5, "write") == 0) {				// Write test pattern to flash writeA:W [A = address; W = num words]
		const int32_t address = ParseInt(cmd, 'e', 0, 0xFFFFFF);
		if (address >= 0) {
			const uint32_t words = ParseInt(cmd, ':');
			printf("Writing %ld words to %ld ...\r\n", words, address);

			for (uint32_t a = 0; a < words; ++a) {
				flashBuff[a] = a + 1;
			}
			extFlash.WriteData(address, flashBuff, words);

			extFlash.MemoryMapped();
			printf("Finished\r\n");

		}


	} else if (cmd.compare(0, 4, "mem:") == 0) {		// Flash: print 256 words of memory mapped data
		const int32_t address = ParseInt(cmd, ':', 0, 0xFFFFFF);
		if (address >= 0) {
			const uint32_t* p = (uint32_t*)(0x90000000 + address);

			for (uint32_t a = 0; a < 256; a += 4) {
				printf("%6ld: %#010lx %#010lx %#010lx %#010lx\r\n", (a * 4) + address, p[a], p[a + 1], p[a + 2], p[a + 3]);
			}
		}


	} else if (cmd.compare(0, 6, "erase:") == 0) {				// Erase sector
		uint32_t addr;
		auto res = std::from_chars(cmd.data() + cmd.find(":") + 1, cmd.data() + cmd.size(), addr, 16);
		if (res.ec == std::errc()) {
			extFlash.BlockErase(addr);
			printf("Flash Sector Erase Address: %#010lx\r\n", addr);
		} else {
			usb->SendString("Invalid address\r\n");
		}

	} else if (cmd.compare("noise") == 0) {
		wavetable.wavetableType = WaveTable::TestData::noise;
		wavetable.Init();

	} else if (cmd.compare("sine") == 0) {
			wavetable.wavetableType = WaveTable::TestData::twintone;
			wavetable.Init();

	} else if (cmd.compare("wav") == 0) {
			wavetable.wavetableType = WaveTable::TestData::wavetable;
			wavetable.Init();

	} else {
		printf("Unrecognised command: %s\r\nType 'help' for supported commands\r\n", cmd.data());
	}

	cmdPending = false;
}


void CDCHandler::DataIn()
{
	if (inBuffSize > 0 && inBuffSize % USB::ep_maxPacket == 0) {
		inBuffSize = 0;
		EndPointTransfer(Direction::in, inEP, 0);				// Fixes issue transmitting an exact multiple of max packet size (n x 64)
	}
}


// As this is called from an interrupt assign the command to a variable so it can be handled in the main loop
void CDCHandler::DataOut()
{
	// Check if sufficient space in command buffer
	const uint32_t newCharCnt = std::min(outBuffCount, maxCmdLen - 1 - buffPos);

	strncpy(&comCmd[buffPos], (char*)outBuff, newCharCnt);
	buffPos += newCharCnt;

	// Check if cr has been sent yet
	if (comCmd[buffPos - 1] == 13 || comCmd[buffPos - 1] == 10 || buffPos == maxCmdLen - 1) {
		comCmd[buffPos - 1] = '\0';
		cmdPending = true;
		buffPos = 0;
	}
}


void CDCHandler::ActivateEP()
{
	EndPointActivate(USB::CDC_In,   Direction::in,  EndPointType::Bulk);			// Activate CDC in endpoint
	EndPointActivate(USB::CDC_Out,  Direction::out, EndPointType::Bulk);			// Activate CDC out endpoint
	EndPointActivate(USB::CDC_Cmd,  Direction::in,  EndPointType::Interrupt);		// Activate Command IN EP

	EndPointTransfer(Direction::out, USB::CDC_Out, USB::ep_maxPacket);
}


void CDCHandler::ClassSetup(usbRequest& req)
{
	if (req.RequestType == DtoH_Class_Interface && req.Request == GetLineCoding) {
		SetupIn(req.Length, (uint8_t*)&lineCoding);
	}

	if (req.RequestType == HtoD_Class_Interface && req.Request == SetLineCoding) {
		// Prepare to receive line coding data in ClassSetupData
		usb->classPendingData = true;
		EndPointTransfer(Direction::out, 0, req.Length);
	}
}


void CDCHandler::ClassSetupData(usbRequest& req, const uint8_t* data)
{
	// ClassSetup passes instruction to set line coding - this is the data portion where the line coding is transferred
	if (req.RequestType == HtoD_Class_Interface && req.Request == SetLineCoding) {
		lineCoding = *(LineCoding*)data;
	}
}

int32_t CDCHandler::ParseInt(const std::string_view cmd, const char precedingChar, const int32_t low, const int32_t high) {
	int32_t val = -1;
	const int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(&cmd[pos + 1], "0123456789-") > 0) {
		val = std::stoi(&cmd[pos + 1]);
	}
	if (high > low && (val > high || val < low)) {
		printf("Must be a value between %ld and %ld\r\n", low, high);
		return low - 1;
	}
	return val;
}

float CDCHandler::ParseFloat(const std::string_view cmd, const char precedingChar, const float low = 0.0f, const float high = 0.0f) {
	float val = -1.0f;
	const int8_t pos = cmd.find(precedingChar);		// locate position of character preceding
	if (pos >= 0 && std::strspn(&cmd[pos + 1], "0123456789.") > 0) {
		val = std::stof(&cmd[pos + 1]);
	}
	if (high > low && (val > high || val < low)) {
		printf("Must be a value between %f and %f\r\n", low, high);
		return low - 1.0f;
	}
	return val;
}


// Descriptor definition here as requires constants from USB class
const uint8_t CDCHandler::Descriptor[] = {
	// IAD Descriptor - Interface association descriptor for CDC class
	0x08,									// bLength (8 bytes)
	USB::IadDescriptor,						// bDescriptorType
	USB::CDCCmdInterface,					// bFirstInterface
	0x02,									// bInterfaceCount
	0x02,									// bFunctionClass (Communications and CDC Control)
	0x02,									// bFunctionSubClass
	0x01,									// bFunctionProtocol
	USB::CommunicationClass,				// String Descriptor

	// Interface Descriptor
	0x09,									// bLength: Interface Descriptor size
	USB::InterfaceDescriptor,				// bDescriptorType: Interface
	USB::CDCCmdInterface,					// bInterfaceNumber: Number of Interface
	0x00,									// bAlternateSetting: Alternate setting
	0x01,									// bNumEndpoints: 1 endpoint used
	0x02,									// bInterfaceClass: Communication Interface Class
	0x02,									// bInterfaceSubClass: Abstract Control Model
	0x01,									// bInterfaceProtocol: Common AT commands
	USB::CommunicationClass,				// iInterface

	// Header Functional Descriptor
	0x05,									// bLength: Endpoint Descriptor size
	USB::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x00,									// bDescriptorSubtype: Header Func Desc
	0x10,									// bcdCDC: spec release number
	0x01,

	// Call Management Functional Descriptor
	0x05,									// bFunctionLength
	USB::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x01,									// bDescriptorSubtype: Call Management Func Desc
	0x00,									// bmCapabilities: D0+D1
	0x01,									// bDataInterface: 1

	// ACM Functional Descriptor
	0x04,									// bFunctionLength
	USB::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x02,									// bDescriptorSubtype: Abstract Control Management desc
	0x02,									// bmCapabilities

	// Union Functional Descriptor
	0x05,									// bFunctionLength
	USB::ClassSpecificInterfaceDescriptor,	// bDescriptorType: CS_INTERFACE
	0x06,									// bDescriptorSubtype: Union func desc
	0x00,									// bMasterInterface: Communication class interface
	0x01,									// bSlaveInterface0: Data Class Interface

	// Endpoint 2 Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USB::EndpointDescriptor,				// bDescriptorType: Endpoint
	USB::CDC_Cmd,							// bEndpointAddress
	USB::Interrupt,							// bmAttributes: Interrupt
	0x08,									// wMaxPacketSize
	0x00,
	0x10,									// bInterval

	//---------------------------------------------------------------------------

	// Data class interface descriptor
	0x09,									// bLength: Endpoint Descriptor size
	USB::InterfaceDescriptor,				// bDescriptorType:
	USB::CDCDataInterface,					// bInterfaceNumber: Number of Interface
	0x00,									// bAlternateSetting: Alternate setting
	0x02,									// bNumEndpoints: Two endpoints used
	0x0A,									// bInterfaceClass: CDC
	0x00,									// bInterfaceSubClass:
	0x00,									// bInterfaceProtocol:
	0x00,									// iInterface:

	// Endpoint OUT Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USB::EndpointDescriptor,				// bDescriptorType: Endpoint
	USB::CDC_Out,							// bEndpointAddress
	USB::Bulk,								// bmAttributes: Bulk
	LOBYTE(USB::ep_maxPacket),				// wMaxPacketSize:
	HIBYTE(USB::ep_maxPacket),
	0x00,									// bInterval: ignore for Bulk transfer

	// Endpoint IN Descriptor
	0x07,									// bLength: Endpoint Descriptor size
	USB::EndpointDescriptor,				// bDescriptorType: Endpoint
	USB::CDC_In,							// bEndpointAddress
	USB::Bulk,								// bmAttributes: Bulk
	LOBYTE(USB::ep_maxPacket),				// wMaxPacketSize:
	HIBYTE(USB::ep_maxPacket),
	0x00,									// bInterval: ignore for Bulk transfer
};


uint32_t CDCHandler::GetInterfaceDescriptor(const uint8_t** buffer) {
	*buffer = Descriptor;
	return sizeof(Descriptor);
}
