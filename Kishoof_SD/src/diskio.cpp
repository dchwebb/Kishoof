#include <diskio.h>
#include "SDCard.h"
#include <string.h>

#define SD_TIMEOUT 200


static int SD_CheckStatusWithTimeout(uint32_t timeout)
{
	uint32_t timer = SysTickVal;
	while (SysTickVal - timer < timeout) {						// block until SDIO free or timeout
		if (sdCard.GetCardState() == HAL_SD_CARD_TRANSFER) {	// Check card is not busy
			return RES_OK;
		}
	}
	return RES_NOTRDY;
}


uint8_t disk_status(uint8_t pdrv)
{
	if (sdCard.GetCardState() == HAL_SD_CARD_TRANSFER) {		// Check card is not busy
		return RES_OK;
	}
	return RES_NOTRDY;
}


DSTATUS disk_initialize(uint8_t pdrv)
{
	if (sdCard.Init()) {
		return disk_status(pdrv);
	}
	return RES_NOTRDY;
}


uint8_t disk_read(uint8_t pdrv, uint8_t* writeAddress, uint32_t readSector, uint32_t sectorCount)
{
	DRESULT res = RES_ERROR;

	if (SD_CheckStatusWithTimeout(SD_TIMEOUT)) {	// ensure the SDCard is ready for a new operation
		return res;
	}

//	// Direct read (no DMA)
//	if (sdCard.ReadBlocks(writeAddress, readSector, sectorCount, SD_TIMEOUT) == 0) {
//		res = RES_OK;
//	}

	if (sdCard.ReadBlocks_DMA(writeAddress, readSector, sectorCount) == 0) {

		uint32_t timeout = SysTickVal;
		while (!sdCard.dmaRead && (SysTickVal - timeout) < SD_TIMEOUT) {}

		if (sdCard.dmaRead) {
			timeout = SysTickVal;

			while ((SysTickVal - timeout) < SD_TIMEOUT) {
				if (disk_status(0) == RES_OK) {
					res = RES_OK;

					// SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address
					//uint32_t alignedAddr = (uint32_t)buff & ~0x1F;
					//SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));

					break;
				}
			}
		}
	}


	return res;
}


uint8_t disk_write(uint8_t pdrv, const uint8_t* readBuff, uint32_t writeSector, uint32_t sectorCount)
{
	DRESULT res = RES_ERROR;

	if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
		return res;
	}

	// SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address adjust the address and the D-Cache size to clean accordingly.
	// uint32_t alignedAddr = (uint32_t)buff &  ~0x1F;
	// SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));

	if (sdCard.WriteBlocks_DMA(readBuff, writeSector, sectorCount) == 0) {
		uint32_t timeout = SysTickVal;
		while (!sdCard.dmaWrite && ((SysTickVal - timeout) < SD_TIMEOUT)) {}

		if (sdCard.dmaWrite) {
			sdCard.dmaWrite = false;
			timeout = SysTickVal;

			while ((SysTickVal - timeout) < SD_TIMEOUT) {
				if (disk_status(0) == RES_OK) {
					res = RES_OK;
					break;
				}
			}
		}
	}

	return res;
}



uint8_t disk_ioctl(uint8_t pdrv, uint8_t cmd, void* buff)
{
	// FIXME - check to see if SD card inserted
	DRESULT res = RES_ERROR;

	switch (cmd) {
	case CTRL_SYNC:				// Make sure that no pending write process
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:		// Get number of sectors on the disk (DWORD)
		*(uint32_t*)buff = sdCard.LogBlockNbr;
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:		// Get R/W sector size (WORD)
		*(uint32_t*)buff = sdCard.LogBlockSize;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:		// Get erase block size in unit of sector (DWORD)
		*(uint32_t*)buff = sdCard.LogBlockSize / sdCard.blockSize;
		res = RES_OK;
		break;

	default:
		res = RES_PARERR;
	}

	return res;
}


