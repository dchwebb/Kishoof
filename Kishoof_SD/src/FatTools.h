#pragma once

#include <string>
#include <array>
#include "ff.h"
#include "ExtFlash.h"

static constexpr uint32_t fatSectorSize = 512;										// Sector size used by FAT


// Class provides interface with FatFS library and some low level helpers not provided with the library
class FatTools
{
public:
	bool busy = false;
	bool noFileSystem = true;

	bool InitFatFS();
	bool Format();
	void InvalidateFatFSCache();
	void PrintFiles(char* path);
private:
	FATFS fatFs;						// File system object for RAM disk logical drive
	const char fatPath[4] = "0:/";		// Logical drive path for FAT File system
};

extern FatTools fatTools;



