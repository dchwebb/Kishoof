#include "SDCard.h"
#include <bit>

SDCard sdCard;


static inline uint32_t ByteSwap(uint32_t val) {
	return ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) | ((val & 0xFF0000) >> 8) | ((val & 0xFF000000) >> 24);
}

bool SDCard::Init()
{
	if (cardDetect.IsHigh()) {	// Check card is inserted
		return false;
	}

	static constexpr uint32_t SD_INIT_FREQ = 400000;
	uint32_t sdmmc_clk = 50000000;
	uint32_t clockDiv = sdmmc_clk / (2U * SD_INIT_FREQ);
	SDMMC1->CLKCR |= clockDiv;

	SDMMC1->POWER |= SDMMC_POWER_PWRCTRL;			// Power-on: the card is clocked

	// Wait 74 Cycles: required power up waiting time before starting the SD initialization sequence
	sdmmc_clk = sdmmc_clk / (2 * clockDiv);
	DelayMS(1 + (74 * 1000 / sdmmc_clk));

	// Identify card operating voltage
	uint32_t errorstate = PowerON();
	if (errorstate != 0) {
		State = HAL_SD_STATE_READY;
		ErrorCode |= errorstate;
		return false;
	}

	errorstate = InitCard();			// Get the Card ID (CID) Card-Specific Data (CSD)
	if (errorstate != 0) {
		State = HAL_SD_STATE_READY;
		ErrorCode |= errorstate;
		return false;
	}

	// Set Card Block Size
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearAllStaticFlags();
		State = HAL_SD_STATE_READY;
		ErrorCode |= errorstate;
		return false;
	}

	HAL_SD_CardStatusTypeDef CardStatus;
	if (GetCardStatus(&CardStatus)) {
		return false;
	}

	uint32_t speedgrade = CardStatus.UhsSpeedGrade;
	uint32_t unitsize = CardStatus.UhsAllocationUnitSize;
	if ((CardType == CARD_SDHC_SDXC) && (speedgrade || unitsize)) {
		CardSpeed = CARD_ULTRA_HIGH_SPEED;
	} else {
		if (CardType == CARD_SDHC_SDXC) {
			CardSpeed  = CARD_HIGH_SPEED;
		} else {
			CardSpeed  = CARD_NORMAL_SPEED;
		}
	}


	// Configure wide bus (4 data bit mode)
	if (ConfigWideBusOperation() != 0) {
		return false;
	}

	// Verify that SD card is ready to use after Initialization
	uint32_t tickstart = SysTickVal;
	while ((GetCardState() != HAL_SD_CARD_TRANSFER)) {
		if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
			ErrorCode = SDMMC_ERROR_TIMEOUT;
			State = HAL_SD_STATE_READY;
			return false;
		}
	}

	ErrorCode = 0;
	State = HAL_SD_STATE_READY;

	return true;
}



uint32_t SDCard::GetCmdError()
{
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000);

	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
	} while ((SDMMC1->STA & SDMMC_STA_CMDSENT) == 0);

	ClearStaticCmdFlags();
	return 0;
}


uint32_t SDCard::GetCmdResp7()
{
	// Checks for error conditions for R7 response.
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000);
	uint32_t sta_reg;
	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {		// Card is not SD V2.0 compliant
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;

	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {		// Card is not SD V2.0 compliant
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	if ((SDMMC1->STA & SDMMC_STA_CMDREND) != 0) {
		SDMMC1->ICR = SDMMC_STA_CMDREND;		// Card is SD V2.0 compliant
	}

	return 0;
}


uint32_t SDCard::GetCmdResp2()
{
	//Checks for error conditions for R2 (CID or CSD) response.

	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000);
	uint32_t sta_reg;
	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;

	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;

	} else {
		ClearStaticCmdFlags();
	}

	return 0;
}

uint32_t SDCard::GetCmdResp3()
{
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000);
	uint32_t sta_reg;
	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {		// Card is not SD V2.0 compliant
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;

	} else {
		ClearStaticCmdFlags();
	}

	return 0;
}


uint32_t SDCard::GetCmdResp6(uint8_t SD_CMD, uint16_t *pRCA)
{
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000);
	uint32_t sta_reg;
	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));


	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;

	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	ClearStaticCmdFlags();

	if (SDMMC1->RESPCMD != SD_CMD) {			// Check response received is of desired command
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	uint32_t response_r1 = SDMMC1->RESP1;

	if ((response_r1 & (SDMMC_R6_GENERAL_UNKNOWN_ERROR | SDMMC_R6_ILLEGAL_CMD | SDMMC_R6_COM_CRC_FAILED)) == 0) {
		*pRCA = (uint16_t)(response_r1 >> 16);
		return 0;
	} else if ((response_r1 & SDMMC_R6_ILLEGAL_CMD) == SDMMC_R6_ILLEGAL_CMD) {
		return SDMMC_ERROR_ILLEGAL_CMD;
	} else if ((response_r1 & SDMMC_R6_COM_CRC_FAILED) == SDMMC_R6_COM_CRC_FAILED) {
		return SDMMC_ERROR_COM_CRC_FAILED;
	} else {
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}


uint32_t SDCard::GetCmdResp1(uint8_t SD_CMD, uint32_t timeout)
{
	// Checks for error conditions for R1 response.
	uint32_t sta_reg;

	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8 / 1000);

	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT |	SDMMC_STA_BUSYD0END)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	ClearStaticCmdFlags();

	if (SDMMC1->RESPCMD != SD_CMD) {		// Check response received is of desired command
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	uint32_t response_r1 = SDMMC1->RESP1;

	if ((response_r1 & SDMMC_OCR_ERRORBITS) == 0) {
		return 0;
	} else if ((response_r1 & SDMMC_OCR_ADDR_OUT_OF_RANGE) == SDMMC_OCR_ADDR_OUT_OF_RANGE) {
		return SDMMC_ERROR_ADDR_OUT_OF_RANGE;
	} else if ((response_r1 & SDMMC_OCR_ADDR_MISALIGNED) == SDMMC_OCR_ADDR_MISALIGNED) {
		return SDMMC_ERROR_ADDR_MISALIGNED;
	} else if ((response_r1 & SDMMC_OCR_BLOCK_LEN_ERR) == SDMMC_OCR_BLOCK_LEN_ERR) {
		return SDMMC_ERROR_BLOCK_LEN_ERR;
	} else if ((response_r1 & SDMMC_OCR_ERASE_SEQ_ERR) == SDMMC_OCR_ERASE_SEQ_ERR) {
		return SDMMC_ERROR_ERASE_SEQ_ERR;
	} else if ((response_r1 & SDMMC_OCR_BAD_ERASE_PARAM) == SDMMC_OCR_BAD_ERASE_PARAM) {
		return SDMMC_ERROR_BAD_ERASE_PARAM;
	} else if ((response_r1 & SDMMC_OCR_WRITE_PROT_VIOLATION) == SDMMC_OCR_WRITE_PROT_VIOLATION) {
		return SDMMC_ERROR_WRITE_PROT_VIOLATION;
	} else if ((response_r1 & SDMMC_OCR_LOCK_UNLOCK_FAILED) == SDMMC_OCR_LOCK_UNLOCK_FAILED) {
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	} else if ((response_r1 & SDMMC_OCR_COM_CRC_FAILED) == SDMMC_OCR_COM_CRC_FAILED) {
		return SDMMC_ERROR_COM_CRC_FAILED;
	} else if ((response_r1 & SDMMC_OCR_ILLEGAL_CMD) == SDMMC_OCR_ILLEGAL_CMD) {
		return SDMMC_ERROR_ILLEGAL_CMD;
	} else if ((response_r1 & SDMMC_OCR_CARD_ECC_FAILED) == SDMMC_OCR_CARD_ECC_FAILED) {
		return SDMMC_ERROR_CARD_ECC_FAILED;
	} else if ((response_r1 & SDMMC_OCR_CC_ERROR) == SDMMC_OCR_CC_ERROR) {
		return SDMMC_ERROR_CC_ERR;
	} else if ((response_r1 & SDMMC_OCR_STREAM_READ_UNDERRUN) == SDMMC_OCR_STREAM_READ_UNDERRUN) {
		return SDMMC_ERROR_STREAM_READ_UNDERRUN;
	} else if ((response_r1 & SDMMC_OCR_STREAM_WRITE_OVERRUN) == SDMMC_OCR_STREAM_WRITE_OVERRUN) {
		return SDMMC_ERROR_STREAM_WRITE_OVERRUN;
	} else if ((response_r1 & SDMMC_OCR_CID_CSD_OVERWRITE) == SDMMC_OCR_CID_CSD_OVERWRITE) {
		return SDMMC_ERROR_CID_CSD_OVERWRITE;
	} else if ((response_r1 & SDMMC_OCR_WP_ERASE_SKIP) == SDMMC_OCR_WP_ERASE_SKIP) {
		return SDMMC_ERROR_WP_ERASE_SKIP;
	} else if ((response_r1 & SDMMC_OCR_CARD_ECC_DISABLED) == SDMMC_OCR_CARD_ECC_DISABLED) {
		return SDMMC_ERROR_CARD_ECC_DISABLED;
	} else if ((response_r1 & SDMMC_OCR_ERASE_RESET) == SDMMC_OCR_ERASE_RESET) {
		return SDMMC_ERROR_ERASE_RESET;
	} else if ((response_r1 & SDMMC_OCR_AKE_SEQ_ERROR) == SDMMC_OCR_AKE_SEQ_ERROR) {
		return SDMMC_ERROR_AKE_SEQ_ERR;
	} else {
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}


uint32_t SDCard::CmdGoIdleState()
{
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_GO_IDLE_STATE;
	sdCmd.Response         = 0;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	return GetCmdError();
}


// Send the Operating Condition command and check the response.
uint32_t SDCard::CmdOperCond()
{
	// Send CMD8 to verify SD card interface operating condition
	// Argument	[31:12]: Reserved (set to '0')
	// 			[11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
	//			[7:0]: Check Pattern (recommended 0xAA)
	sdCmd.Argument         = SDMMC_CHECK_PATTERN;
	sdCmd.CmdIndex         = SDMMC_CMD_HS_SEND_EXT_CSD;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	return GetCmdResp7();
}


//	Verify that that the next command is an application specific command rather than a standard command
uint32_t SDCard::CmdAppCommand(uint32_t argument)
{
	sdCmd.Argument         = argument;
	sdCmd.CmdIndex         = SDMMC_CMD_APP_CMD;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	// If error is a MMC card; otherwise SD card: SD card 2.0 (voltage range mismatch) or SD card 1.x
	return GetCmdResp1(SDMMC_CMD_APP_CMD, SDMMC_CMDTIMEOUT);
}


// Send the command asking the accessed card to send its operating condition register (OCR)
uint32_t SDCard::CmdAppOperCommand(uint32_t argument)
{
	sdCmd.Argument         = argument;
	sdCmd.CmdIndex         = SDMMC_CMD_SD_APP_OP_COND;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	return GetCmdResp3();
}


// Send the Send CID command and check the response.
uint32_t SDCard::CmdSendCID()
{
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_ALL_SEND_CID;
	sdCmd.Response         = SDMMC_RESPONSE_LONG;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	return GetCmdResp2();
}


uint32_t SDCard::CmdSetRelAdd(uint16_t *rca)
{
	// Send CMD3 SD_CMD_SET_REL_ADDR
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_SET_REL_ADDR;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp6(SDMMC_CMD_SET_REL_ADDR, rca);
}


uint32_t SDCard::CmdSendCSD(uint32_t argument)
{
	sdCmd.Argument         = argument;
	sdCmd.CmdIndex         = SDMMC_CMD_SEND_CSD;
	sdCmd.Response         = SDMMC_RESPONSE_LONG;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp2();
}


uint32_t SDCard::CmdSelDesel(uint32_t addr)
{
	sdCmd.Argument         = addr;
	sdCmd.CmdIndex         = SDMMC_CMD_SEL_DESEL_CARD;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_SEL_DESEL_CARD, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdBlockLength(uint32_t blockSize)
{
	sdCmd.Argument         = blockSize;
	sdCmd.CmdIndex         = SDMMC_CMD_SET_BLOCKLEN;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_SET_BLOCKLEN, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdStatusRegister()
{
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_SD_APP_STATUS;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_SD_APP_STATUS, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdSendSCR()
{
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_SD_APP_SEND_SCR;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_SD_APP_SEND_SCR, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdBusWidth(uint32_t busWidth)
{
	sdCmd.Argument         = busWidth;
	sdCmd.CmdIndex         = SDMMC_CMD_APP_SD_SET_BUSWIDTH;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_APP_SD_SET_BUSWIDTH, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdSendStatus(uint32_t argument)
{
	sdCmd.Argument         = argument;
	sdCmd.CmdIndex         = SDMMC_CMD_SEND_STATUS;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_SEND_STATUS, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdReadSingleBlock(uint32_t readAdd)
{
	sdCmd.Argument         = readAdd;
	sdCmd.CmdIndex         = SDMMC_CMD_READ_SINGLE_BLOCK;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_READ_SINGLE_BLOCK, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdReadMultiBlock(uint32_t readAdd)
{
	sdCmd.Argument         = readAdd;
	sdCmd.CmdIndex         = SDMMC_CMD_READ_MULT_BLOCK;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_READ_MULT_BLOCK, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdWriteSingleBlock(uint32_t writeAdd)
{
	sdCmd.Argument         = writeAdd;
	sdCmd.CmdIndex         = SDMMC_CMD_WRITE_SINGLE_BLOCK;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_WRITE_SINGLE_BLOCK, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdWriteMultiBlock(uint32_t writeAdd)
{
	sdCmd.Argument         = writeAdd;
	sdCmd.CmdIndex         = SDMMC_CMD_WRITE_MULT_BLOCK;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;
	sdCmd.Send();

	return GetCmdResp1(SDMMC_CMD_WRITE_MULT_BLOCK, SDMMC_CMDTIMEOUT);
}


uint32_t SDCard::CmdStopTransfer()
{
	sdCmd.Argument         = 0;
	sdCmd.CmdIndex         = SDMMC_CMD_STOP_TRANSMISSION;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CPSM_ENABLE;

	SDMMC1->CMD |= SDMMC_CMD_CMDSTOP;
	SDMMC1->CMD &= ~SDMMC_CMD_CMDTRANS;

	sdCmd.Send();

	uint32_t errorstate = GetCmdResp1(SDMMC_CMD_STOP_TRANSMISSION, SDMMC_STOPTRANSFERTIMEOUT);

	SDMMC1->CMD &= ~SDMMC_CMD_CMDSTOP;

	// Ignore Address Out Of Range Error, Not relevant at end of memory
	if (errorstate == SDMMC_ERROR_ADDR_OUT_OF_RANGE) {
		errorstate = 0;
	}

	return errorstate;
}


void SDCard::ConfigData(SDMMC_DataInitTypeDef *Data)
{
	SDMMC1->DTIMER = Data->DataTimeOut;		// Set the SDMMC Data TimeOut value
	SDMMC1->DLEN = Data->DataLength;		// Set the SDMMC DataLength value

	// Set the SDMMC data configuration parameters
	uint32_t tmpreg = Data->DataBlockSize | Data->TransferDir | Data->TransferMode | Data->DPSM;

	MODIFY_REG(SDMMC1->DCTRL, DCTRL_CLEAR_MASK, tmpreg);

}

uint32_t SDCard::PowerON()
{
	// Enquires cards about their operating voltage and configures clock controls
	volatile uint32_t count = 0;
	uint32_t response = 0;
	uint32_t errorstate;


	// CMD0: GO_IDLE_STATE
	errorstate = CmdGoIdleState();
	if (errorstate) {
		return errorstate;
	}

	// CMD8: SEND_IF_COND: Command available only on V2.0 cards
	if (CmdOperCond() == SDMMC_ERROR_TIMEOUT) {		// No response to CMD8
		cardVersion = CardVersion::CARD_V1_X;

		errorstate = CmdGoIdleState();				// CMD0: GO_IDLE_STATE
		if (errorstate != 0) {
			return errorstate;
		}

	} else {
		cardVersion = CardVersion::CARD_V2_X;

		errorstate = CmdAppCommand(0);		// SEND CMD55 APP_CMD with RCA as 0
		if (errorstate) {
			return SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}
	}

	// Send ACMD41 SD_APP_OP_COND with Argument 0x80100000
	while ((count < SDMMC_MAX_VOLT_TRIAL)) {

		errorstate = CmdAppCommand(0);		// SEND CMD55 APP_CMD with RCA as 0
		if (errorstate)	{
			return errorstate;
		}

		// Send CMD41
		errorstate = CmdAppOperCommand(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY);
		if (errorstate) {
			return SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}

		response = SDMMC1->RESP1;			// Get command response

		if (response >> 31) {				// Get operating voltage
			break;
		}

		count++;
	}
	if (count >= SDMMC_MAX_VOLT_TRIAL) {
		return SDMMC_ERROR_INVALID_VOLTRANGE;
	}

	CardType = (response & SDMMC_HIGH_CAPACITY) ? CARD_SDHC_SDXC : CARD_SDSC;
	return 0;
}


uint32_t SDCard::InitCard()
{
	uint32_t errorstate;
	uint16_t sd_rca = 0;
	uint32_t tickstart = SysTickVal;

	// Check the power State
	if ((SDMMC1->POWER & SDMMC_POWER_PWRCTRL) == 0) {		// Power off
		return SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
	}

	if (CardType != CARD_SECURED) {
		errorstate = CmdSendCID();		// Send CMD2 ALL_SEND_CID
		if (errorstate != 0) {
			return errorstate;
		}

		// Get Card identification number data
		CID[0] = SDMMC1->RESP1;
		CID[1] = SDMMC1->RESP2;
		CID[2] = SDMMC1->RESP3;
		CID[3] = SDMMC1->RESP4;

		// Send CMD3 SET_REL_ADDR with argument 0; SD Card publishes its RCA
		while (sd_rca == 0) {
			errorstate = CmdSetRelAdd(&sd_rca);
			if (errorstate != 0) {
				return errorstate;
			}
			if ((SysTickVal - tickstart) >= SDMMC_CMDTIMEOUT) {
				return SDMMC_ERROR_TIMEOUT;
			}
		}

		RelCardAdd = sd_rca;			// Store the SD card RCA

		// Send CMD9 SEND_CSD with argument as card's RCA
		errorstate = CmdSendCSD(RelCardAdd << 16);
		if (errorstate != 0) {
			return errorstate;
		}

		// Get Card Specific Data
		CSD[0] = SDMMC1->RESP1;
		CSD[1] = SDMMC1->RESP2;
		CSD[2] = SDMMC1->RESP3;
		CSD[3] = SDMMC1->RESP4;
	}


	Class = (SDMMC1->RESP2 >> 20);		// Get the Card Class

	if (GetCardCSD() != 0) {			// Get Card-Specific Data (CSD) parameters
		return SDMMC_ERROR_UNSUPPORTED_FEATURE;
	}

	// Select the Card
	errorstate = CmdSelDesel(RelCardAdd << 16);
	if (errorstate != 0) {
		return errorstate;
	}

	return 0;
}


uint32_t SDCard::GetCardCSD()
{
	// Parse the raw Card-Specific Data (CSD) data - this does not actually seem to be used except to store some block info
	parsedCSD.CSDStruct = 		((CSD[0] & 0xC0000000) >> 30);
	parsedCSD.SysSpecVersion = 	((CSD[0] & 0x3C000000) >> 26);
	parsedCSD.Reserved1 = 		((CSD[0] & 0x03000000) >> 24);
	parsedCSD.TAAC = 			((CSD[0] & 0x00FF0000) >> 16);
	parsedCSD.NSAC = 			((CSD[0] & 0x0000FF00) >> 8);
	parsedCSD.MaxBusClkFrec = 	(CSD[0] & 0x000000FF);
	parsedCSD.CardComdClasses = ((CSD[1] & 0xFFF00000) >> 20);
	parsedCSD.RdBlockLen = 		((CSD[1] & 0x000F0000) >> 16);
	parsedCSD.PartBlockRead   = ((CSD[1] & 0x00008000) >> 15);
	parsedCSD.WrBlockMisalign = ((CSD[1] & 0x00004000) >> 14);
	parsedCSD.RdBlockMisalign = ((CSD[1] & 0x00002000) >> 13);
	parsedCSD.DSRImpl = 		((CSD[1] & 0x00001000) >> 12);
	parsedCSD.Reserved2 = 0;

	if (CardType == CARD_SDSC) {
		parsedCSD.DeviceSize =         ((CSD[1] & 0x000003FF) << 2) | ((CSD[2] & 0xC0000000) >> 30);
		parsedCSD.MaxRdCurrentVDDMin = ((CSD[2] & 0x38000000) >> 27);
		parsedCSD.MaxRdCurrentVDDMax = ((CSD[2] & 0x07000000) >> 24);
		parsedCSD.MaxWrCurrentVDDMin = ((CSD[2] & 0x00E00000) >> 21);
		parsedCSD.MaxWrCurrentVDDMax = ((CSD[2] & 0x001C0000) >> 18);
		parsedCSD.DeviceSizeMul =      ((CSD[2] & 0x00038000) >> 15);

		BlockNbr  = parsedCSD.DeviceSize + 1;
		BlockNbr *= (1 << ((parsedCSD.DeviceSizeMul & 0x07) + 2));
		BlockSize = (1 << (parsedCSD.RdBlockLen & 0x0F));

		LogBlockNbr = BlockNbr * (BlockSize / 512);
		LogBlockSize = 512;
	} else if (CardType == CARD_SDHC_SDXC) {
		parsedCSD.DeviceSize = (((CSD[1] & 0x0000003F) << 16) | ((CSD[2] & 0xFFFF0000) >> 16));

		BlockNbr = (parsedCSD.DeviceSize + 1) * 1024;
		LogBlockNbr = BlockNbr;
		BlockSize = 512;
		LogBlockSize = BlockSize;
	} else {
		ClearAllStaticFlags();
		ErrorCode |= SDMMC_ERROR_UNSUPPORTED_FEATURE;
		State = HAL_SD_STATE_READY;
		return 1;
	}

	parsedCSD.EraseGrSize =			((CSD[2] & 0x00004000) >> 14);
	parsedCSD.EraseGrMul =			((CSD[2] & 0x00003F80) >> 7);
	parsedCSD.WrProtectGrSize =		(CSD[2] & 0x0000007F);
	parsedCSD.WrProtectGrEnable =	((CSD[3] & 0x80000000) >> 31);
	parsedCSD.ManDeflECC =			((CSD[3] & 0x60000000) >> 29);
	parsedCSD.WrSpeedFact =			((CSD[3] & 0x1C000000) >> 26);
	parsedCSD.MaxWrBlockLen =		((CSD[3] & 0x03C00000) >> 22);
	parsedCSD.WriteBlockPaPartial = ((CSD[3] & 0x00200000) >> 21);
	parsedCSD.Reserved3 =			0;
	parsedCSD.ContentProtectAppli = ((CSD[3] & 0x00010000) >> 16);
	parsedCSD.FileFormatGroup =		((CSD[3] & 0x00008000) >> 15);
	parsedCSD.CopyFlag =			((CSD[3] & 0x00004000) >> 14);
	parsedCSD.PermWrProtect =		((CSD[3] & 0x00002000) >> 13);
	parsedCSD.TempWrProtect =		((CSD[3] & 0x00001000) >> 12);
	parsedCSD.FileFormat =			((CSD[3] & 0x00000C00) >> 10);
	parsedCSD.ECC =					((CSD[3] & 0x00000300) >> 8);
	parsedCSD.CSD_CRC =				((CSD[3] & 0x000000FE) >> 1);
	parsedCSD.Reserved4 =			1;

	return 0;
}


uint32_t SDCard::GetCardStatus(HAL_SD_CardStatusTypeDef *pStatus)
{
	uint32_t sd_status[16];
	uint32_t errorstate;
	uint32_t status = 0;

	if (State == HAL_SD_STATE_BUSY) {
		return 1;
	}

	errorstate = SendSDStatus(sd_status);
	if (errorstate != 0) {
		ClearAllStaticFlags();
		ErrorCode |= errorstate;
		State = HAL_SD_STATE_READY;
		status = 1;
	} else {
		pStatus->DataBusWidth =			((sd_status[0] & 0xC0) >> 6);
		pStatus->SecuredMode =			((sd_status[0] & 0x20) >> 5);
		pStatus->CardType = 			(((sd_status[0] & 0x00FF0000) >> 8) | ((sd_status[0] & 0xFF000000) >> 24));
		pStatus->ProtectedAreaSize =	ByteSwap(sd_status[1]);
		pStatus->SpeedClass =			(sd_status[2] & 0xFF);
		pStatus->PerformanceMove =		((sd_status[2] & 0xFF00) >> 8);
		pStatus->AllocationUnitSize =	((sd_status[2] & 0xF00000) >> 20);
		pStatus->EraseSize =			(((sd_status[2] & 0xFF000000) >> 16) | (sd_status[3] & 0xFF));
		pStatus->EraseTimeout =			((sd_status[3] & 0xFC00) >> 10);
		pStatus->EraseOffset =			((sd_status[3] & 0x0300) >> 8);
		pStatus->UhsSpeedGrade =		((sd_status[3] & 0x00F0) >> 4);
		pStatus->UhsAllocationUnitSize = (sd_status[3] & 0x000F);
		pStatus->VideoSpeedClass =		((sd_status[4] & 0xFF000000) >> 24);
	}

	// Set Block Size
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearAllStaticFlags();
		ErrorCode |= errorstate;
		State = HAL_SD_STATE_READY;
		status = 1;
	}


	return status;
}


uint32_t SDCard::SendSDStatus(uint32_t* pSDstatus)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = SysTickVal;
	uint32_t *pData = pSDstatus;

	// Check SD response
	if ((SDMMC1->RESP1 & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	}

	// Set block size for card if MASK is not equal to current block size for card
	errorstate = CmdBlockLength(64);
	if (errorstate != 0) {
		ErrorCode |= errorstate;
		return errorstate;
	}

	// Send CMD55
	errorstate = CmdAppCommand(RelCardAdd << 16);
	if (errorstate != 0) {
		ErrorCode |= errorstate;
		return errorstate;
	}
	// Configure the SD DPSM (Data Path State Machine)
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 64;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
	config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDMMC_DPSM_ENABLE;
	ConfigData(&config);

	// Send ACMD13 (SD_APP_STAUS)  with argument as card's RCA
	errorstate = CmdStatusRegister();
	if (errorstate != 0) {
		ErrorCode |= errorstate;
		return errorstate;
	}

	// Get status data
	while ((SDMMC1->STA & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) == 0) {
		if (SDMMC1->STA & SDMMC_STA_RXFIFOHF) {
			for (uint32_t count = 0; count < 8; ++count) {
				*pData = SDMMC1->FIFO;
				pData++;
			}
		}

		if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	if (SDMMC1->STA & SDMMC_STA_DTIMEOUT) {
		return SDMMC_ERROR_DATA_TIMEOUT;
	} else if (SDMMC1->STA & SDMMC_STA_DCRCFAIL) {
		return SDMMC_ERROR_DATA_CRC_FAIL;
	} else if (SDMMC1->STA & SDMMC_STA_RXOVERR) {
		return SDMMC_ERROR_RX_OVERRUN;
	}

	// FIXME - not sure what this is for as should never be reached
	while (SDMMC1->STA & SDMMC_STA_DPSMACT) {		// Data path state machine active
		*pData = SDMMC1->FIFO;
		pData++;

		if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	ClearStaticDataFlags();

	return 0;
}


uint32_t SDCard::ConfigWideBusOperation()
{
	uint32_t errorstate;
	uint32_t error = 0;

	State = HAL_SD_STATE_BUSY;

	errorstate = WideBus_Enable();			// Checks if card supports wide bus mode and sets it on the card if so

	if (ErrorCode != 0) {
		ErrorCode |= errorstate;
		ClearAllStaticFlags();
		error = 1;
	} else {
		// Configure the SDMMC peripheral - FIXME may want to set Clock save power here

		// Set the clock divider to full speed and enable wide bus mode
		SDMMC1->CLKCR = SDMMC_CLKCR_WIDBUS_0;			// 01: 4-bit wide bus mode: SDMMC_D[3:0] used
//		SDMMC1->CLKCR |= 1;								// Debug - set the clock divider
	}

	// Set Block Size for Card
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearAllStaticFlags();
		ErrorCode |= errorstate;
		error = 1;
	}

	State = HAL_SD_STATE_READY;
	return error;
}


uint32_t SDCard::WideBus_Enable()
{
	uint32_t scr[2] = {0, 0};
	uint32_t errorstate;

	if ((SDMMC1->RESP1 & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	}

	errorstate = FindSCR(scr);							// Get SD Configuration Register (SCR) Register
	if (errorstate != 0) {
		return errorstate;
	}

	if (scr[1] & SDMMC_WIDE_BUS_SUPPORT) {				// Card supports wide bus operation

		errorstate = CmdAppCommand(RelCardAdd << 16);	// Send CMD55 APP_CMD with argument as card's RCA
		if (errorstate != 0) {
			return errorstate;
		}

		errorstate = CmdBusWidth(2);					// Send ACMD6 APP_CMD with argument as 2 for wide bus mode
		if (errorstate != 0) {
			return errorstate;
		}

		return 0;
	} else {
		return SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
	}
}


uint32_t SDCard::FindSCR(uint32_t *scr)
{
	uint32_t errorstate;
	uint32_t tickstart = SysTickVal;
	uint32_t tempscr[2] = {0, 0};
	//uint32_t *scr = pSCR;

	// Set Block Size To 8 Bytes
	errorstate = CmdBlockLength(8);
	if (errorstate != 0) {
		return errorstate;
	}

	/// Send CMD55 APP_CMD with argument as card's RCA
	errorstate = CmdAppCommand(RelCardAdd << 16);
	if (errorstate != 0) {
		return errorstate;
	}

	SDMMC_DataInitTypeDef config;
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 8;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_8B;
	config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDMMC_DPSM_ENABLE;
	ConfigData(&config);

	errorstate = CmdSendSCR();	// Send ACMD51 SD_APP_SEND_SCR with argument as 0
	if (errorstate != 0) {
		return errorstate;
	}

	uint32_t index = 0;
	while ((SDMMC1->STA & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) == 0) {

		if ((SDMMC1->STA & SDMMC_STA_RXFIFOE) == 0 && (index == 0)) {
			tempscr[0] = SDMMC1->FIFO;
			tempscr[1] = SDMMC1->FIFO;
			index++;
		}

		if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	if (SDMMC1->STA & SDMMC_STA_DTIMEOUT) {
		SDMMC1->ICR = SDMMC_STA_DTIMEOUT;
		return SDMMC_ERROR_DATA_TIMEOUT;
	} else if (SDMMC1->STA & SDMMC_STA_DCRCFAIL) {
		SDMMC1->ICR = SDMMC_STA_DCRCFAIL;
		return SDMMC_ERROR_DATA_CRC_FAIL;
	} else if (SDMMC1->STA & SDMMC_STA_RXOVERR) {
		SDMMC1->ICR = SDMMC_STA_RXOVERR;
		return SDMMC_ERROR_RX_OVERRUN;
	} else {
		ClearStaticDataFlags();
		scr[0] = ByteSwap(tempscr[1]);
		scr[1] = ByteSwap(tempscr[0]);
	}

	return 0;
}

uint32_t SDCard::GetCardState()
{
	uint32_t errorstate = CmdSendStatus(RelCardAdd << 16);
	if (errorstate != 0) {
		ErrorCode |= errorstate;
	}

	return ((SDMMC1->RESP1 >> 9) & 0x0F);
}


uint32_t SDCard::WriteBlocks_DMA(const uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, bool blocking)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t add = BlockAdd;
	dmaWrite = false;			// Interrupt handler will use this to signal completion

	if (State == HAL_SD_STATE_READY) {
		ErrorCode = 0;

		if ((add + NumberOfBlocks) > (LogBlockNbr)) {
			ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return 1;
		}

		State = HAL_SD_STATE_BUSY;

		// Initialize data control register
		SDMMC1->DCTRL = 0;

		pTxBuffPtr = pData;
		TxXferSize = blockSize * NumberOfBlocks;

		if (CardType != CARD_SDHC_SDXC) {
			add *= 512;
		}

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut   = SDMMC_DATATIMEOUT;
		config.DataLength    = blockSize * NumberOfBlocks;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir   = SDMMC_TRANSFER_DIR_TO_CARD;
		config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM          = SDMMC_DPSM_DISABLE;
		ConfigData(&config);

		SDMMC1->CMD |= SDMMC_CMD_CMDTRANS;
		SDMMC1->IDMABASE0 = (uint32_t)pData;
		SDMMC1->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

		if (NumberOfBlocks > 1) {		// Write Blocks in Polling mode
			Context = SD_CONTEXT_WRITE_MULTIPLE_BLOCK | SD_CONTEXT_DMA;
			errorstate = CmdWriteMultiBlock(add);			// Write Multi Block command
		} else {
			Context = (SD_CONTEXT_WRITE_SINGLE_BLOCK | SD_CONTEXT_DMA);
			errorstate = CmdWriteSingleBlock(add);			// Write Single Block command
		}

		if (errorstate != 0) {
			ClearAllStaticFlags();
			ErrorCode |= errorstate;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		}

		if (blocking) {
			// If in blocking mode wait until data end flag and then call interrupt handler directly
			uint32_t tickstart = SysTickVal;

			while ((SDMMC1->STA & SDMMC_STA_DATAEND) == 0) {
				if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
					return SDMMC_ERROR_TIMEOUT;
				}
			}
			InterruptHandler();
		} else {
			// Enable transfer interrupts
			SDMMC1->MASK |= SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_DATAENDIE;
		}

		return 0;
	} else {
		return 2;
	}
}


uint32_t SDCard::ReadBlocks(uint8_t *pData, uint32_t blockAdd, uint32_t blocks, uint32_t timeout)
{

	uint32_t errorstate;
	uint32_t tickstart = SysTickVal;
	uint32_t* tempBuff = (uint32_t*)pData;

	if (State == HAL_SD_STATE_READY) {
		ErrorCode = 0;

		if (blockAdd + blocks > LogBlockNbr) {
			ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return 1;
		}

		State = HAL_SD_STATE_BUSY;

		SDMMC1->DCTRL = 0;		// Initialize data control register

		if (CardType != CARD_SDHC_SDXC) {
			blockAdd *= 512U;
		}

		// Configure the SD DPSM (Data Path State Machine)
		SDMMC_DataInitTypeDef config;
		config.DataTimeOut   = SDMMC_DATATIMEOUT;
		config.DataLength    = blocks * blockSize;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
		config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM          = SDMMC_DPSM_DISABLE;
		ConfigData(&config);
		SDMMC1->CMD |= SDMMC_CMD_CMDTRANS;

		if (blocks > 1) {
			Context = SD_CONTEXT_READ_MULTIPLE_BLOCK;
			errorstate = CmdReadMultiBlock(blockAdd);
		} else {
			Context = SD_CONTEXT_READ_SINGLE_BLOCK;
			errorstate = CmdReadSingleBlock(blockAdd);
		}
		if (errorstate != 0) {
			ClearAllStaticFlags();
			ErrorCode |= errorstate;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		}

		uint32_t dataremaining = config.DataLength;

		while ((SDMMC1->STA & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) == 0) {
			if (SDMMC1->STA & SDMMC_STA_RXFIFOHF && (dataremaining >= 32)) {

				for (uint32_t count = 0; count < 8; count++) {
					*tempBuff++ = SDMMC1->FIFO;
				}
				dataremaining -= 32;
			}

			if (SysTickVal - tickstart >= timeout || timeout == 0) {
				ClearAllStaticFlags();
				ErrorCode |= SDMMC_ERROR_TIMEOUT;
				State = HAL_SD_STATE_READY;
				Context = 0;
				return 3;
			}
		}
		SDMMC1->CMD &= ~SDMMC_CMD_CMDTRANS;

		// Send stop transmission command in case of multiblock read
		if ((SDMMC1->STA & SDMMC_STA_DATAEND) && blocks > 1) {
			if (CardType != CARD_SECURED) {

				errorstate = CmdStopTransfer();				// Send stop transmission command
				if (errorstate != 0) {
					ClearAllStaticFlags();
					ErrorCode |= errorstate;
					State = HAL_SD_STATE_READY;
					Context = 0;
					return 1;
				}
			}
		}

		// Get error state
		if ((SDMMC1->STA & SDMMC_STA_DTIMEOUT)) {
			ClearAllStaticFlags();
			ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		} else if ((SDMMC1->STA & SDMMC_STA_DCRCFAIL)) {
			ClearAllStaticFlags();
			ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		} else if ((SDMMC1->STA & SDMMC_STA_RXOVERR)) {
			ClearAllStaticFlags();
			ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		}

		ClearAllStaticFlags();
		State = HAL_SD_STATE_READY;

		return 0;
	} else {
		ErrorCode |= SDMMC_ERROR_BUSY;
		return 1;
	}
}


uint32_t SDCard::ReadBlocks_DMA(uint8_t *rxBuffer, uint32_t blockAddr, uint32_t blocks, bool blocking, void (*callback)())
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	dmaRead = false;			// Interrupt handler will use this to signal completion

	if (State == HAL_SD_STATE_READY) {
		GpioPin::SetHigh(GPIOG, 11);			 // Debug pin high

		ErrorCode = 0;

		if (blockAddr + blocks > LogBlockNbr) {
			ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return 1;
		}

		State = HAL_SD_STATE_BUSY;

		SDMMC1->DCTRL = 0;		// Initialize data control register

		pRxBuffPtr = rxBuffer;
		RxXferSize = blockSize * blocks;

		if (CardType != CARD_SDHC_SDXC) {
			blockAddr *= 512;
		}

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut   = SDMMC_DATATIMEOUT;
		config.DataLength    = blockSize * blocks;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
		config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM          = SDMMC_DPSM_DISABLE;
		ConfigData(&config);

		SDMMC1->CMD |= SDMMC_CMD_CMDTRANS;
		SDMMC1->IDMABASE0 = (uint32_t)rxBuffer;
		SDMMC1->IDMACTRL  = SDMMC_IDMA_IDMAEN;

		// Read Blocks in DMA mode
		if (blocks > 1) {
			Context = SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA;
			errorstate = CmdReadMultiBlock(blockAddr);			// Read Multi Block command
		} else {
			Context = SD_CONTEXT_READ_SINGLE_BLOCK | SD_CONTEXT_DMA;
			errorstate = CmdReadSingleBlock(blockAddr);			// Read Single Block command
		}
		if (errorstate != 0) {
			ClearStaticDataFlags();
			ErrorCode |= errorstate;
			State = HAL_SD_STATE_READY;
			Context = 0;
			return 1;
		}

		// Store callback if required, for processing in interrupt handler
		if (callback) {
			dmaCallback = callback;
			Context |= SD_CONTEXT_CALLBACK;
		}

		if (blocking) {
			// If in blocking mode wait until data end flag and then call interrupt handler directly
			uint32_t tickstart = SysTickVal;

			while ((SDMMC1->STA & SDMMC_STA_DATAEND) == 0) {
				if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
					return SDMMC_ERROR_TIMEOUT;
				}
			}
			InterruptHandler();
		} else {
			// Enable transfer interrupts
			SDMMC1->MASK |= (SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_DATAENDIE);
		}

		return 0;
	} else {
		return 2;
	}
}



uint32_t SDCard::ReadBlocksDMAMultiBuffer(uint32_t blockAddr, uint32_t blocks, uint32_t* buffer0, uint32_t* buffer1)
{
	//  Reads blocks from specified address and store in Buffer0 and Buffer1.

	if (State == HAL_SD_STATE_READY) {
		if ((blockAddr + blocks) > (LogBlockNbr)) {
			ErrorCode |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return 1;
		}

		SDMMC1->IDMABASE0 = (uint32_t)buffer0;
		SDMMC1->IDMABASE1 = (uint32_t)buffer1;
		SDMMC1->IDMABSIZE = ((blockSize * blocks) / 2);		// Each buffer must be the same size (half size of received data)
		SDMMC1->DCTRL = 0;

		ClearStaticDataFlags();

		ErrorCode = 0;
		State = HAL_SD_STATE_BUSY;

		if (CardType != CARD_SDHC_SDXC) {
			blockAddr *= 512U;
		}

		// Configure the SD DPSM (Data Path State Machine)
		SDMMC_DataInitTypeDef config;
		config.DataTimeOut   = SDMMC_DATATIMEOUT;
		config.DataLength    = blockSize * blocks;
		config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
		config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
		config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
		config.DPSM          = SDMMC_DPSM_DISABLE;
		ConfigData(&config);

		SDMMC1->DCTRL |= SDMMC_DCTRL_FIFORST;
		SDMMC1->CMD |= SDMMC_CMD_CMDTRANS;
		SDMMC1->IDMACTRL = SDMMC_ENABLE_IDMA_DOUBLE_BUFF0;

		// Read Blocks in DMA mode
		Context = (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA);

		// Read Multi Block command
		uint32_t errorstate = CmdReadMultiBlock(blockAddr);
		if (errorstate != 0) {
			State = HAL_SD_STATE_READY;
			ErrorCode |= errorstate;
			return 1;
		}

		SDMMC1->MASK |= (SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_IDMABTCIE);

		return 0;
	} else {
		return 2;
	}

}


void SDCard::InterruptHandler()
{
	uint32_t errorstate;
	uint32_t context = Context;

	// Check for SDMMC interrupt flags
	if ((SDMMC1->STA & SDMMC_STA_DATAEND) != 0) {
		SDMMC1->ICR = SDMMC_ICR_DATAENDC;

		SDMMC1->MASK &= ~(SDMMC_MASK_DATAENDIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE |
				SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_TXFIFOHEIE |
				SDMMC_MASK_RXFIFOHFIE | SDMMC_MASK_IDMABTCIE);
		SDMMC1->CMD &= ~SDMMC_CMD_CMDTRANS;

		if ((context & SD_CONTEXT_DMA) != 0) {
			SDMMC1->DLEN = 0;
			SDMMC1->DCTRL = 0;
			SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;

			// Stop Transfer for Write Multi blocks or Read Multi blocks
			if (((context & SD_CONTEXT_READ_MULTIPLE_BLOCK) != 0) || ((context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0)) {
				errorstate = CmdStopTransfer();
				if (errorstate != 0) {
					ErrorCode |= errorstate;
				}
			}

			if (((context & SD_CONTEXT_WRITE_SINGLE_BLOCK) != 0) || ((context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0)) {
				dmaWrite = true;
			}
			if (((context & SD_CONTEXT_READ_SINGLE_BLOCK) != 0) || ((context & SD_CONTEXT_READ_MULTIPLE_BLOCK) != 0)) {
				GpioPin::SetLow(GPIOG, 11);			 // Debug pin low
				dmaRead = true;
				if (Context & SD_CONTEXT_CALLBACK && dmaCallback) {
					dmaCallback();
				}
			}
			State = HAL_SD_STATE_READY;
			Context = 0;
		}


	} else if ((SDMMC1->STA & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_RXOVERR | SDMMC_STA_TXUNDERR)) != 0) {
		// Error handling
		if ((SDMMC1->STA & SDMMC_STA_DCRCFAIL) != 0) {
			ErrorCode |= SDMMC_ERROR_DATA_CRC_FAIL;
		}
		if ((SDMMC1->STA & SDMMC_STA_DTIMEOUT) != 0) {
			ErrorCode |= SDMMC_ERROR_DATA_TIMEOUT;
		}
		if ((SDMMC1->STA & SDMMC_STA_RXOVERR) != 0) {
			ErrorCode |= SDMMC_ERROR_RX_OVERRUN;
		}
		if ((SDMMC1->STA & SDMMC_STA_TXUNDERR) != 0) {
			ErrorCode |= SDMMC_ERROR_TX_UNDERRUN;
		}
		ClearStaticDataFlags();

		// Disable all interrupts
		SDMMC1->MASK &= ~(SDMMC_MASK_DATAENDIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_RXOVERRIE);

		SDMMC1->CMD &= ~SDMMC_CMD_CMDTRANS;
		SDMMC1->DCTRL |= SDMMC_DCTRL_FIFORST;
		SDMMC1->CMD |= SDMMC_CMD_CMDSTOP;
		ErrorCode |= CmdStopTransfer();
		SDMMC1->CMD &= ~SDMMC_CMD_CMDSTOP;
		SDMMC1->ICR = SDMMC_ICR_DABORTC;

		if ((context & SD_CONTEXT_DMA) != 0) {
			if (ErrorCode != 0) {
				// Disable Internal DMA
				SDMMC1->MASK &= ~SDMMC_MASK_IDMABTCIE;
				SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;
				State = HAL_SD_STATE_READY;
			}
		}

	} else if ((SDMMC1->STA & SDMMC_STA_IDMABTC) != 0) {
		// Double-buffering
		SDMMC1->ICR = SDMMC_ICR_IDMABTCC;

		if ((SDMMC1->IDMACTRL & SDMMC_IDMA_IDMABACT) == 0) {
			// Current buffer is buffer0, Transfer complete for buffer1
			if ((context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0) {
				//HAL_SDEx_Write_DMADoubleBuf1CpltCallback(hsd);
			} else {
				//HAL_SDEx_Read_DMADoubleBuf1CpltCallback(hsd);
			}

		} else {
			// Current buffer is buffer1, Transfer complete for buffer0
			if ((context & SD_CONTEXT_WRITE_MULTIPLE_BLOCK) != 0) {
				//HAL_SDEx_Write_DMADoubleBuf0CpltCallback(hsd);
			} else {
				//HAL_SDEx_Read_DMADoubleBuf0CpltCallback(hsd);
			}
		}
	}
}
