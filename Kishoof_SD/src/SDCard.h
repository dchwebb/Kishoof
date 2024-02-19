#pragma once
#include "initialisation.h"
#include "GpioPin.h"


#define SDMMC_ERROR_NONE                     ((uint32_t)0x00000000U)   /*!< No error                                                      */
#define SDMMC_ERROR_CMD_CRC_FAIL             ((uint32_t)0x00000001U)   /*!< Command response received (but CRC check failed)              */
#define SDMMC_ERROR_DATA_CRC_FAIL            ((uint32_t)0x00000002U)   /*!< Data block sent/received (CRC check failed)                   */
#define SDMMC_ERROR_CMD_RSP_TIMEOUT          ((uint32_t)0x00000004U)   /*!< Command response timeout                                      */
#define SDMMC_ERROR_DATA_TIMEOUT             ((uint32_t)0x00000008U)   /*!< Data timeout                                                  */
#define SDMMC_ERROR_TX_UNDERRUN              ((uint32_t)0x00000010U)   /*!< Transmit FIFO underrun                                        */
#define SDMMC_ERROR_RX_OVERRUN               ((uint32_t)0x00000020U)   /*!< Receive FIFO overrun                                          */
#define SDMMC_ERROR_ADDR_MISALIGNED          ((uint32_t)0x00000040U)   /*!< Misaligned address                                            */
#define SDMMC_ERROR_BLOCK_LEN_ERR            ((uint32_t)0x00000080U)   /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length   */
#define SDMMC_ERROR_ERASE_SEQ_ERR            ((uint32_t)0x00000100U)   /*!< An error in the sequence of erase command occurs              */
#define SDMMC_ERROR_BAD_ERASE_PARAM          ((uint32_t)0x00000200U)   /*!< An invalid selection for erase groups                         */
#define SDMMC_ERROR_WRITE_PROT_VIOLATION     ((uint32_t)0x00000400U)   /*!< Attempt to program a write protect block                      */
#define SDMMC_ERROR_LOCK_UNLOCK_FAILED       ((uint32_t)0x00000800U)   /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card    */
#define SDMMC_ERROR_COM_CRC_FAILED           ((uint32_t)0x00001000U)   /*!< CRC check of the previous command failed                      */
#define SDMMC_ERROR_ILLEGAL_CMD              ((uint32_t)0x00002000U)   /*!< Command is not legal for the card state                       */
#define SDMMC_ERROR_CARD_ECC_FAILED          ((uint32_t)0x00004000U)   /*!< Card internal ECC was applied but failed to correct the data  */
#define SDMMC_ERROR_CC_ERR                   ((uint32_t)0x00008000U)   /*!< Internal card controller error                                */
#define SDMMC_ERROR_GENERAL_UNKNOWN_ERR      ((uint32_t)0x00010000U)   /*!< General or unknown error                                      */
#define SDMMC_ERROR_STREAM_READ_UNDERRUN     ((uint32_t)0x00020000U)   /*!< The card could not sustain data reading in stream rmode       */
#define SDMMC_ERROR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00040000U)   /*!< The card could not sustain data programming in stream mode    */
#define SDMMC_ERROR_CID_CSD_OVERWRITE        ((uint32_t)0x00080000U)   /*!< CID/CSD overwrite error                                       */
#define SDMMC_ERROR_WP_ERASE_SKIP            ((uint32_t)0x00100000U)   /*!< Only partial address space was erased                         */
#define SDMMC_ERROR_CARD_ECC_DISABLED        ((uint32_t)0x00200000U)   /*!< Command has been executed without using internal ECC          */
#define SDMMC_ERROR_ERASE_RESET              ((uint32_t)0x00400000U)   /*!< Erase sequence was cleared before executing because an out of erase sequence command was received                        */
#define SDMMC_ERROR_AKE_SEQ_ERR              ((uint32_t)0x00800000U)   /*!< Error in sequence of authentication                           */
#define SDMMC_ERROR_INVALID_VOLTRANGE        ((uint32_t)0x01000000U)   /*!< Error in case of invalid voltage range                        */
#define SDMMC_ERROR_ADDR_OUT_OF_RANGE        ((uint32_t)0x02000000U)   /*!< Error when addressed block is out of range                    */
#define SDMMC_ERROR_REQUEST_NOT_APPLICABLE   ((uint32_t)0x04000000U)   /*!< Error when command request is not applicable                  */
#define SDMMC_ERROR_INVALID_PARAMETER        ((uint32_t)0x08000000U)   /*!< the used parameter is not valid                               */
#define SDMMC_ERROR_UNSUPPORTED_FEATURE      ((uint32_t)0x10000000U)   /*!< Error when feature is not insupported                         */
#define SDMMC_ERROR_BUSY                     ((uint32_t)0x20000000U)   /*!< Error when transfer process is busy                           */
#define SDMMC_ERROR_DMA                      ((uint32_t)0x40000000U)   /*!< Error while DMA transfer                                      */
#define SDMMC_ERROR_TIMEOUT                  ((uint32_t)0x80000000U)   /*!< Timeout error                                                 */

/**
 * @brief SDMMC Commands Index
 */
#define SDMMC_CMD_GO_IDLE_STATE                       ((uint8_t)0U)   /*!< Resets the SD memory card.                                                               */
#define SDMMC_CMD_SEND_OP_COND                        ((uint8_t)1U)   /*!< Sends host capacity support information and activates the card's initialization process. */
#define SDMMC_CMD_ALL_SEND_CID                        ((uint8_t)2U)   /*!< Asks any card connected to the host to send the CID numbers on the CMD line.             */
#define SDMMC_CMD_SET_REL_ADDR                        ((uint8_t)3U)   /*!< Asks the card to publish a new relative address (RCA).                                   */
#define SDMMC_CMD_SET_DSR                             ((uint8_t)4U)   /*!< Programs the DSR of all cards.                                                           */
#define SDMMC_CMD_SDMMC_SEN_OP_COND                   ((uint8_t)5U)   /*!< Sends host capacity support information (HCS) and asks the accessed card to send its operating condition register (OCR) content in the response on the CMD line.*/
#define SDMMC_CMD_HS_SWITCH                           ((uint8_t)6U)   /*!< Checks switchable function (mode 0) and switch card function (mode 1).                   */
#define SDMMC_CMD_SEL_DESEL_CARD                      ((uint8_t)7U)   /*!< Selects the card by its own relative address and gets deselected by any other address    */
#define SDMMC_CMD_HS_SEND_EXT_CSD                     ((uint8_t)8U)   /*!< Sends SD Memory Card interface condition, which includes host supply voltage information  and asks the card whether card supports voltage.                      */
#define SDMMC_CMD_SEND_CSD                            ((uint8_t)9U)   /*!< Addressed card sends its card specific data (CSD) on the CMD line.                       */
#define SDMMC_CMD_SEND_CID                            ((uint8_t)10U)  /*!< Addressed card sends its card identification (CID) on the CMD line.                      */
#define SDMMC_CMD_VOLTAGE_SWITCH                      ((uint8_t)11U)  /*!< SD card Voltage switch to 1.8V mode.                                                     */
#define SDMMC_CMD_STOP_TRANSMISSION                   ((uint8_t)12U)  /*!< Forces the card to stop transmission.                                                    */
#define SDMMC_CMD_SEND_STATUS                         ((uint8_t)13U)  /*!< Addressed card sends its status register.                                                */
#define SDMMC_CMD_HS_BUSTEST_READ                     ((uint8_t)14U)  /*!< Reserved                                                                                 */
#define SDMMC_CMD_GO_INACTIVE_STATE                   ((uint8_t)15U)  /*!< Sends an addressed card into the inactive state.                                         */
#define SDMMC_CMD_SET_BLOCKLEN                        ((uint8_t)16U)  /*!< Sets the block length (in bytes for SDSC) for all following block commands (read, write, lock). Default block length is fixed to 512 Bytes. Not effective        */
/*!< for SDHS and SDXC.                                                                       */
#define SDMMC_CMD_READ_SINGLE_BLOCK                   ((uint8_t)17U)  /*!< Reads single block of size selected by SET_BLOCKLEN in case of SDSC, and a block of fixed 512 bytes in case of SDHC and SDXC.                                    */
#define SDMMC_CMD_READ_MULT_BLOCK                     ((uint8_t)18U)  /*!< Continuously transfers data blocks from card to host until interrupted by  STOP_TRANSMISSION command.                                                            */
#define SDMMC_CMD_HS_BUSTEST_WRITE                    ((uint8_t)19U)  /*!< 64 bytes tuning pattern is sent for SDR50 and SDR104.                                    */
#define SDMMC_CMD_WRITE_DAT_UNTIL_STOP                ((uint8_t)20U)  /*!< Speed class control command.                                                             */
#define SDMMC_CMD_SET_BLOCK_COUNT                     ((uint8_t)23U)  /*!< Specify block count for CMD18 and CMD25.                                                 */
#define SDMMC_CMD_WRITE_SINGLE_BLOCK                  ((uint8_t)24U)  /*!< Writes single block of size selected by SET_BLOCKLEN in case of SDSC, and a block of fixed 512 bytes in case of SDHC and SDXC.                                   */
#define SDMMC_CMD_WRITE_MULT_BLOCK                    ((uint8_t)25U)  /*!< Continuously writes blocks of data until a STOP_TRANSMISSION follows.                    */
#define SDMMC_CMD_PROG_CID                            ((uint8_t)26U)  /*!< Reserved for manufacturers.                                                              */
#define SDMMC_CMD_PROG_CSD                            ((uint8_t)27U)  /*!< Programming of the programmable bits of the CSD.                                         */
#define SDMMC_CMD_SET_WRITE_PROT                      ((uint8_t)28U)  /*!< Sets the write protection bit of the addressed group.                                    */
#define SDMMC_CMD_CLR_WRITE_PROT                      ((uint8_t)29U)  /*!< Clears the write protection bit of the addressed group.                                  */
#define SDMMC_CMD_SEND_WRITE_PROT                     ((uint8_t)30U)  /*!< Asks the card to send the status of the write protection bits.                           */
#define SDMMC_CMD_SD_ERASE_GRP_START                  ((uint8_t)32U)  /*!< Sets the address of the first write block to be erased. (For SD card only).              */
#define SDMMC_CMD_SD_ERASE_GRP_END                    ((uint8_t)33U)  /*!< Sets the address of the last write block of the continuous range to be erased.           */
#define SDMMC_CMD_ERASE_GRP_START                     ((uint8_t)35U)  /*!< Sets the address of the first write block to be erased. Reserved for each command system set by switch function command (CMD6).                                  */
#define SDMMC_CMD_ERASE_GRP_END                       ((uint8_t)36U)  /*!< Sets the address of the last write block of the continuous range to be erased. Reserved for each command system set by switch function command (CMD6).           */
#define SDMMC_CMD_ERASE                               ((uint8_t)38U)  /*!< Reserved for SD security applications.                                                   */
#define SDMMC_CMD_FAST_IO                             ((uint8_t)39U)  /*!< SD card doesn't support it (Reserved).                                                   */
#define SDMMC_CMD_GO_IRQ_STATE                        ((uint8_t)40U)  /*!< SD card doesn't support it (Reserved).                                                   */
#define SDMMC_CMD_LOCK_UNLOCK                         ((uint8_t)42U)  /*!< Sets/resets the password or lock/unlock the card. The size of the data block is set by the SET_BLOCK_LEN command.                                                */
#define SDMMC_CMD_APP_CMD                             ((uint8_t)55U)  /*!< Indicates to the card that the next command is an application specific command rather than a standard command.                                                   */
#define SDMMC_CMD_GEN_CMD                             ((uint8_t)56U)  /*!< Used either to transfer a data block to the card or to get a data block from the card for general purpose/application specific commands.                         */
#define SDMMC_CMD_NO_CMD                              ((uint8_t)64U)  /*!< No command                                                                               */

/**
 * @brief Following commands are SD Card Specific commands.
 *        SDMMC_APP_CMD should be sent before sending these commands.
 */
#define SDMMC_CMD_APP_SD_SET_BUSWIDTH                 ((uint8_t)6U)   /*!< (ACMD6) Defines the data bus width to be used for data transfer. The allowed data bus widths are given in SCR register.                                                   */
#define SDMMC_CMD_SD_APP_STATUS                       ((uint8_t)13U)  /*!< (ACMD13) Sends the SD status.                                                            */
#define SDMMC_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((uint8_t)22U)  /*!< (ACMD22) Sends the number of the written (without errors) write blocks. Responds with 32bit+CRC data block.                                                               */
#define SDMMC_CMD_SD_APP_OP_COND                      ((uint8_t)41U)  /*!< (ACMD41) Sends host capacity support information (HCS) and asks the accessed card to send its operating condition register (OCR) content in the response on the CMD line. */
#define SDMMC_CMD_SD_APP_SET_CLR_CARD_DETECT          ((uint8_t)42U)  /*!< (ACMD42) Connect/Disconnect the 50 KOhm pull-up resistor on CD/DAT3 (pin 1) of the card  */
#define SDMMC_CMD_SD_APP_SEND_SCR                     ((uint8_t)51U)  /*!< Reads the SD Configuration Register (SCR).                                               */
#define SDMMC_CMD_SDMMC_RW_DIRECT                     ((uint8_t)52U)  /*!< For SD I/O card only, reserved for security specification.                               */
#define SDMMC_CMD_SDMMC_RW_EXTENDED                   ((uint8_t)53U)  /*!< For SD I/O card only, reserved for security specification.                               */

/**
 * @brief Following commands are MMC Specific commands.
 */
#define SDMMC_CMD_MMC_SLEEP_AWAKE                     ((uint8_t)5U)   /*!< Toggle the device between Sleep state and Standby state.                                 */

/**
 * @brief Following commands are SD Card Specific security commands.
 *        SDMMC_CMD_APP_CMD should be sent before sending these commands.
 */
#define SDMMC_CMD_SD_APP_GET_MKB                      ((uint8_t)43U)
#define SDMMC_CMD_SD_APP_GET_MID                      ((uint8_t)44U)
#define SDMMC_CMD_SD_APP_SET_CER_RN1                  ((uint8_t)45U)
#define SDMMC_CMD_SD_APP_GET_CER_RN2                  ((uint8_t)46U)
#define SDMMC_CMD_SD_APP_SET_CER_RES2                 ((uint8_t)47U)
#define SDMMC_CMD_SD_APP_GET_CER_RES1                 ((uint8_t)48U)
#define SDMMC_CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((uint8_t)18U)
#define SDMMC_CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((uint8_t)25U)
#define SDMMC_CMD_SD_APP_SECURE_ERASE                 ((uint8_t)38U)
#define SDMMC_CMD_SD_APP_CHANGE_SECURE_AREA           ((uint8_t)49U)
#define SDMMC_CMD_SD_APP_SECURE_WRITE_MKB             ((uint8_t)48U)

/**
 * @brief  Masks for errors Card Status R1 (OCR Register)
 */
#define SDMMC_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000U)
#define SDMMC_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000U)
#define SDMMC_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000U)
#define SDMMC_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000U)
#define SDMMC_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000U)
#define SDMMC_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000U)
#define SDMMC_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000U)
#define SDMMC_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000U)
#define SDMMC_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000U)
#define SDMMC_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000U)
#define SDMMC_OCR_CC_ERROR                 ((uint32_t)0x00100000U)
#define SDMMC_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000U)
#define SDMMC_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000U)
#define SDMMC_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000U)
#define SDMMC_OCR_CID_CSD_OVERWRITE        ((uint32_t)0x00010000U)
#define SDMMC_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000U)
#define SDMMC_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000U)
#define SDMMC_OCR_ERASE_RESET              ((uint32_t)0x00002000U)
#define SDMMC_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008U)
#define SDMMC_OCR_ERRORBITS                ((uint32_t)0xFDFFE008U)

#define SDMMC_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000U)
#define SDMMC_R6_ILLEGAL_CMD               ((uint32_t)0x00004000U)
#define SDMMC_R6_COM_CRC_FAILED            ((uint32_t)0x00008000U)

#define SDMMC_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000U)
#define SDMMC_HIGH_CAPACITY                ((uint32_t)0x40000000U)
#define SDMMC_STD_CAPACITY                 ((uint32_t)0x00000000U)
#define SDMMC_CHECK_PATTERN                ((uint32_t)0x000001AAU)
#define SD_SWITCH_1_8V_CAPACITY            ((uint32_t)0x01000000U)
#define SDMMC_DDR50_SWITCH_PATTERN         ((uint32_t)0x80FFFF04U)
#define SDMMC_SDR104_SWITCH_PATTERN        ((uint32_t)0x80FF1F03U)
#define SDMMC_SDR50_SWITCH_PATTERN         ((uint32_t)0x80FF1F02U)
#define SDMMC_SDR25_SWITCH_PATTERN         ((uint32_t)0x80FFFF01U)
#define SDMMC_SDR12_SWITCH_PATTERN         ((uint32_t)0x80FFFF00U)

#define SDMMC_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFFU)

#define SDMMC_MAX_TRIAL                    ((uint32_t)0x0000FFFFU)

#define SDMMC_ALLZERO                      ((uint32_t)0x00000000U)

#define SDMMC_WIDE_BUS_SUPPORT             ((uint32_t)0x00040000U)
#define SDMMC_SINGLE_BUS_SUPPORT           ((uint32_t)0x00010000U)
#define SDMMC_CARD_LOCKED                  ((uint32_t)0x02000000U)

#ifndef SDMMC_DATATIMEOUT
#define SDMMC_DATATIMEOUT                  ((uint32_t)0xFFFFFFFFU)
#endif /* SDMMC_DATATIMEOUT */
#define SDMMC_0TO7BITS                     ((uint32_t)0x000000FFU)
#define SDMMC_8TO15BITS                    ((uint32_t)0x0000FF00U)
#define SDMMC_16TO23BITS                   ((uint32_t)0x00FF0000U)
#define SDMMC_24TO31BITS                   ((uint32_t)0xFF000000U)
#define SDMMC_MAX_DATA_LENGTH              ((uint32_t)0x01FFFFFFU)

#define SDMMC_HALFFIFO                     ((uint32_t)0x00000008U)
#define SDMMC_HALFFIFOBYTES                ((uint32_t)0x00000020U)

/**
 * @brief  Command Class supported
 */
#define SDMMC_CCCC_ERASE                   ((uint32_t)0x00000020U)

#define SDMMC_CMDTIMEOUT                   ((uint32_t)5000U)        /* Command send and response timeout     */
#define SDMMC_MAXERASETIMEOUT              ((uint32_t)63000U)       /* Max erase Timeout 63 s                */
#define SDMMC_STOPTRANSFERTIMEOUT          ((uint32_t)100000000U)   /* Timeout for STOP TRANSMISSION command */


#define SDMMC_CLOCK_EDGE_RISING               ((uint32_t)0x00000000U)
#define SDMMC_CLOCK_EDGE_FALLING              SDMMC_CLKCR_NEGEDGE

#define IS_SDMMC_CLOCK_EDGE(EDGE) (((EDGE) == SDMMC_CLOCK_EDGE_RISING) || \
		((EDGE) == SDMMC_CLOCK_EDGE_FALLING))


#define SDMMC_CLOCK_POWER_SAVE_DISABLE         ((uint32_t)0x00000000U)
#define SDMMC_CLOCK_POWER_SAVE_ENABLE          SDMMC_CLKCR_PWRSAV

#define IS_SDMMC_CLOCK_POWER_SAVE(SAVE) (((SAVE) == SDMMC_CLOCK_POWER_SAVE_DISABLE) || \
		((SAVE) == SDMMC_CLOCK_POWER_SAVE_ENABLE))
#define SDMMC_BUS_WIDE_1B                      ((uint32_t)0x00000000U)
#define SDMMC_BUS_WIDE_4B                      SDMMC_CLKCR_WIDBUS_0
#define SDMMC_BUS_WIDE_8B                      SDMMC_CLKCR_WIDBUS_1

#define IS_SDMMC_BUS_WIDE(WIDE) (((WIDE) == SDMMC_BUS_WIDE_1B) || \
		((WIDE) == SDMMC_BUS_WIDE_4B) || \
		((WIDE) == SDMMC_BUS_WIDE_8B))

#define SDMMC_SPEED_MODE_AUTO                  ((uint32_t)0x00000000U)
#define SDMMC_SPEED_MODE_DEFAULT               ((uint32_t)0x00000001U)
#define SDMMC_SPEED_MODE_HIGH                  ((uint32_t)0x00000002U)
#define SDMMC_SPEED_MODE_ULTRA                 ((uint32_t)0x00000003U)
#define SDMMC_SPEED_MODE_ULTRA_SDR104          SDMMC_SPEED_MODE_ULTRA
#define SDMMC_SPEED_MODE_DDR                   ((uint32_t)0x00000004U)
#define SDMMC_SPEED_MODE_ULTRA_SDR50           ((uint32_t)0x00000005U)



#define   SD_CONTEXT_NONE                 ((uint32_t)0x00000000U)  /*!< None                             */
#define   SD_CONTEXT_READ_SINGLE_BLOCK    ((uint32_t)0x00000001U)  /*!< Read single block operation      */
#define   SD_CONTEXT_READ_MULTIPLE_BLOCK  ((uint32_t)0x00000002U)  /*!< Read multiple blocks operation   */
#define   SD_CONTEXT_WRITE_SINGLE_BLOCK   ((uint32_t)0x00000010U)  /*!< Write single block operation     */
#define   SD_CONTEXT_WRITE_MULTIPLE_BLOCK ((uint32_t)0x00000020U)  /*!< Write multiple blocks operation  */
#define   SD_CONTEXT_IT                   ((uint32_t)0x00000008U)  /*!< Process in Interrupt mode        */
#define   SD_CONTEXT_DMA                  ((uint32_t)0x00000080U)  /*!< Process in DMA mode              */

#define CARD_NORMAL_SPEED        ((uint32_t)0x00000000U)    /*!< Normal Speed Card <12.5Mo/s , Spec Version 1.01    */
#define CARD_HIGH_SPEED          ((uint32_t)0x00000100U)    /*!< High Speed Card <25Mo/s , Spec version 2.00        */
#define CARD_ULTRA_HIGH_SPEED    ((uint32_t)0x00000200U)    /*!< UHS-I SD Card <50Mo/s for SDR50, DDR5 Cards
                                                                 and <104Mo/s for SDR104, Spec version 3.01          */

#define CARD_SDSC                  ((uint32_t)0x00000000U)  /*!< SD Standard Capacity <2Go                          */
#define CARD_SDHC_SDXC             ((uint32_t)0x00000001U)  /*!< SD High Capacity <32Go, SD Extended Capacity <2To  */
#define CARD_SECURED               ((uint32_t)0x00000003U)

#define CARD_V1_X                  ((uint32_t)0x00000000U)
#define CARD_V2_X                  ((uint32_t)0x00000001U)

#define SDMMC_RESPONSE_SHORT                 SDMMC_CMD_WAITRESP_0
#define SDMMC_RESPONSE_LONG                  SDMMC_CMD_WAITRESP

#define SDMMC_WAIT_IT                        SDMMC_CMD_WAITINT
#define SDMMC_WAIT_PEND                      SDMMC_CMD_WAITPEND

#define SDMMC_CPSM_ENABLE                    SDMMC_CMD_CPSMEN

#define SDMMC_STATIC_CMD_FLAGS         ((uint32_t)(SDMMC_STA_CCRCFAIL | SDMMC_STA_CTIMEOUT  | SDMMC_STA_CMDREND | SDMMC_STA_CMDSENT  | SDMMC_STA_BUSYD0END))

#define SDMMC_FLAG_CMDACT                    SDMMC_STA_CPSMACT

#define SDMMC_DATABLOCK_SIZE_1B               ((uint32_t)0x00000000U)
#define SDMMC_DATABLOCK_SIZE_2B               SDMMC_DCTRL_DBLOCKSIZE_0
#define SDMMC_DATABLOCK_SIZE_4B               SDMMC_DCTRL_DBLOCKSIZE_1
#define SDMMC_DATABLOCK_SIZE_8B               (SDMMC_DCTRL_DBLOCKSIZE_0|SDMMC_DCTRL_DBLOCKSIZE_1)
#define SDMMC_DATABLOCK_SIZE_16B              SDMMC_DCTRL_DBLOCKSIZE_2
#define SDMMC_DATABLOCK_SIZE_32B              (SDMMC_DCTRL_DBLOCKSIZE_0|SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_64B              (SDMMC_DCTRL_DBLOCKSIZE_1|SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_128B             (SDMMC_DCTRL_DBLOCKSIZE_0| \
                                               SDMMC_DCTRL_DBLOCKSIZE_1|SDMMC_DCTRL_DBLOCKSIZE_2)
#define SDMMC_DATABLOCK_SIZE_256B             SDMMC_DCTRL_DBLOCKSIZE_3
#define SDMMC_DATABLOCK_SIZE_512B             (SDMMC_DCTRL_DBLOCKSIZE_0|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_1024B            (SDMMC_DCTRL_DBLOCKSIZE_1|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_2048B            (SDMMC_DCTRL_DBLOCKSIZE_0| \
                                               SDMMC_DCTRL_DBLOCKSIZE_1|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_4096B            (SDMMC_DCTRL_DBLOCKSIZE_2|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_8192B            (SDMMC_DCTRL_DBLOCKSIZE_0| \
                                               SDMMC_DCTRL_DBLOCKSIZE_2|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_DATABLOCK_SIZE_16384B           (SDMMC_DCTRL_DBLOCKSIZE_1| \
                                               SDMMC_DCTRL_DBLOCKSIZE_2|SDMMC_DCTRL_DBLOCKSIZE_3)
#define SDMMC_TRANSFER_DIR_TO_CARD            ((uint32_t)0x00000000U)
#define SDMMC_TRANSFER_DIR_TO_SDMMC            SDMMC_DCTRL_DTDIR

#define SDMMC_TRANSFER_MODE_BLOCK             ((uint32_t)0x00000000U)
#define SDMMC_TRANSFER_MODE_STREAM            SDMMC_DCTRL_DTMODE_1

#define SDMMC_TRANSFER_MODE_BLOCK             ((uint32_t)0x00000000U)
#define SDMMC_TRANSFER_MODE_STREAM            SDMMC_DCTRL_DTMODE_1

#define SDMMC_DPSM_DISABLE                    ((uint32_t)0x00000000U)
#define SDMMC_DPSM_ENABLE                     SDMMC_DCTRL_DTEN

#define DCTRL_CLEAR_MASK         ((uint32_t)(SDMMC_DCTRL_DTEN    | SDMMC_DCTRL_DTDIR |\
                                             SDMMC_DCTRL_DTMODE  | SDMMC_DCTRL_DBLOCKSIZE))


#define HAL_SD_CARD_READY          0x00000001U  /*!< Card state is ready                     */
#define HAL_SD_CARD_IDENTIFICATION 0x00000002U  /*!< Card is in identification state         */
#define HAL_SD_CARD_STANDBY        0x00000003U  /*!< Card is in standby state                */
#define HAL_SD_CARD_TRANSFER       0x00000004U  /*!< Card is in transfer state               */
#define HAL_SD_CARD_SENDING        0x00000005U  /*!< Card is sending an operation            */
#define HAL_SD_CARD_RECEIVING      0x00000006U  /*!< Card is receiving operation information */
#define HAL_SD_CARD_PROGRAMMING    0x00000007U  /*!< Card is in programming state            */
#define HAL_SD_CARD_DISCONNECTED   0x00000008U  /*!< Card is disconnected                    */
#define HAL_SD_CARD_ERROR          0x000000FFU  /*!< Card response Error                     */

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

	static inline void ClearStaticFlags() {
		SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
	}

	static constexpr uint32_t blockSize = 512;

	uint32_t CardType;                     // card Type
	uint32_t CardVersion;                  // card version
	uint32_t Class;                        // class of the card class
	uint32_t RelCardAdd;                   // Relative Card Address
	uint32_t BlockNbr;                     // Card Capacity in blocks
	uint32_t BlockSize;                    // one block size in bytes
	uint32_t LogBlockNbr;                  // Card logical Capacity in blocks
	uint32_t LogBlockSize;                 // logical block size in bytes
	uint32_t CardSpeed;                    // Card Speed

	enum CardState {
		HAL_SD_STATE_RESET        = 0x00000000U,  // SD not yet initialized or disabled
		HAL_SD_STATE_READY        = 0x00000001U,  // SD initialized and ready for use
		HAL_SD_STATE_TIMEOUT      = 0x00000002U,  // SD Timeout state
		HAL_SD_STATE_BUSY         = 0x00000003U,  // SD process ongoing
		HAL_SD_STATE_PROGRAMMING  = 0x00000004U,  // SD Programming State
		HAL_SD_STATE_RECEIVING    = 0x00000005U,  // SD Receiving State
		HAL_SD_STATE_TRANSFER     = 0x00000006U,  // SD Transfer State
		HAL_SD_STATE_ERROR        = 0x0000000FU   // SD is in error state
	} State;           						// SD card State
	uint32_t ErrorCode;						// SD Card Error codes
	uint32_t CID[4];						// Card ID
	uint32_t CSD[4];						// Card Specific Data

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
