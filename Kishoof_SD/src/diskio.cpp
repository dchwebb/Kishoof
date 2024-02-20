#include <diskio.h>
#include "SDCard.h"
#include <string.h>

#define SD_TIMEOUT 30 * 1000
#define SD_DEFAULT_BLOCK_SIZE 512


#if defined(ENABLE_SCRATCH_BUFFER)
#if defined (ENABLE_SD_DMA_CACHE_MAINTENANCE)
ALIGN_32BYTES(static uint8_t scratch[BLOCKSIZE]); // 32-Byte aligned for cache maintenance
#else
__ALIGN_BEGIN static uint8_t scratch[BLOCKSIZE] __ALIGN_END;
#endif
#endif

static volatile DSTATUS Stat = STA_NOINIT;


typedef uint32_t HAL_SD_CardStateTypeDef;

#define HAL_SD_CARD_READY          0x00000001U  // Card state is ready
#define HAL_SD_CARD_IDENTIFICATION 0x00000002U  // Card is in identification state
#define HAL_SD_CARD_STANDBY        0x00000003U  // Card is in standby state
#define HAL_SD_CARD_TRANSFER       0x00000004U  // Card is in transfer state
#define HAL_SD_CARD_SENDING        0x00000005U  // Card is sending an operation
#define HAL_SD_CARD_RECEIVING      0x00000006U  // Card is receiving operation information
#define HAL_SD_CARD_PROGRAMMING    0x00000007U  // Card is in programming state
#define HAL_SD_CARD_DISCONNECTED   0x00000008U  // Card is disconnected
#define HAL_SD_CARD_ERROR          0x000000FFU  // Card response Error



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
	if (sdCard.Init() == 0) {
		return disk_status(pdrv);
	}
	return RES_NOTRDY;
}


uint8_t disk_read(uint8_t pdrv, uint8_t* writeAddress, uint32_t readSector, uint32_t sectorCount)
{
	DRESULT res = RES_ERROR;
	uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
	uint8_t ret;
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
#endif

	//ensure the SDCard is ready for a new operation
	if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
		return res;
	}

#if defined(ENABLE_SCRATCH_BUFFER)
	if (!((uint32_t)buff & 0x3)) {
#endif
		if (sdCard.ReadBlocks_DMA(writeAddress, readSector, sectorCount) == 0) {
			uint32_t ReadStatus = 0;
			// Wait that the reading process is completed or a timeout occurs
			timeout = SysTickVal;
			while ((ReadStatus == 0) && ((SysTickVal - timeout) < SD_TIMEOUT)) {}

			if (ReadStatus == 0) {
				res = RES_ERROR;
			} else {
				ReadStatus = 0;
				timeout = SysTickVal;

				while ((SysTickVal - timeout) < SD_TIMEOUT) {
					if (sdCard.GetCardState() == 0) {
						res = RES_OK;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
						// the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address, adjust the address and the D-Cache size to invalidate accordingly.
						alignedAddr = (uint32_t)buff & ~0x1F;
						SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif
						break;
					}
				}
			}
		}
#if defined(ENABLE_SCRATCH_BUFFER)
	} else {
		// Slow path, fetch each sector a part and memcpy to destination buffer
		int i;

		for (i = 0; i < count; i++) {
			ret = BSP_SD_ReadBlocks_DMA((uint32_t*)scratch, (uint32_t)sector++, 1);
			if (ret == 0) {
				// wait until the read is successful or a timeout occurs
				timeout = SysTickVal;
				while((ReadStatus == 0) && ((SysTickVal - timeout) < SD_TIMEOUT)) {}
				if (ReadStatus == 0) {
					res = RES_ERROR;
					break;
				}
				ReadStatus = 0;

#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
				// invalidate the scratch buffer before the next read to get the actual data instead of the cached one
				SCB_InvalidateDCache_by_Addr((uint32_t*)scratch, BLOCKSIZE);
#endif
				memcpy(buff, scratch, BLOCKSIZE);
				buff += BLOCKSIZE;
			} else {
				break;
			}
		}

		if ((i == count) && (ret == 0)) {
			res = RES_OK;
		}
	}
#endif

	return res;
}


uint8_t disk_write(uint8_t pdrv, const uint8_t* readBuff, uint32_t writeSector, uint32_t sectorCount)
{
	DRESULT res = RES_ERROR;
	uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
	uint8_t ret;
	int i;
#endif

	uint32_t WriteStatus = 0;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
#endif

	if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
		return res;
	}

#if defined(ENABLE_SCRATCH_BUFFER)
	if (!((uint32_t)buff & 0x3)) {
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
		// the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address adjust the address and the D-Cache size to clean accordingly.
		alignedAddr = (uint32_t)buff &  ~0x1F;
		SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif

		if (sdCard.WriteBlocks_DMA(readBuff, writeSector, sectorCount) == 0) {
			// Wait that writing process is completed or a timeout occurs

			timeout = SysTickVal;
			while ((WriteStatus == 0) && ((SysTickVal - timeout) < SD_TIMEOUT)) {}

			if (WriteStatus == 0) {
				res = RES_ERROR;
			} else {
				WriteStatus = 0;
				timeout = SysTickVal;

				while ((SysTickVal - timeout) < SD_TIMEOUT) {
					if (sdCard.GetCardState() == 0) {
						res = RES_OK;
						break;
					}
				}
			}
		}
#if defined(ENABLE_SCRATCH_BUFFER)
	} else {
		// Slow path, fetch each sector a part and memcpy to destination buffer
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
		// invalidate the scratch buffer before the next write to get the actual data instead of the cached one
		SCB_InvalidateDCache_by_Addr((uint32_t*)scratch, BLOCKSIZE);
#endif

		for (i = 0; i < count; i++) {
			WriteStatus = 0;

			memcpy((void *)scratch, (void *)buff, BLOCKSIZE);
			buff += BLOCKSIZE;

			ret = BSP_SD_WriteBlocks_DMA((uint32_t*)scratch, (uint32_t)sector++, 1);
			if (ret == 0) {
				// wait for a message from the queue or a timeout
				timeout = SysTickVal;
				while ((WriteStatus == 0) && ((SysTickVal - timeout) < SD_TIMEOUT)) {	}
				if (WriteStatus == 0) {
					break;
				}

			} else {
				break;
			}
		}
		if ((i == count) && (ret == 0))
			res = RES_OK;
	}
#endif
	return res;
}



uint8_t disk_ioctl(uint8_t pdrv, uint8_t cmd, void* buff)
{
	DRESULT res = RES_ERROR;

	if (Stat & STA_NOINIT) {
		return RES_NOTRDY;
	}

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
		*(uint32_t*)buff = sdCard.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
		res = RES_OK;
		break;

	default:
		res = RES_PARERR;
	}

	return res;
}



void BSP_SD_WriteCpltCallback(void)
{
	uint32_t WriteStatus = 1;
}


void BSP_SD_ReadCpltCallback(void)
{
	uint32_t ReadStatus = 1;
}


