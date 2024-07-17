#pragma once

#include <string>
#include <array>
#include "ff.h"
#include "ExtFlash.h"

/* FAT16 Structure on 512 MBit Flash device:

Sector = 512 bytes
Cluster = 8 * 512 bytes = 4096 bytes
Block = 4096 bytes (minimum erase size)

Note that there are 8 blocks before the cluster counting system starts
64 MBytes = 125,000 Sectors = 15625 Clusters

Block    Bytes			Description
------------------------------------
0           0 -   511	Boot Sector (AKA Reserved): 1 sector
0         512 - 32767	FAT (holds cluster linked list): 63 sectors - 15625 entries each 16 bit
8       32768 - 36863	Root Directory: 8 sectors - 128 root directory entries at 32 bytes each (32 * 128 = 4096)
9       36864 - 40959	Cluster 2 - contains 'System Volume Information' directory
10      40960 - 45055	Cluster 3 - contains 'IndexerVolume' file
11      45056 - 49151	Cluster 4 - contains 'WPSettings.dat' file
12..    49152 - 		Cluster 5 etc contains sample data
.. Data section: 7803 clusters = 31,212 sectors

*/

static constexpr uint32_t fatSectorSize = 512;										// Sector size used by FAT
static constexpr uint32_t fatSectorCount = 125000;									// 125000 sectors of 512 bytes = 64 MBytes
static constexpr uint32_t fatClusterSize = 4096;									// Cluster size used by FAT (ie block size in data section)
static constexpr uint32_t fatMaxCluster = (fatSectorSize * fatSectorCount) / fatClusterSize;		// Store largest cluster number
static constexpr uint32_t fatEraseSectors = 8;										// Number of sectors in an erase block (4096 bytes per device)
static constexpr uint32_t fatCacheSectors = 128;									// 72 in Header + extra for testing NB - must be divisible by 8 (fatEraseSectors)
static constexpr uint32_t fatDirectoryEntries = 128;								// Number of 32 byte directory items - limit to 128 to ensure directory fits in cluster


// Struct to hold regular 32 byte directory entries (SFN)
struct FATFileInfo {
	enum FileAttribute {READ_ONLY = 0x01, HIDDEN = 0x02, SYSTEM = 0x04, VOLUME_ID = 0x08, DIRECTORY = 0x10, ARCHIVE = 0x20, LONG_NAME = 0xF};
	static const uint8_t fileDeleted = 0xE5;

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
	friend class CDCHandler;
public:
	bool flushCacheBusy = false;
	bool writeBusy = false;
	static constexpr uint32_t writingWaitSet = 80;		// Block sample output for at least X ms after a write
	uint32_t writingWait = 0;			// Time to block sample output since a write last reported
	bool updateWavetables = false;		// Set during write so that updates to wavetables can be batched

	bool noFileSystem = true;
	uint16_t* clusterChain;				// Pointer to beginning of cluster chain (AKA FAT)
	FATFileInfo* rootDirectory;			// Pointer to start of FAT directory listing

	bool InitFatFS();
	void Read(uint8_t* buffAddress, const uint32_t readSector, const uint32_t sectorCount);
	const uint8_t* GetSectorAddr(const uint32_t sector);
	const uint8_t* GetClusterAddr(const uint32_t cluster, const bool ignoreCache = false);
	void Write(const uint8_t* readBuff, const uint32_t writeSector, const uint32_t sectorCount);
	void PrintDirInfo(uint32_t cluster = 0);
	void PrintFatInfo();
	void PrintFiles(char* path);
	void CheckCache();
	uint8_t FlushCache();
	void InvalidateFatFSCache();
	bool Format();
	bool Busy() { return flushCacheBusy | writeBusy | (writingWait > SysTickVal); }
private:
	FATFS fatFs;						// File system object for RAM disk logical drive
	const char fatPath[4] = "0:/";		// Logical drive path for FAT File system

	// Create cache for header part of FAT [Header consists of 1 sector boot sector; 31 sectors FAT; 8 sectors Root Directory + extra]
	uint32_t cacheUpdated = 0;			// Store the systick time the cache was last updated so separate timer can periodically clean up cache
	uint8_t headerCache[fatSectorSize * fatCacheSectors];	// Cache for storing the header section of the Flash FAT drive
	uint64_t dirtyCacheBlocks = 0;		// Bit array containing dirty blocks in header cache (block = erasesector)

	// Initialise Write Cache - this is used to cache write data into blocks for safe erasing when overwriting existing data
	uint8_t writeBlockCache[fatSectorSize * fatEraseSectors];
	int32_t writeBlock = -1;			// Keep track of which block is currently held in the write cache
	bool writeCacheDirty = false;		// Indicates whether the data in the write cache has changes

	std::string GetFileName(const FATFileInfo* lfn);
	std::string GetAttributes(const FATFileInfo* fi);
	std::string FileDate(const uint16_t date);
	void MakeDummyFiles();
	void LFNDirEntries(uint8_t* address, const char* sfn, const char* lfn1, const char* lfn2, const uint8_t checksum, const uint8_t attributes, const uint16_t cluster, const uint32_t size);

};


extern FatTools fatTools;



