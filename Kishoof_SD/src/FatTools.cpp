#include "FatTools.h"
#include <cstring>

FatTools fatTools;

bool FatTools::InitFatFS()
{
	const FRESULT res = f_mount(&fatFs, fatPath, 1) ;		// Register the file system object to the FatFs module
	if (res == FR_NO_FILESYSTEM) {
		return false;

	}
	noFileSystem = false;

	return true;
}


bool FatTools::Format()
{
	//extFlash.FullErase();

	printf("Mounting File System ...\r\n");
	uint8_t fsWork[fatSectorSize];							// Work buffer for the f_mkfs()
	MKFS_PARM parms;										// Create parameter struct
	parms.fmt = FM_FAT | FM_SFD;							// format as FAT12/16 using SFD (Supper Floppy Drive)
	parms.n_root = 128;										// Number of root directory entries (each uses 32 bytes of storage)
	parms.align = 0;										// Default initialise remaining values
	parms.au_size = 0;
	parms.n_fat = 0;

	f_mkfs(fatPath, &parms, fsWork, sizeof(fsWork));		// Make file system in header cache (via diskio.cpp helper functions)
	f_mount(&fatFs, fatPath, 1) ;							// Mount the file system (needed to set up FAT locations for next functions)

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
				printf("%s/%s %i bytes\n", path, fileInfo.fname, (int)fileInfo.fsize);
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
