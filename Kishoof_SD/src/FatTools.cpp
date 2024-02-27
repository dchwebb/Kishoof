#include "FatTools.h"
#include <cstring>
#include "SDCard.h"
#include "diskio.h"


FatTools __attribute__((section (".ram_d1_data"))) fatTools;

bool FatTools::InitFatFS()
{
	FRESULT res = f_mount(&fatFs, fatPath, 1) ;		// Register the file system object to the FatFs module
	if (res == FR_NO_FILESYSTEM) {
		return false;

	}
	noFileSystem = false;

	fatClusterSize = fatFs.csize * SDCard::blockSize;
	fatMaxCluster = (SDCard::blockSize * sdCard.LogBlockNbr) / fatClusterSize;		// Store largest cluster number
	return true;
}


bool FatTools::ReadBuffer(const uint32_t sector, bufferOptions options)
{
	uint8_t* buff = (uint8_t*)((options & bufferOptions::useFatBuffer) ? fatBuffer : buffer);
	int32_t& currentSector = (options & bufferOptions::useFatBuffer) ? fatBufferedSector : bufferedSector;

	if ((options & bufferOptions::forceRefresh) || (currentSector != (int32_t)sector)) {
		if (disk_read(0, buff, sector, 1) != RES_OK) {
			return false;
		}

		currentSector = sector;
	}
	return true;
}


void FatTools::PrintDirInfo(const uint32_t cluster)
{
	// Output a detailed analysis of FAT directory structure
	if (noFileSystem) {
		printf("** No file System **\r\n");
		return;
	}


	uint32_t sector;
	if (cluster == 0) {
		printf("\r\n  Attrib Cluster   Bytes    Created   Accessed Name          Clusters\r\n"
				   "  ------------------------------------------------------------------\r\n");
		sector = fatFs.database;
	} else {
		sector = fatFs.database + (fatFs.csize * (cluster - 2));
	}

	if (!ReadBuffer(sector)) {			// Copy the sector into the buffer
		return;
	}

	const FATFileInfo* fatInfo = (FATFileInfo*)buffer;


	if (fatInfo->fileSize == 0xFFFFFFFF && *(uint32_t*)fatInfo->name == 0xFFFFFFFF) {		// Handle corrupt subdirectory (where flash has not been initialised
		return;
	}

	while (fatInfo->name[0] != 0) {
		if (fatInfo->attr == 0xF) {							// Long file name
			const FATLongFilename* lfn = (FATLongFilename*)fatInfo;
			printf("%c LFN %2i                                       %-14s [0x%02x]\r\n",
					(cluster == 0 ? ' ' : '>'),
					lfn->order & (~0x40),
					GetFileName(fatInfo).c_str(),
					lfn->checksum);
		} else {
			printf("%c %s %8i %7lu %10s %10s %-14s",
					(cluster == 0 ? ' ' : '>'),
					(fatInfo->name[0] == 0xE5 ? "*Del*" : GetAttributes(fatInfo).c_str()),
					fatInfo->firstClusterLow,
					fatInfo->fileSize,
					FileDate(fatInfo->createDate).c_str(),
					FileDate(fatInfo->accessedDate).c_str(),
					GetFileName(fatInfo).c_str());

			// Print cluster chain
			if (fatInfo->name[0] != 0xE5 && fatInfo->fileSize > fatClusterSize) {

				bool seq = false;					// used to check for sequential blocks

				uint32_t cluster = (fatInfo->firstClusterHigh << 8) | fatInfo->firstClusterLow;
				printf("%lu", cluster);

				// Check the cluster chain is available in a memory buffer
				uint32_t fatBlockPos = fatFs.fatbase + (cluster / SDCard::blockSize);
				ReadBuffer(fatBlockPos, bufferOptions::useFatBuffer);
				uint32_t* clusterChain = fatBuffer;

				while (clusterChain[cluster % SDCard::blockSize] != 0x0FFFFFFF) {
					if (clusterChain[cluster % SDCard::blockSize] == cluster + 1) {
						if (!seq) {
							printf("-");
							seq = true;
						}
					} else {
						seq = false;
						printf("%lu, %lu", cluster, clusterChain[cluster % SDCard::blockSize]);
					}
					cluster = clusterChain[cluster % SDCard::blockSize];
				}
				if (seq) {
					printf("%lu", cluster);
				}

			}

			printf("\r\n");
		}

		// Recursively call function to print sub directory details (ignoring directories '.' and '..' which hold current and parent directory clusters
		if ((fatInfo->attr & AM_DIR) && (fatInfo->name[0] != '.') && (fatInfo->firstClusterLow < fatMaxCluster)) {
			PrintDirInfo(fatInfo->firstClusterLow);
			if (!ReadBuffer(sector)) {			// Copy the sector into the buffer
				return;
			}
		}

		// move to next entry updating cache if beyond buffer size
		fatInfo++;
		if ((uint32_t)fatInfo - (uint32_t)buffer >= bufferSize) {
			ReadBuffer(++sector);
			fatInfo = (FATFileInfo*)buffer;
		}

	}
}



std::string FatTools::GetAttributes(const FATFileInfo* fi)
{
	const char cs[6] = {(fi->attr & AM_RDO) ? 'R' : '-',
			(fi->attr & AM_HID) ? 'H' : '-',
			(fi->attr & AM_SYS) ? 'S' : '-',
			(fi->attr & AM_DIR) ? 'D' : '-',
			(fi->attr & AM_ARC) ? 'A' : '-', '\0'};
	const std::string s = std::string(cs);
	return s;
}


std::string FatTools::FileDate(const uint16_t date)
{
	// Bits 0–4: Day of month, valid value range 1-31 inclusive.
	// Bits 5–8: Month of year, 1 = January, valid value range 1–12 inclusive.
	// Bits 9–15: Count of years from 1980, valid value range 0–127 inclusive (1980–2107).
	return  std::to_string( date & 0b0000000000011111) + "/" +
			std::to_string((date & 0b0000000111100000) >> 5) + "/" +
			std::to_string(((date & 0b1111111000000000) >> 9) + 1980);
}


/*
 Time Format. A FAT directory entry time stamp is a 16-bit field that has a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word).

 Bits 0–4: 2-second count, valid value range 0–29 inclusive (0 – 58 seconds).
 Bits 5–10: Minutes, valid value range 0–59 inclusive.
 Bits 11–15: Hours, valid value range 0–23 inclusive.
 */

std::string FatTools::GetFileName(const FATFileInfo* fi)
{
	char cs[14];
	std::string s;
	if (fi->attr == 0xF) {									// LFN. NB - unicode characters not properly handled - just assuming ASCII char in lower byte
		FATLongFilename* lfn = (FATLongFilename*)fi;
		return std::string({lfn->name1[0], lfn->name1[2], lfn->name1[4], lfn->name1[6], lfn->name1[8],
				lfn->name2[0], lfn->name2[2], lfn->name2[4], lfn->name2[6], lfn->name2[8], lfn->name2[10],
				lfn->name3[0], lfn->name3[2], '\0'});
	} else {
		uint8_t pos = 0;
		for (uint8_t i = 0; i < 11; ++i) {
			if (fi->name[i] != 0x20) {
				cs[pos++] = fi->name[i];
			} else if (fi->name[i] == 0x20 && cs[pos - 1] == '.') {
				// do nothing
			} else {
				cs[pos++] = (fi->attr & AM_DIR) ? '\0' : '.';
			}

		}
		cs[pos] = '\0';
		return std::string(cs);
	}
}

bool FatTools::Format()
{
	//extFlash.FullErase();

	printf("Mounting File System ...\r\n");
	uint8_t fsWork[fatSectorSize];							// Work buffer for the f_mkfs()
	MKFS_PARM parms;										// Create parameter struct
	parms.fmt = FM_FAT32;									// format as FAT12/16 using SFD (Supper Floppy Drive)
	parms.n_root = 0;										// Number of root directory entries (each uses 32 bytes of storage)
	parms.align = 0;										// Default initialise remaining values
	parms.au_size = 0;
	parms.n_fat = 0;

	FRESULT res = f_mkfs(fatPath, &parms, fsWork, sizeof(fsWork));		// Make file system
	//f_mount(&fatFs, fatPath, 1) ;							// Mount the file system (needed to set up FAT locations for next functions)

//	printf("Creating system files ...\r\n");
//	MakeDummyFiles();										// Create Windows index files to force them to be created in header cache

	if (InitFatFS()) {										// Mount FAT file system and initialise directory pointers
		printf("Successfully formatted drive\r\n");
		return true;
	} else {
		printf("Failed to format drive\r\n");
		return false;
	}
}




void FatTools::InvalidateFatFSCache()
{
	// Clear the cache window in the fatFS object so that new writes will be correctly read
	fatFs.winsect = ~0;
}


// Uses FatFS to get a recursive directory listing (just shows paths and files)
void FatTools::PrintFiles(char* path)						// Start node to be scanned (also used as work area)
{
	DIR dirObj;												// Pointer to the directory object structure
	FILINFO fileInfo;										// File information structure
	FIL SDFile;
	FRESULT res = f_opendir(&dirObj, path);					// second parm is directory name (root)

	if (res == FR_OK) {
		for (;;) {
			res = f_readdir(&dirObj, &fileInfo);			// Read a directory item */
			if (res != FR_OK || fileInfo.fname[0] == 0) {	// Break on error or end of dir */
				break;
			}
			if (fileInfo.fattrib & AM_DIR) {				// It is a directory
				const uint32_t i = strlen(path);
				sprintf(&path[i], "/%s", fileInfo.fname);
				PrintFiles(path);							// Enter the directory
				path[i] = 0;
			} else {										// It is a file
				uint32_t sect = 0;
				if (f_open(&SDFile, fileInfo.fname, FA_READ) == FR_OK) {
					//sect = ClusterToSector(SDFile.obj.fs, SDFile.obj.sclust);			// Get current sector
					sect = SDFile.obj.fs->database + SDFile.obj.fs->csize * (SDFile.obj.sclust - 2);
				}
				printf("%s/%s %lu bytes sector: %lu \n", path, fileInfo.fname, fileInfo.fsize, sect);
			}
		}
		f_closedir(&dirObj);
	}
}

/*
void CreateTestFile()
{
	FIL MyFile;												// File object

	uint32_t byteswritten, bytesread;						// File write/read counts
	uint8_t wtext[] = "This is STM32 working with FatFs";	// File write buffer
	uint8_t rtext[100];										// File read buffer

	Create and Open a new text file object with write access
	if (f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
		res = f_write(&MyFile, wtext, sizeof(wtext), (unsigned int*)&byteswritten);			// Write data to the text file
		if ((byteswritten != 0) && (res == FR_OK)) {
			f_close(&MyFile);																// Close the open text file
			if (f_open(&MyFile, "STM32.TXT", FA_READ) == FR_OK) {							// Open the text file object with read access
				res = f_read(&MyFile, rtext, sizeof(rtext), (unsigned int *)&bytesread);	// Read data from the text file
				if ((bytesread > 0) && (res == FR_OK)) {
					f_close(&MyFile);														// Close the open text file
				}
			}
		}
	}
}
*/
