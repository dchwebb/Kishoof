#include "ff_gen_drv.h"
#include "sd_diskio.h"
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
static volatile UINT WriteStatus = 0, ReadStatus = 0;

static DSTATUS SD_CheckStatus(BYTE lun);
DSTATUS SD_initialize (BYTE);
DSTATUS SD_status (BYTE);
DRESULT SD_read (BYTE, BYTE*, DWORD, UINT);

DRESULT SD_write (BYTE, const BYTE*, DWORD, UINT);
DRESULT SD_ioctl (BYTE, BYTE, void*);

const Diskio_drvTypeDef SD_Driver = {
		SD_initialize,
		SD_status,
		SD_read,
		SD_write,
		SD_ioctl,
};



typedef uint32_t HAL_SD_CardStateTypeDef;

#define HAL_SD_CARD_READY          0x00000001U  /*!< Card state is ready                     */
#define HAL_SD_CARD_IDENTIFICATION 0x00000002U  /*!< Card is in identification state         */
#define HAL_SD_CARD_STANDBY        0x00000003U  /*!< Card is in standby state                */
#define HAL_SD_CARD_TRANSFER       0x00000004U  /*!< Card is in transfer state               */
#define HAL_SD_CARD_SENDING        0x00000005U  /*!< Card is sending an operation            */
#define HAL_SD_CARD_RECEIVING      0x00000006U  /*!< Card is receiving operation information */
#define HAL_SD_CARD_PROGRAMMING    0x00000007U  /*!< Card is in programming state            */
#define HAL_SD_CARD_DISCONNECTED   0x00000008U  /*!< Card is disconnected                    */
#define HAL_SD_CARD_ERROR          0x000000FFU  /*!< Card response Error                     */



static int SD_CheckStatusWithTimeout(uint32_t timeout)
{
	// block until SDIO free or timeout
	uint32_t timer = SysTickVal;

	while (SysTickVal - timer < timeout) {
		if (sdCard.GetCardState() == HAL_SD_CARD_TRANSFER) {		// Check card is not busy
			return 0;
		}
	}

	return -1;
}


static DSTATUS SD_CheckStatus(BYTE lun)
{
	if (sdCard.GetCardState() == HAL_SD_CARD_TRANSFER) {		// Check card is not busy
		return 0;
	}

	return 1;
}


DSTATUS SD_initialize(BYTE lun)
{
	if (sdCard.Init() == 0) {
		Stat = SD_CheckStatus(lun);
	}

	return Stat;
}


DSTATUS SD_status(BYTE lun)
{
	return SD_CheckStatus(lun);
}

/**
 * @brief  Reads Sector(s)
 * @param  lun : not used
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
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
	if (!((uint32_t)buff & 0x3))
	{
#endif
		if (BSP_SD_ReadBlocks_DMA((uint32_t*)buff, (uint32_t) (sector), count) == MSD_OK) {
			ReadStatus = 0;
			// Wait that the reading process is completed or a timeout occurs
			timeout = HAL_GetTick();
			while((ReadStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {}
			/* in case of a timeout return error */
			if (ReadStatus == 0) {
				res = RES_ERROR;
			} else {
				ReadStatus = 0;
				timeout = HAL_GetTick();

				while ((HAL_GetTick() - timeout) < SD_TIMEOUT) {
					if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
						res = RES_OK;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
						/*
            the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
            adjust the address and the D-Cache size to invalidate accordingly.
						 */
						alignedAddr = (uint32_t)buff & ~0x1F;
						SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif
						break;
					}
				}
			}
		}
#if defined(ENABLE_SCRATCH_BUFFER)
	}
	else
	{
		/* Slow path, fetch each sector a part and memcpy to destination buffer */
		int i;

		for (i = 0; i < count; i++) {
			ret = BSP_SD_ReadBlocks_DMA((uint32_t*)scratch, (uint32_t)sector++, 1);
			if (ret == MSD_OK) {
				/* wait until the read is successful or a timeout occurs */

				timeout = HAL_GetTick();
				while((ReadStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT))
				{
				}
				if (ReadStatus == 0)
				{
					res = RES_ERROR;
					break;
				}
				ReadStatus = 0;

#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
				/*
				 *
				 * invalidate the scratch buffer before the next read to get the actual data instead of the cached one
				 */
				SCB_InvalidateDCache_by_Addr((uint32_t*)scratch, BLOCKSIZE);
#endif
				memcpy(buff, scratch, BLOCKSIZE);
				buff += BLOCKSIZE;
			}
			else
			{
				break;
			}
		}

		if ((i == count) && (ret == MSD_OK))
			res = RES_OK;
	}
#endif

	return res;
}


/**
 * @brief  Writes Sector(s)
 * @param  lun : not used
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */

DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res = RES_ERROR;
	uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
	uint8_t ret;
	int i;
#endif

	WriteStatus = 0;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
#endif

	if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
		return res;
	}

#if defined(ENABLE_SCRATCH_BUFFER)
	if (!((uint32_t)buff & 0x3))
	{
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)

		/*
    the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
    adjust the address and the D-Cache size to clean accordingly.
		 */
		alignedAddr = (uint32_t)buff &  ~0x1F;
		SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif

		if (BSP_SD_WriteBlocks_DMA((uint32_t*)buff, (uint32_t)(sector), count) == MSD_OK) {
			// Wait that writing process is completed or a timeout occurs

			timeout = SysTickVal;
			while ((WriteStatus == 0) && ((SysTickVal - timeout) < SD_TIMEOUT)) {}
			// in case of a timeout return error
			if (WriteStatus == 0) {
				res = RES_ERROR;
			} else {
				WriteStatus = 0;
				timeout = HAL_GetTick();

				while((HAL_GetTick() - timeout) < SD_TIMEOUT) {
					if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
						res = RES_OK;
						break;
					}
				}
			}
		}
#if defined(ENABLE_SCRATCH_BUFFER)
	} else {
		/* Slow path, fetch each sector a part and memcpy to destination buffer */
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
		/*
		 * invalidate the scratch buffer before the next write to get the actual data instead of the cached one
		 */
		SCB_InvalidateDCache_by_Addr((uint32_t*)scratch, BLOCKSIZE);
#endif

		for (i = 0; i < count; i++) {
			WriteStatus = 0;

			memcpy((void *)scratch, (void *)buff, BLOCKSIZE);
			buff += BLOCKSIZE;

			ret = BSP_SD_WriteBlocks_DMA((uint32_t*)scratch, (uint32_t)sector++, 1);
			if (ret == MSD_OK) {
				/* wait for a message from the queue or a timeout */
				timeout = HAL_GetTick();
				while((WriteStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {	}
				if (WriteStatus == 0) {
					break;
				}

			} else {
				break;
			}
		}
		if ((i == count) && (ret == MSD_OK))
			res = RES_OK;
	}
#endif
	return res;
}


/**
 * @brief  I/O control operation
 * @param  lun : not used
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;

	if (Stat & STA_NOINIT) return RES_NOTRDY;

	switch (cmd)
	{
	/* Make sure that no pending write process */
	case CTRL_SYNC :
		res = RES_OK;
		break;

		/* Get number of sectors on the disk (DWORD) */
	case GET_SECTOR_COUNT :
		*(DWORD*)buff = sdCard.LogBlockNbr;
		res = RES_OK;
		break;

		/* Get R/W sector size (WORD) */
	case GET_SECTOR_SIZE :
		*(WORD*)buff = sdCard.LogBlockSize;
		res = RES_OK;
		break;

		/* Get erase block size in unit of sector (DWORD) */
	case GET_BLOCK_SIZE :
		*(DWORD*)buff = sdCard.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
		res = RES_OK;
		break;

	default:
		res = RES_PARERR;
	}

	return res;
}



/**
 * @brief Tx Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
void BSP_SD_WriteCpltCallback(void)
{
	WriteStatus = 1;
}

/**
 * @brief Rx Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
void BSP_SD_ReadCpltCallback(void)
{
	ReadStatus = 1;
}


