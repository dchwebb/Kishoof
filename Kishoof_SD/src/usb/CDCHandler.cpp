#include "USB.h"
#include "CDCHandler.h"
#include "ExtFlash.h"
#include "FatTools.h"
#include "diskio.h"
//#include "WaveTable.h"
#include "ff.h"

uint32_t __attribute__((section (".ram_d1_data")))  flashBuff[8192];
uint32_t* heapVal;		// Debug




void CDCHandler::ProcessCommand()
{
	if (!cmdPending) {
		return;
	}

	std::string_view cmd {comCmd};

	if (cmd.compare("help") == 0) {
		usb->SendString("Mountjoy Kishoof\r\n"
				"\r\nSupported commands:\r\n"
				"saveconfig  -  Save configuration to internal memory\r\n"
				"restoreconfig  Restore saved configuration\r\n"
				"clearconfig -  Erase configuration and restart\r\n"
				"dirdetails  -  Print detailed file list for root directory\r\n"
				"dir         -  Print list of all files and their directories\r\n"
				"reboot      -  Reboot device\r\n"
				"\r\n"
				"SD Card Tools:\r\n"
				"------------\r\n"
				"printsector:A  Print 512 bytes of SD data (A = decimal address)\r\n"
				"clusterchain   List chain of FAT clusters\r\n"
				"format         Format SD Card\r\n"

		);


#if (USB_DEBUG)
	} else if (cmd.compare("usbdebug") == 0) {					// Activate USB debugging
		extern volatile bool debugStart;
		debugStart = true;
#endif



	} else if (cmd.compare("reboot") == 0) {					// Restart
		printf("Rebooting ...\r\n");
		DelayMS(10);
		Reboot();

	} else if (fatTools.noFileSystem) {
		printf("** No file system **\r\n");


	} else if (cmd.compare("dir") == 0) {						// Get basic FAT directory list
		if (fatTools.noFileSystem) {
			printf("** No file System **\r\n");
		} else {
			char workBuff[256];
			strcpy(workBuff, "/");

			fatTools.InvalidateFatFSCache();					// Ensure that the FAT FS cache is updated
			fatTools.PrintFiles(workBuff);
		}

	} else if (cmd.compare("dirdetails") == 0) {				// Get detailed FAT directory info
		fatTools.PrintDirInfo();


	} else if (cmd.compare(0, 12, "printsector:") == 0) {				// print 512 bytes of SD data
		const int32_t address = ParseInt(cmd, ':', 0, sdCard.LogBlockNbr);
		int32_t blocks = ParseInt(cmd, ',', 0, 32);
		if (address >= 0) {
			if (blocks < 1) {
				blocks = 1;
			}
			printf("Sector: %ld\r\n", address);
			uint8_t readBuffer[512 * blocks];
			disk_read(0, readBuffer, address, blocks);

			const uint32_t* p = (uint32_t*)(readBuffer);

			for (int32_t a = 0; a < blocks * 128; a += 4) {
				printf("%6ld: %#010lx %#010lx %#010lx %#010lx\r\n", (a * 4), p[a], p[a + 1], p[a + 2], p[a + 3]);
			}
		}

	} else if (cmd.compare(0, 13, "bprintsector:") == 0) {				// print 512 bytes of SD data using double buffering
		const int32_t address = ParseInt(cmd, ':', 0, sdCard.LogBlockNbr);
		int32_t blocks = 2;
		if (address >= 0) {
			printf("Sector: %ld\r\n", address);
			sdCard.ReadBlocksDMAMultiBuffer(address, blocks, &flashBuff[0], &flashBuff[128]);

			uint32_t timeout = SysTickVal;

			while (!sdCard.dmaRead && (SysTickVal - timeout) < 30000) {}

			if (sdCard.dmaRead) {
				const uint32_t* p = (uint32_t*)(flashBuff);

				for (int32_t a = 0; a < blocks * 128; a += 4) {
					printf("%6ld: %#010lx %#010lx %#010lx %#010lx\r\n", (a * 4), p[a], p[a + 1], p[a + 2], p[a + 3]);
				}
			}
		}

	} else if (cmd.compare("format") == 0) {					// Format Flash storage with FAT
		printf("Formatting flash:\r\n");
		fatTools.Format();

/*




	} else if (cmd.compare("clusterchain") == 0) {			// Print used clusters with links from FAT area
		printf("Cluster | Link\r\n");
		uint32_t cluster = 0;
		while (fatTools.clusterChain[cluster]) {
			printf("%7lu   0x%04x\r\n", cluster, fatTools.clusterChain[cluster]);
			++cluster;
		}


	} else if (cmd.compare("cacheinfo") == 0) {				// Basic counts of differences between cache and Flash
		for (uint32_t blk = 0; blk < (fatCacheSectors / fatEraseSectors); ++blk) {

			// Check if block is actually dirty or clean
			uint32_t dirtyBytes = 0, firstDirtyByte = 0, lastDirtyByte = 0;
			for (uint32_t byte = 0; byte < (fatEraseSectors * fatSectorSize); ++byte) {
				uint32_t offset = (blk * fatEraseSectors * fatSectorSize) + byte;
				if (fatTools.headerCache[offset] != flashAddress[offset]) {
					++dirtyBytes;
					if (firstDirtyByte == 0) {
						firstDirtyByte = offset;
					}
					lastDirtyByte = offset;
				}
			}

			const bool blockDirty = (fatTools.dirtyCacheBlocks & (1 << blk));
			printf("Block %2lu: %s  Dirty bytes: %lu from %lu to %lu\r\n",
					blk, (blockDirty ? "dirty" : "     "), dirtyBytes, firstDirtyByte, lastDirtyByte);
		}

		// the write cache holds any blocks currently being written to to avoid multiple block erasing when writing large data
		if (fatTools.writeCacheDirty) {
			uint32_t dirtyBytes = 0, firstDirtyByte = 0, lastDirtyByte = 0;
			for (uint32_t byte = 0; byte < (fatEraseSectors * fatSectorSize); ++byte) {
				uint32_t offset = (fatTools.writeBlock * fatEraseSectors * fatSectorSize) + byte;
				if (fatTools.writeBlockCache[byte] != flashAddress[offset]) {
					++dirtyBytes;
					if (firstDirtyByte == 0) {
						firstDirtyByte = offset;
					}
					lastDirtyByte = offset;
				}
			}

			printf("Block %2lu: %s  Dirty bytes: %lu from %lu to %lu\r\n",
					fatTools.writeBlock, (fatTools.writeCacheDirty ? "dirty" : "     "), dirtyBytes, firstDirtyByte, lastDirtyByte);

		}


	} else if (cmd.compare("cachechanges") == 0) {			// List bytes that are different in cache to Flash
		uint32_t count = 0;
		uint8_t oldCache = 0, oldFlash = 0;
		bool skipDuplicates = false;

		for (uint32_t i = 0; i < (fatCacheSectors * fatSectorSize); ++i) {

			if (flashAddress[i] != fatTools.headerCache[i]) {					// Data has changed
				if (oldCache == fatTools.headerCache[i] && oldFlash == flashAddress[i] && i > 0) {
					if (!skipDuplicates) {
						printf("...\r\n");						// Print continuation mark
						skipDuplicates = true;
					}
				} else {
					printf("%5lu c: 0x%02x f: 0x%02x\r\n", i, fatTools.headerCache[i], flashAddress[i]);
				}

				oldCache = fatTools.headerCache[i];
				oldFlash = flashAddress[i];
				++count;
			} else {
				if (skipDuplicates) {
					printf("%5lu c: 0x%02x f: 0x%02x\r\n", i - 1, oldCache, oldFlash);
					skipDuplicates = false;
				}
			}

		}
		printf("Found %lu different bytes\r\n", count);





	} else if (cmd.compare(0, 7, "setzero") == 0) {				// Set data at address to 0 [A = address; W = num words]
		const int32_t address = ParseInt(cmd, 'o', 0, 0xFFFFFF);
		if (address >= 0) {
			const uint32_t words = ParseInt(cmd, ':', 0, 0xFFFFFF);
			printf("Clearing %ld words at %ld ...\r\n", words, address);

			for (uint32_t a = 0; a < words; ++a) {
				flashBuff[a] = 0;
			}
			extFlash.WriteData(address, flashBuff, words);
			extFlash.MemoryMapped();
			printf("Finished\r\n");
		}


	} else if (cmd.compare("eraseflash") == 0) {				// Erase all flash memory
		printf("Erasing flash. Please wait ...\r\n");
		extFlash.FullErase();
		printf("Flash erased\r\n");


	} else if (cmd.compare(0, 11, "eraseblock:") == 0) {		// Erase block of flash memory
		int32_t address = ParseInt(cmd, ':', 0, 0xFFFFFF);
		if (address >= 0) {
			address = address & ~(fatEraseSectors - 1);
			extFlash.BlockErase(address);		// Force address to 4096 byte (8192 in dual flash mode) boundary

			SCB_InvalidateDCache_by_Addr((uint32_t*)(flashAddress + address), fatEraseSectors * fatSectorSize);	// Ensure cache is refreshed after write or erase
			printf("Block at address 0x%08lx erased\r\n", address);
		}
		extFlash.MemoryMapped();


	} else if (cmd.compare(0, 12, "writesector:") == 0) {		// Write 1 sector of test data: format writesector:S [S = sector]
		const int32_t sector = ParseInt(cmd, ':', 0, 0xFFFFFF);
		if (sector >= 0) {
			printf("Writing to %ld ...\r\n", sector);

			for (uint32_t a = 0; a < 8192; ++a) {
				flashBuff[a] = a + 1;
			}
			fatTools.Write((uint8_t*)flashBuff, sector, 16);
			fatTools.FlushCache();

			printf("Finished\r\n");
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
*/


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


int32_t CDCHandler::ParseInt(const std::string_view cmd, const char precedingChar, const int32_t low = 0, const int32_t high = 0) {
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
