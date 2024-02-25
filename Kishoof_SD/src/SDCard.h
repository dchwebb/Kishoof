#pragma once
#include "initialisation.h"
#include "GpioPin.h"


#define SDMMC_ERROR_NONE                     0x00000000   // No error
#define SDMMC_ERROR_CMD_CRC_FAIL             0x00000001   // Command response received (but CRC check failed)
#define SDMMC_ERROR_DATA_CRC_FAIL            0x00000002   // Data block sent/received (CRC check failed)
#define SDMMC_ERROR_CMD_RSP_TIMEOUT          0x00000004   // Command response timeout
#define SDMMC_ERROR_DATA_TIMEOUT             0x00000008   // Data timeout
#define SDMMC_ERROR_TX_UNDERRUN              0x00000010   // Transmit FIFO underrun
#define SDMMC_ERROR_RX_OVERRUN               0x00000020   // Receive FIFO overrun
#define SDMMC_ERROR_ADDR_MISALIGNED          0x00000040   // Misaligned address
#define SDMMC_ERROR_BLOCK_LEN_ERR            0x00000080   // Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length
#define SDMMC_ERROR_ERASE_SEQ_ERR            0x00000100   // An error in the sequence of erase command occurs
#define SDMMC_ERROR_BAD_ERASE_PARAM          0x00000200   // An invalid selection for erase groups
#define SDMMC_ERROR_WRITE_PROT_VIOLATION     0x00000400   // Attempt to program a write protect block
#define SDMMC_ERROR_LOCK_UNLOCK_FAILED       0x00000800   // Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card
#define SDMMC_ERROR_COM_CRC_FAILED           0x00001000   // CRC check of the previous command failed
#define SDMMC_ERROR_ILLEGAL_CMD              0x00002000   // Command is not legal for the card state
#define SDMMC_ERROR_CARD_ECC_FAILED          0x00004000   // Card internal ECC was applied but failed to correct the data
#define SDMMC_ERROR_CC_ERR                   0x00008000   // Internal card controller error
#define SDMMC_ERROR_GENERAL_UNKNOWN_ERR      0x00010000   // General or unknown error
#define SDMMC_ERROR_STREAM_READ_UNDERRUN     0x00020000   // The card could not sustain data reading in stream rmode
#define SDMMC_ERROR_STREAM_WRITE_OVERRUN     0x00040000   // The card could not sustain data programming in stream mode
#define SDMMC_ERROR_CID_CSD_OVERWRITE        0x00080000   // CID/CSD overwrite error
#define SDMMC_ERROR_WP_ERASE_SKIP            0x00100000   // Only partial address space was erased
#define SDMMC_ERROR_CARD_ECC_DISABLED        0x00200000   // Command has been executed without using internal ECC
#define SDMMC_ERROR_ERASE_RESET              0x00400000   // Erase sequence was cleared before executing because an out of erase sequence command was received
#define SDMMC_ERROR_AKE_SEQ_ERR              0x00800000   // Error in sequence of authentication
#define SDMMC_ERROR_INVALID_VOLTRANGE        0x01000000   // Error in case of invalid voltage range
#define SDMMC_ERROR_ADDR_OUT_OF_RANGE        0x02000000   // Error when addressed block is out of range
#define SDMMC_ERROR_REQUEST_NOT_APPLICABLE   0x04000000   // Error when command request is not applicable
#define SDMMC_ERROR_INVALID_PARAMETER        0x08000000   // the used parameter is not valid
#define SDMMC_ERROR_UNSUPPORTED_FEATURE      0x10000000   // Error when feature is not insupported
#define SDMMC_ERROR_BUSY                     0x20000000   // Error when transfer process is busy
#define SDMMC_ERROR_DMA                      0x40000000   // Error while DMA transfer
#define SDMMC_ERROR_TIMEOUT                  0x80000000   // Timeout error

// SDMMC Commands Index
#define SDMMC_CMD_GO_IDLE_STATE                 0   // Resets the SD memory card.
#define SDMMC_CMD_SEND_OP_COND                  1   // Sends host capacity support information and activates the card's initialization process.
#define SDMMC_CMD_ALL_SEND_CID                  2   // Asks any card connected to the host to send the CID numbers on the CMD line.
#define SDMMC_CMD_SET_REL_ADDR                  3   // Asks the card to publish a new relative address (RCA).
#define SDMMC_CMD_SET_DSR                       4   // Programs the DSR of all cards.
#define SDMMC_CMD_SDMMC_SEN_OP_COND             5   // Sends host capacity support information (HCS) and asks the accessed card to send its operating condition register (OCR) content in the response on the CMD line.*/
#define SDMMC_CMD_HS_SWITCH                     6   // Checks switchable function (mode 0) and switch card function (mode 1).
#define SDMMC_CMD_SEL_DESEL_CARD                7   // Selects the card by its own relative address and gets deselected by any other address
#define SDMMC_CMD_HS_SEND_EXT_CSD               8   // Sends SD Memory Card interface condition, which includes host supply voltage information  and asks the card whether card supports voltage.
#define SDMMC_CMD_SEND_CSD                      9   // Addressed card sends its card specific data (CSD) on the CMD line.
#define SDMMC_CMD_SEND_CID                      10  // Addressed card sends its card identification (CID) on the CMD line.
#define SDMMC_CMD_VOLTAGE_SWITCH                11  // SD card Voltage switch to 1.8V mode.
#define SDMMC_CMD_STOP_TRANSMISSION             12  // Forces the card to stop transmission.
#define SDMMC_CMD_SEND_STATUS                   13  // Addressed card sends its status register.
#define SDMMC_CMD_HS_BUSTEST_READ               14  // Reserved
#define SDMMC_CMD_GO_INACTIVE_STATE             15  // Sends an addressed card into the inactive state.
#define SDMMC_CMD_SET_BLOCKLEN                  16  // Sets the block length (in bytes for SDSC) for all following block commands (read, write, lock). Default block length is fixed to 512 Bytes. Not effective

// for SDHS and SDXC
#define SDMMC_CMD_READ_SINGLE_BLOCK             17  // Reads single block of size selected by SET_BLOCKLEN in case of SDSC, and a block of fixed 512 bytes in case of SDHC and SDXC.
#define SDMMC_CMD_READ_MULT_BLOCK               18  // Continuously transfers data blocks from card to host until interrupted by  STOP_TRANSMISSION command.
#define SDMMC_CMD_HS_BUSTEST_WRITE              19  // 64 bytes tuning pattern is sent for SDR50 and SDR104.
#define SDMMC_CMD_WRITE_DAT_UNTIL_STOP          20  // Speed class control command.
#define SDMMC_CMD_SET_BLOCK_COUNT               23  // Specify block count for CMD18 and CMD25.
#define SDMMC_CMD_WRITE_SINGLE_BLOCK            24  // Writes single block of size selected by SET_BLOCKLEN in case of SDSC, and a block of fixed 512 bytes in case of SDHC and SDXC.
#define SDMMC_CMD_WRITE_MULT_BLOCK              25  // Continuously writes blocks of data until a STOP_TRANSMISSION follows.
#define SDMMC_CMD_PROG_CID                      26  // Reserved for manufacturers.
#define SDMMC_CMD_PROG_CSD                      27  // Programming of the programmable bits of the CSD.
#define SDMMC_CMD_SET_WRITE_PROT                28  // Sets the write protection bit of the addressed group.
#define SDMMC_CMD_CLR_WRITE_PROT                29  // Clears the write protection bit of the addressed group.
#define SDMMC_CMD_SEND_WRITE_PROT               30  // Asks the card to send the status of the write protection bits.
#define SDMMC_CMD_SD_ERASE_GRP_START            32  // Sets the address of the first write block to be erased. (For SD card only).
#define SDMMC_CMD_SD_ERASE_GRP_END              33  // Sets the address of the last write block of the continuous range to be erased.
#define SDMMC_CMD_ERASE_GRP_START               35  // Sets the address of the first write block to be erased. Reserved for each command system set by switch function command (CMD6).
#define SDMMC_CMD_ERASE_GRP_END                 36  // Sets the address of the last write block of the continuous range to be erased. Reserved for each command system set by switch function command (CMD6).
#define SDMMC_CMD_ERASE                         38  // Reserved for SD security applications.
#define SDMMC_CMD_FAST_IO                       39  // SD card doesn't support it (Reserved).
#define SDMMC_CMD_GO_IRQ_STATE                  40  // SD card doesn't support it (Reserved).
#define SDMMC_CMD_LOCK_UNLOCK                   42  // Sets/resets the password or lock/unlock the card. The size of the data block is set by the SET_BLOCK_LEN command.
#define SDMMC_CMD_APP_CMD                       55  // Indicates to the card that the next command is an application specific command rather than a standard command.
#define SDMMC_CMD_GEN_CMD                       56  // Used either to transfer a data block to the card or to get a data block from the card for general purpose/application specific commands.
#define SDMMC_CMD_NO_CMD                        64  // No command

// Following commands are SD Card Specific commands. SDMMC_APP_CMD should be sent before sending these commands.
#define SDMMC_CMD_APP_SD_SET_BUSWIDTH           6   // (ACMD6) Defines the data bus width to be used for data transfer. The allowed data bus widths are given in SCR register.
#define SDMMC_CMD_SD_APP_STATUS                 13  // (ACMD13) Sends the SD status.
#define SDMMC_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS  22  // (ACMD22) Sends the number of the written (without errors) write blocks. Responds with 32bit+CRC data block.
#define SDMMC_CMD_SD_APP_OP_COND                41  // (ACMD41) Sends host capacity support information (HCS) and asks the accessed card to send its operating condition register (OCR) content in the response on the CMD line.
#define SDMMC_CMD_SD_APP_SET_CLR_CARD_DETECT    42  // (ACMD42) Connect/Disconnect the 50 KOhm pull-up resistor on CD/DAT3 (pin 1) of the card
#define SDMMC_CMD_SD_APP_SEND_SCR               51  // Reads the SD Configuration Register (SCR).
#define SDMMC_CMD_SDMMC_RW_DIRECT               52  // For SD I/O card only, reserved for security specification.
#define SDMMC_CMD_SDMMC_RW_EXTENDED             53  // For SD I/O card only, reserved for security specification.

//  Masks for errors Card Status R1 (OCR Register)
#define SDMMC_OCR_ADDR_OUT_OF_RANGE        0x80000000
#define SDMMC_OCR_ADDR_MISALIGNED          0x40000000
#define SDMMC_OCR_BLOCK_LEN_ERR            0x20000000
#define SDMMC_OCR_ERASE_SEQ_ERR            0x10000000
#define SDMMC_OCR_BAD_ERASE_PARAM          0x08000000
#define SDMMC_OCR_WRITE_PROT_VIOLATION     0x04000000
#define SDMMC_OCR_LOCK_UNLOCK_FAILED       0x01000000
#define SDMMC_OCR_COM_CRC_FAILED           0x00800000
#define SDMMC_OCR_ILLEGAL_CMD              0x00400000
#define SDMMC_OCR_CARD_ECC_FAILED          0x00200000
#define SDMMC_OCR_CC_ERROR                 0x00100000
#define SDMMC_OCR_GENERAL_UNKNOWN_ERROR    0x00080000
#define SDMMC_OCR_STREAM_READ_UNDERRUN     0x00040000
#define SDMMC_OCR_STREAM_WRITE_OVERRUN     0x00020000
#define SDMMC_OCR_CID_CSD_OVERWRITE        0x00010000
#define SDMMC_OCR_WP_ERASE_SKIP            0x00008000
#define SDMMC_OCR_CARD_ECC_DISABLED        0x00004000
#define SDMMC_OCR_ERASE_RESET              0x00002000
#define SDMMC_OCR_AKE_SEQ_ERROR            0x00000008
#define SDMMC_OCR_ERRORBITS                0xFDFFE008

#define SDMMC_R6_GENERAL_UNKNOWN_ERROR     0x00002000
#define SDMMC_R6_ILLEGAL_CMD               0x00004000
#define SDMMC_R6_COM_CRC_FAILED            0x00008000

#define SDMMC_VOLTAGE_WINDOW_SD            0x80100000
#define SDMMC_HIGH_CAPACITY                0x40000000
#define SDMMC_STD_CAPACITY                 0x00000000
#define SDMMC_CHECK_PATTERN                0x000001AA
#define SD_SWITCH_1_8V_CAPACITY            0x01000000
#define SDMMC_DDR50_SWITCH_PATTERN         0x80FFFF04
#define SDMMC_SDR104_SWITCH_PATTERN        0x80FF1F03
#define SDMMC_SDR50_SWITCH_PATTERN         0x80FF1F02
#define SDMMC_SDR25_SWITCH_PATTERN         0x80FFFF01
#define SDMMC_SDR12_SWITCH_PATTERN         0x80FFFF00

#define SDMMC_MAX_VOLT_TRIAL               0x0000FFFF

#define SDMMC_WIDE_BUS_SUPPORT             0x00040000
#define SDMMC_SINGLE_BUS_SUPPORT           0x00010000
#define SDMMC_CARD_LOCKED                  0x02000000

#ifndef SDMMC_DATATIMEOUT
#define SDMMC_DATATIMEOUT                  0xFFFFFFFF
#endif

#define SDMMC_MAX_DATA_LENGTH              0x01FFFFFF

#define SDMMC_CCCC_ERASE                   0x00000020

#define SDMMC_CMDTIMEOUT                   5000        // Command send and response timeout
#define SDMMC_MAXERASETIMEOUT              63000       // Max erase Timeout 63s
#define SDMMC_STOPTRANSFERTIMEOUT          100000000   // Timeout for STOP TRANSMISSION command


#define SDMMC_CLOCK_EDGE_RISING            0
#define SDMMC_CLOCK_EDGE_FALLING           SDMMC_CLKCR_NEGEDGE

#define SDMMC_CLOCK_POWER_SAVE_DISABLE     0
#define SDMMC_CLOCK_POWER_SAVE_ENABLE      SDMMC_CLKCR_PWRSAV

#define SDMMC_BUS_WIDE_1B                  0
#define SDMMC_BUS_WIDE_4B                  SDMMC_CLKCR_WIDBUS_0
#define SDMMC_BUS_WIDE_8B                  SDMMC_CLKCR_WIDBUS_1

#define SDMMC_SPEED_MODE_AUTO              0x00000000
#define SDMMC_SPEED_MODE_DEFAULT           0x00000001
#define SDMMC_SPEED_MODE_HIGH              0x00000002
#define SDMMC_SPEED_MODE_ULTRA             0x00000003
#define SDMMC_SPEED_MODE_ULTRA_SDR104      SDMMC_SPEED_MODE_ULTRA
#define SDMMC_SPEED_MODE_DDR               0x00000004
#define SDMMC_SPEED_MODE_ULTRA_SDR50       0x00000005

#define SD_CONTEXT_NONE                    0x00000000  // None
#define SD_CONTEXT_READ_SINGLE_BLOCK       0x00000001  // Read single block operation
#define SD_CONTEXT_READ_MULTIPLE_BLOCK     0x00000002  // Read multiple blocks operation
#define SD_CONTEXT_WRITE_SINGLE_BLOCK      0x00000010  // Write single block operation
#define SD_CONTEXT_WRITE_MULTIPLE_BLOCK    0x00000020  // Write multiple blocks operation
#define SD_CONTEXT_IT                      0x00000008  // Process in Interrupt mode
#define SD_CONTEXT_DMA                     0x00000080  // Process in DMA mode
#define SD_CONTEXT_CALLBACK                0x00000100  // DW - test for MSC callback from DMA

#define CARD_NORMAL_SPEED                  0x00000000   // Normal Speed Card <12.5Mo/s , Spec Version 1.01
#define CARD_HIGH_SPEED                    0x00000100   // High Speed Card <25Mo/s , Spec version 2.00
#define CARD_ULTRA_HIGH_SPEED              0x00000200   // UHS-I SD Card <50Mo/s for SDR50, DDR5 Cards and <104Mo/s for SDR104, Spec version 3.01

#define CARD_SDSC                          0x00000000  // SD Standard Capacity <2Go
#define CARD_SDHC_SDXC                     0x00000001  // SD High Capacity <32Go, SD Extended Capacity <2To
#define CARD_SECURED                       0x00000003

#define SDMMC_RESPONSE_SHORT               SDMMC_CMD_WAITRESP_0
#define SDMMC_RESPONSE_LONG                SDMMC_CMD_WAITRESP

#define SDMMC_WAIT_IT                      SDMMC_CMD_WAITINT
#define SDMMC_WAIT_PEND                    SDMMC_CMD_WAITPEND

#define SDMMC_CPSM_ENABLE                  SDMMC_CMD_CPSMEN

#define SDMMC_STATIC_CMD_FLAGS         ((SDMMC_STA_CCRCFAIL | SDMMC_STA_CTIMEOUT | SDMMC_STA_CMDREND | SDMMC_STA_CMDSENT | SDMMC_STA_BUSYD0END))

#define SDMMC_STATIC_DATA_FLAGS        ((SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_TXUNDERR   |\
                                         SDMMC_STA_RXOVERR  | SDMMC_STA_DATAEND  | SDMMC_STA_DHOLD      |\
                                         SDMMC_STA_DBCKEND  | SDMMC_STA_DABORT   | SDMMC_STA_IDMATE     |\
                                         SDMMC_STA_IDMABTC))

#define SDMMC_STATIC_FLAGS             ((SDMMC_STA_CCRCFAIL   | SDMMC_STA_DCRCFAIL | SDMMC_STA_CTIMEOUT |\
                                         SDMMC_STA_DTIMEOUT   | SDMMC_STA_TXUNDERR | SDMMC_STA_RXOVERR  |\
                                         SDMMC_STA_CMDREND    | SDMMC_STA_CMDSENT  | SDMMC_STA_DATAEND  |\
                                         SDMMC_STA_DHOLD      | SDMMC_STA_DBCKEND  | SDMMC_STA_DABORT   |\
                                         SDMMC_STA_BUSYD0END  | SDMMC_STA_SDIOIT   | SDMMC_STA_ACKFAIL  |\
                                         SDMMC_STA_ACKTIMEOUT | SDMMC_STA_VSWEND   | SDMMC_STA_CKSTOP   |\
                                         SDMMC_STA_IDMATE     | SDMMC_STA_IDMABTC))

#define SDMMC_FLAG_CMDACT                     SDMMC_STA_CPSMACT

#define SDMMC_DATABLOCK_SIZE_1B               0
#define SDMMC_DATABLOCK_SIZE_2B               SDMMC_DCTRL_DBLOCKSIZE_0
#define SDMMC_DATABLOCK_SIZE_4B               SDMMC_DCTRL_DBLOCKSIZE_1
#define SDMMC_DATABLOCK_SIZE_8B               (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_1)
#define SDMMC_DATABLOCK_SIZE_16B              SDMMC_DCTRL_DBLOCKSIZE_2
#define SDMMC_DATABLOCK_SIZE_32B              (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_64B              (SDMMC_DCTRL_DBLOCKSIZE_1 | SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_128B             (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_1 | SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_256B             SDMMC_DCTRL_DBLOCKSIZE_3
#define SDMMC_DATABLOCK_SIZE_512B             (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_1024B            (SDMMC_DCTRL_DBLOCKSIZE_1 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_2048B            (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_1 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_4096B            (SDMMC_DCTRL_DBLOCKSIZE_2 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_8192B            (SDMMC_DCTRL_DBLOCKSIZE_0 | SDMMC_DCTRL_DBLOCKSIZE_2 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_16384B           (SDMMC_DCTRL_DBLOCKSIZE_1 | SDMMC_DCTRL_DBLOCKSIZE_2 | SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_TRANSFER_DIR_TO_CARD            0
#define SDMMC_TRANSFER_DIR_TO_SDMMC           SDMMC_DCTRL_DTDIR

#define SDMMC_TRANSFER_MODE_BLOCK             0
#define SDMMC_TRANSFER_MODE_STREAM            SDMMC_DCTRL_DTMODE_1

#define SDMMC_DPSM_DISABLE                    0
#define SDMMC_DPSM_ENABLE                     SDMMC_DCTRL_DTEN

#define DCTRL_CLEAR_MASK         (SDMMC_DCTRL_DTEN | SDMMC_DCTRL_DTDIR | SDMMC_DCTRL_DTMODE  | SDMMC_DCTRL_DBLOCKSIZE)

#define SDMMC_DISABLE_IDMA              0
#define SDMMC_ENABLE_IDMA_SINGLE_BUFF   (SDMMC_IDMA_IDMAEN)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF0  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF1  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE | SDMMC_IDMA_IDMABACT)

#define HAL_SD_CARD_READY          0x00000001  // Card state is ready
#define HAL_SD_CARD_IDENTIFICATION 0x00000002  // Card is in identification state
#define HAL_SD_CARD_STANDBY        0x00000003  // Card is in standby state
#define HAL_SD_CARD_TRANSFER       0x00000004  // Card is in transfer state
#define HAL_SD_CARD_SENDING        0x00000005  // Card is sending an operation
#define HAL_SD_CARD_RECEIVING      0x00000006  // Card is receiving operation information
#define HAL_SD_CARD_PROGRAMMING    0x00000007  // Card is in programming state
#define HAL_SD_CARD_DISCONNECTED   0x00000008  // Card is disconnected
#define HAL_SD_CARD_ERROR          0x000000FF  // Card response Error

typedef struct {
	uint32_t DataTimeOut;         // Specifies the data timeout period in card bus clock periods.
	uint32_t DataLength;          // Specifies the number of data bytes to be transferred.
	uint32_t DataBlockSize;       // Specifies the data block size for block transfer.
	uint32_t TransferDir;         // Specifies the data transfer direction, whether the transfer is a read or write.
	uint32_t TransferMode;        // Specifies whether data transfer is in stream or block mode.
	uint32_t DPSM;                // Specifies whether SDMMC Data path state machine (DPSM) is enabled or disabled.
} SDMMC_DataInitTypeDef;

class SDCard
{
public:
	typedef struct {
		uint8_t  DataBusWidth;           // Shows the currently defined data bus width
		uint8_t  SecuredMode;            // Card is in secured mode of operation
		uint16_t CardType;               // Carries information about card type
		uint32_t ProtectedAreaSize;      // Carries information about the capacity of protected area
		uint8_t  SpeedClass;             // Carries information about the speed class of the card
		uint8_t  PerformanceMove;        // Carries information about the card's performance move
		uint8_t  AllocationUnitSize;     // Carries information about the card's allocation unit size
		uint16_t EraseSize;              // Determines the number of AUs to be erased in one operation
		uint8_t  EraseTimeout;           // Determines the timeout for any number of AU erase
		uint8_t  EraseOffset;            // Carries information about the erase offset
		uint8_t  UhsSpeedGrade;          // Carries information about the speed grade of UHS card
		uint8_t  UhsAllocationUnitSize;  // Carries information about the UHS card's allocation unit size
		uint8_t  VideoSpeedClass;        // Carries information about the Video Speed Class of UHS card
	} HAL_SD_CardStatusTypeDef;

	bool Init();
	uint32_t PowerON();
	uint32_t InitCard();
	uint32_t GetCardStatus(HAL_SD_CardStatusTypeDef *pStatus);
	uint32_t ConfigWideBusOperation();
	uint32_t WideBus_Enable();
	uint32_t FindSCR(uint32_t *pSCR);
	uint32_t GetCardState();
	uint32_t SendStatus(uint32_t *pCardStatus);
	uint32_t WriteBlocks_DMA(const uint8_t *pData, uint32_t blockAdd, uint32_t blocks, bool blocking);
	uint32_t ReadBlocks(uint8_t *pData, uint32_t blockAdd, uint32_t noBlocks, uint32_t timeout);
	uint32_t ReadBlocks_DMA(uint8_t *pData, uint32_t blockAdd, uint32_t NoBlocks, void (*callback)() = nullptr);
	void InterruptHandler();


	uint32_t CmdGoIdleState();
	uint32_t CmdOperCond();
	uint32_t CmdAppCommand(uint32_t argument);
	uint32_t CmdAppOperCommand(uint32_t argument);
	uint32_t CmdSendCID();
	uint32_t CmdSetRelAdd(uint16_t *rca);
	uint32_t CmdSendCSD(uint32_t argument);
	uint32_t GetCardCSD();
	uint32_t CmdSelDesel(uint32_t addr);
	uint32_t CmdBlockLength(uint32_t blockSize);
	uint32_t CmdStatusRegister();
	uint32_t CmdSendSCR();
	uint32_t CmdBusWidth(uint32_t busWidth);
	uint32_t CmdSendStatus(uint32_t argument);
	uint32_t CmdReadSingleBlock(uint32_t readAdd);
	uint32_t CmdReadMultiBlock(uint32_t readAdd);
	uint32_t CmdWriteSingleBlock(uint32_t writeAdd);
	uint32_t CmdWriteMultiBlock(uint32_t writeAdd);
	uint32_t CmdStopTransfer();

	uint32_t SendSDStatus(uint32_t *pSDstatus);

	uint32_t GetCmdError();
	uint32_t GetCmdResp1(uint8_t SD_CMD, uint32_t Timeout);
	uint32_t GetCmdResp2();
	uint32_t GetCmdResp3();
	uint32_t GetCmdResp4();
	uint32_t GetCmdResp5();
	uint32_t GetCmdResp6(uint8_t SD_CMD, uint16_t *pRCA);
	uint32_t GetCmdResp7();
	void ConfigData(SDMMC_DataInitTypeDef *Data);

	static inline void ClearStaticCmdFlags() {
		SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
	}
	static inline void ClearStaticDataFlags() {
		SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
	}
	static inline void ClearAllStaticFlags() {
		SDMMC1->ICR = SDMMC_STATIC_FLAGS;
	}
	static constexpr uint32_t blockSize = 512;

	uint32_t CardType;                     // card Type
	enum class CardVersion {CARD_V1_X, CARD_V2_X} cardVersion;                  // card version
	uint32_t Class;                        // class of the card class
	uint32_t RelCardAdd;                   // Relative Card Address
	uint32_t BlockNbr;                     // Card Capacity in blocks
	uint32_t BlockSize;                    // one block size in bytes
	uint32_t LogBlockNbr;                  // Card logical Capacity in blocks
	uint32_t LogBlockSize;                 // logical block size in bytes
	uint32_t CardSpeed;                    // Card Speed

	enum CardState {
		HAL_SD_STATE_RESET        = 0x00000000,  // SD not yet initialized or disabled
		HAL_SD_STATE_READY        = 0x00000001,  // SD initialized and ready for use
		HAL_SD_STATE_TIMEOUT      = 0x00000002,  // SD Timeout state
		HAL_SD_STATE_BUSY         = 0x00000003,  // SD process ongoing
		HAL_SD_STATE_PROGRAMMING  = 0x00000004,  // SD Programming State
		HAL_SD_STATE_RECEIVING    = 0x00000005,  // SD Receiving State
		HAL_SD_STATE_TRANSFER     = 0x00000006,  // SD Transfer State
		HAL_SD_STATE_ERROR        = 0x0000000F   // SD is in error state
	} State;           						// SD card State
	uint32_t ErrorCode;						// SD Card Error codes
	uint32_t Context;          				// SD transfer context

	const uint8_t *pTxBuffPtr;				// Pointer to SD Tx transfer Buffer
	uint32_t      TxXferSize;				// SD Tx Transfer size
	uint8_t       *pRxBuffPtr;				// Pointer to SD Rx transfer Buffer
	uint32_t      RxXferSize;				// SD Rx Transfer size
	bool dmaRead = false;					// Used to indicate DMA read has completed
	bool dmaWrite = false;					// Used to indicate DMA write has completed

	uint32_t CID[4];						// Card ID
	uint32_t CSD[4];						// Card Specific Data

	void (*dmaCallback)() = nullptr;		// Used to store the callback function when DMA transfer complete interupt fires

	struct {
	  uint8_t  CSDStruct;            // CSD structure
	  uint8_t  SysSpecVersion;       // System specification version
	  uint8_t  Reserved1;            // Reserved
	  uint8_t  TAAC;                 // Data read access time 1
	  uint8_t  NSAC;                 // Data read access time 2 in CLK cycles
	  uint8_t  MaxBusClkFrec;        // Max. bus clock frequency
	  uint16_t CardComdClasses;      // Card command classes
	  uint8_t  RdBlockLen;           // Max. read data block length
	  uint8_t  PartBlockRead;        // Partial blocks for read allowed
	  uint8_t  WrBlockMisalign;      // Write block misalignment
	  uint8_t  RdBlockMisalign;      // Read block misalignment
	  uint8_t  DSRImpl;              // DSR implemented
	  uint8_t  Reserved2;            // Reserved
	  uint32_t DeviceSize;           // Device Size
	  uint8_t  MaxRdCurrentVDDMin;   // Max. read current @ VDD min
	  uint8_t  MaxRdCurrentVDDMax;   // Max. read current @ VDD max
	  uint8_t  MaxWrCurrentVDDMin;   // Max. write current @ VDD min
	  uint8_t  MaxWrCurrentVDDMax;   // Max. write current @ VDD max
	  uint8_t  DeviceSizeMul;        // Device size multiplier
	  uint8_t  EraseGrSize;          // Erase group size
	  uint8_t  EraseGrMul;           // Erase group size multiplier
	  uint8_t  WrProtectGrSize;      // Write protect group size
	  uint8_t  WrProtectGrEnable;    // Write protect group enable
	  uint8_t  ManDeflECC;           // Manufacturer default ECC
	  uint8_t  WrSpeedFact;          // Write speed factor
	  uint8_t  MaxWrBlockLen;        // Max. write data block length
	  uint8_t  WriteBlockPaPartial;  // Partial blocks for write allowed
	  uint8_t  Reserved3;            // Reserved
	  uint8_t  ContentProtectAppli;  // Content protection application
	  uint8_t  FileFormatGroup;      // File format group
	  uint8_t  CopyFlag;             // Copy flag (OTP)
	  uint8_t  PermWrProtect;        // Permanent write protection
	  uint8_t  TempWrProtect;        // Temporary write protection
	  uint8_t  FileFormat;           // File format
	  uint8_t  ECC;                  // ECC code
	  uint8_t  CSD_CRC;              // CSD CRC
	  uint8_t  Reserved4;            // Always 1
	} parsedCSD;



	struct  {
		uint32_t Argument;            // Specifies the SDMMC command argument
		uint32_t CmdIndex;            // Specifies the SDMMC command index. It must be Min_Data = 0 and
		uint32_t Response;            // Specifies the SDMMC response type.
		uint32_t WaitForInterrupt;    // Specifies whether SDMMC wait for interrupt request is enabled or disabled.
		uint32_t CPSM;                // Specifies whether SDMMC Command path state machine (CPSM) is enabled or disabled.

		void Send() {
			  SDMMC1->ARG = Argument;
			  uint32_t clear = SDMMC_CMD_CMDINDEX | SDMMC_CMD_WAITRESP | SDMMC_CMD_WAITINT | SDMMC_CMD_WAITPEND | SDMMC_CMD_CPSMEN | SDMMC_CMD_CMDSUSPEND;
			  uint32_t set = CmdIndex | Response |  WaitForInterrupt | CPSM;
			  SDMMC1->CMD = (SDMMC1->CMD &(~clear)) | set;
		}
	} sdCmd;

	GpioPin cardDetect {GPIOG, 3, GpioPin::Type::Input};
};


extern SDCard sdCard;
