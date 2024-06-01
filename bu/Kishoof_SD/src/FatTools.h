#pragma once

#include <string>
#include <array>
#include "ff.h"
#include "SDCard.h"

static constexpr uint32_t fatSectorSize = 512;										// Sector size used by FAT


// Struct to hold regular 32 byte directory entries (SFN)
struct FATFileInfo {
	enum FileAttribute {READ_ONLY = 0x01, HIDDEN = 0x02, SYSTEM = 0x04, VOLUME_ID = 0x08, DIRECTORY = 0x10, ARCHIVE = 0x20, LONG_NAME = 0xF};
	//static const uint8_t fileDeleted = 0xE5;

	char name[11];						// Short name: If name[0] == 0xE5 directory entry is free (0x00 also means free and rest of directory is free)
	uint8_t attr;						// READ_ONLY 0x01; HIDDEN 0x02; SYSTEM 0x04; VOLUME_ID 0x08; DIRECTORY 0x10; ARCHIVE 0x20; LONG_NAME 0xF
	uint8_t NTRes;						// Reserved for use by Windows NT
	uint8_t createTimeTenth;			// File creation time in count of tenths of a second
	uint16_t createTime;				// Time file was created
	uint16_t createDate;				// Date file was created
	uint16_t accessedDate;				// Last access date
	uint16_t firstClusterHigh;			// High word of first cluster number (always 0 for a FAT12 or FAT16 volume)
	uint16_t writeTime;					// Time of last write. Note that file creation is considered a write
	uint16_t writeDate;					// Date of last write
	uint16_t firstClusterLow;			// Low word of this entry’s first cluster number
	uint32_t fileSize;					// File size in bytes
};


// Long File Name (LFN) directory entries
struct FATLongFilename {
	uint8_t order;						// Order in sequence of LFN entries associated with the short dir entry at the end of the LFN set. If masked with 0x40 entry is the last long dir entry
	char name1[10];						// Characters 1-5 of the long-name sub-component in this dir entry
	uint8_t attr;						// Attribute - must be 0xF
	uint8_t Type;						// If zero, indicates a directory entry that is a sub-component of a long name
	uint8_t checksum;					// Checksum of name in the short dir entry at the end of the long dir set
	char name2[12];						// Characters 6-11 of the long-name sub-component in this dir entry
	uint16_t firstClusterLow;			// Must be ZERO
	char name3[4];						// Characters 12-13 of the long-name sub-component in this dir entry
};



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
	void PrintDirInfo(const uint32_t cluster = 0);
private:
	std::string GetFileName(const FATFileInfo* lfn);
	std::string GetAttributes(const FATFileInfo* fi);
	std::string FileDate(const uint16_t date);

	enum bufferOptions {none = 0, forceRefresh = 1, useFatBuffer = 2};
	bool ReadBuffer(const uint32_t sector, bufferOptions options = bufferOptions::none);

	FATFS fatFs;						// File system object for RAM disk logical drive
	const char fatPath[4] = "0:/";		// Logical drive path for FAT File system

	uint32_t fatClusterSize = 32768;	// Cluster size used by FAT (ie block size in data section)
	uint32_t fatMaxCluster;

	static constexpr uint32_t bufferSize = SDCard::blockSize;
	uint32_t buffer[bufferSize];		// Currently used in directory printing
	int32_t bufferedSector = -1;		// which sector is currently buffered

	uint32_t fatBuffer[bufferSize];		// Used to buffer the fat cluster chain
	int32_t fatBufferedSector = -1;		// which sector is currently buffered
};

extern FatTools fatTools;



