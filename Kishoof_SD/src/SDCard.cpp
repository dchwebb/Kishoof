#include "SDCard.h"

SDCard sdCard;

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
	sdmmc_clk = sdmmc_clk / (2U * clockDiv);
	DelayMS(1U + (74U * 1000U / (sdmmc_clk)));

	// Identify card operating voltage
	uint32_t errorstate = PowerON();
	if (errorstate != 0) {
		State = HAL_SD_STATE_READY;
		ErrorCode |= errorstate;
		return false;
	}

	errorstate = InitCard();
	if (errorstate != 0) {
		State = HAL_SD_STATE_READY;
		ErrorCode |= errorstate;
		return false;
	}

	// Set Card Block Size
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearStaticFlags();
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
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);

	do {
		if (count-- == 0) {
			return SDMMC_ERROR_TIMEOUT;
		}
	} while ((SDMMC1->STA & SDMMC_STA_CMDSENT) == 0);

	ClearStaticFlags();
	return SDMMC_ERROR_NONE;
}


uint32_t SDCard::GetCmdResp7()
{
	// Checks for error conditions for R7 response.
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);
	uint32_t sta_reg;

	do {
		if (count-- == 0U) {
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

	return SDMMC_ERROR_NONE;
}


uint32_t SDCard::GetCmdResp2()
{
	//Checks for error conditions for R2 (CID or CSD) response.

	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);
	uint32_t sta_reg;
	do {
		if (count-- == 0U) {
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
		ClearStaticFlags();
	}

	return SDMMC_ERROR_NONE;
}

uint32_t SDCard::GetCmdResp3()
{
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);
	uint32_t sta_reg;

	do {
		if (count-- == 0U) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) == 0) ||
			((sta_reg & SDMMC_STA_CPSMACT) != 0));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {		// Card is not SD V2.0 compliant
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;

	} else {
		ClearStaticFlags();
	}

	return SDMMC_ERROR_NONE;
}


uint32_t SDCard::GetCmdResp6(uint8_t SD_CMD, uint16_t *pRCA)
{
	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);
	uint32_t sta_reg;

	do {
		if (count-- == 0U) {
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

	if (SDMMC1->RESPCMD != SD_CMD) {			// Check response received is of desired command
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	ClearStaticFlags();

	uint32_t response_r1 = SDMMC1->RESP1;

	if ((response_r1 & (SDMMC_R6_GENERAL_UNKNOWN_ERROR | SDMMC_R6_ILLEGAL_CMD | SDMMC_R6_COM_CRC_FAILED)) == 0) {
		*pRCA = (uint16_t)(response_r1 >> 16);
		return SDMMC_ERROR_NONE;
	} else if ((response_r1 & SDMMC_R6_ILLEGAL_CMD) == SDMMC_R6_ILLEGAL_CMD) {
		return SDMMC_ERROR_ILLEGAL_CMD;
	} else if ((response_r1 & SDMMC_R6_COM_CRC_FAILED) == SDMMC_R6_COM_CRC_FAILED) {
		return SDMMC_ERROR_COM_CRC_FAILED;
	} else {
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}


uint32_t SDCard::GetCmdResp1(uint8_t SD_CMD, uint32_t Timeout)
{
	// Checks for error conditions for R1 response.
	uint32_t response_r1;
	uint32_t sta_reg;

	// 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
	uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);

	do {
		if (count-- == 0U) {
			return SDMMC_ERROR_TIMEOUT;
		}
		sta_reg = SDMMC1->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT |
			SDMMC_STA_BUSYD0END)) == 0U) || ((sta_reg & SDMMC_STA_CPSMACT) != 0U));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	ClearStaticFlags();

	if (SDMMC1->RESPCMD != SD_CMD)  {		// Check response received is of desired command
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	response_r1 = SDMMC1->RESP1;

	if ((response_r1 & SDMMC_OCR_ERRORBITS) == SDMMC_ALLZERO)  {
		return SDMMC_ERROR_NONE;
	} else if ((response_r1 & SDMMC_OCR_ADDR_OUT_OF_RANGE) == SDMMC_OCR_ADDR_OUT_OF_RANGE)  {
		return SDMMC_ERROR_ADDR_OUT_OF_RANGE;
	} else if ((response_r1 & SDMMC_OCR_ADDR_MISALIGNED) == SDMMC_OCR_ADDR_MISALIGNED)  {
		return SDMMC_ERROR_ADDR_MISALIGNED;
	} else if ((response_r1 & SDMMC_OCR_BLOCK_LEN_ERR) == SDMMC_OCR_BLOCK_LEN_ERR)  {
		return SDMMC_ERROR_BLOCK_LEN_ERR;
	} else if ((response_r1 & SDMMC_OCR_ERASE_SEQ_ERR) == SDMMC_OCR_ERASE_SEQ_ERR)  {
		return SDMMC_ERROR_ERASE_SEQ_ERR;
	} else if ((response_r1 & SDMMC_OCR_BAD_ERASE_PARAM) == SDMMC_OCR_BAD_ERASE_PARAM)  {
		return SDMMC_ERROR_BAD_ERASE_PARAM;
	} else if ((response_r1 & SDMMC_OCR_WRITE_PROT_VIOLATION) == SDMMC_OCR_WRITE_PROT_VIOLATION)  {
		return SDMMC_ERROR_WRITE_PROT_VIOLATION;
	} else if ((response_r1 & SDMMC_OCR_LOCK_UNLOCK_FAILED) == SDMMC_OCR_LOCK_UNLOCK_FAILED)  {
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	} else if ((response_r1 & SDMMC_OCR_COM_CRC_FAILED) == SDMMC_OCR_COM_CRC_FAILED)  {
		return SDMMC_ERROR_COM_CRC_FAILED;
	} else if ((response_r1 & SDMMC_OCR_ILLEGAL_CMD) == SDMMC_OCR_ILLEGAL_CMD)  {
		return SDMMC_ERROR_ILLEGAL_CMD;
	} else if ((response_r1 & SDMMC_OCR_CARD_ECC_FAILED) == SDMMC_OCR_CARD_ECC_FAILED)  {
		return SDMMC_ERROR_CARD_ECC_FAILED;
	} else if ((response_r1 & SDMMC_OCR_CC_ERROR) == SDMMC_OCR_CC_ERROR)  {
		return SDMMC_ERROR_CC_ERR;
	} else if ((response_r1 & SDMMC_OCR_STREAM_READ_UNDERRUN) == SDMMC_OCR_STREAM_READ_UNDERRUN)  {
		return SDMMC_ERROR_STREAM_READ_UNDERRUN;
	} else if ((response_r1 & SDMMC_OCR_STREAM_WRITE_OVERRUN) == SDMMC_OCR_STREAM_WRITE_OVERRUN)  {
		return SDMMC_ERROR_STREAM_WRITE_OVERRUN;
	} else if ((response_r1 & SDMMC_OCR_CID_CSD_OVERWRITE) == SDMMC_OCR_CID_CSD_OVERWRITE)  {
		return SDMMC_ERROR_CID_CSD_OVERWRITE;
	} else if ((response_r1 & SDMMC_OCR_WP_ERASE_SKIP) == SDMMC_OCR_WP_ERASE_SKIP)  {
		return SDMMC_ERROR_WP_ERASE_SKIP;
	} else if ((response_r1 & SDMMC_OCR_CARD_ECC_DISABLED) == SDMMC_OCR_CARD_ECC_DISABLED)  {
		return SDMMC_ERROR_CARD_ECC_DISABLED;
	} else if ((response_r1 & SDMMC_OCR_ERASE_RESET) == SDMMC_OCR_ERASE_RESET)  {
		return SDMMC_ERROR_ERASE_RESET;
	} else if ((response_r1 & SDMMC_OCR_AKE_SEQ_ERROR) == SDMMC_OCR_AKE_SEQ_ERROR)  {
		return SDMMC_ERROR_AKE_SEQ_ERR;
	} else  {
		return SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
	}
}


uint32_t SDCard::CmdGoIdleState()
{
	sdCmd.Argument         = 0U;
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
	volatile uint32_t count = 0U;
	uint32_t response = 0U;
	uint32_t validvoltage = 0U;
	uint32_t errorstate;


	// CMD0: GO_IDLE_STATE
	errorstate = CmdGoIdleState();
	if (errorstate) {
		return errorstate;
	}

	// CMD8: SEND_IF_COND: Command available only on V2.0 cards
	if (CmdOperCond() == SDMMC_ERROR_TIMEOUT) {		// No response to CMD8
		CardVersion = CARD_V1_X;

		errorstate = CmdGoIdleState();				// CMD0: GO_IDLE_STATE
		if (errorstate != SDMMC_ERROR_NONE) {
			return errorstate;
		}

	} else {
		CardVersion = CARD_V2_X;
	}

	if (CardVersion == CARD_V2_X) {
		errorstate = CmdAppCommand(0);		// SEND CMD55 APP_CMD with RCA as 0
		if (errorstate) {
			return SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}
	}

	// Send ACMD41 SD_APP_OP_COND with Argument 0x80100000
	while ((count < SDMMC_MAX_VOLT_TRIAL) && (validvoltage == 0U)) {

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

		validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);		// Get operating voltage

		count++;
	}

	if (count >= SDMMC_MAX_VOLT_TRIAL) {
		return SDMMC_ERROR_INVALID_VOLTRANGE;
	}

	CardType = CARD_SDSC;		// Set default card type

	if ((response & SDMMC_HIGH_CAPACITY) == SDMMC_HIGH_CAPACITY) {
		CardType = CARD_SDHC_SDXC;
	}

	return SDMMC_ERROR_NONE;
}


uint32_t SDCard::InitCard()
{
	uint32_t errorstate;
	uint16_t sd_rca = 0U;
	uint32_t tickstart = SysTickVal;

	// Check the power State
	if ((SDMMC1->POWER & SDMMC_POWER_PWRCTRL) == 0U) {		// Power off
		return SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
	}

	if (CardType != CARD_SECURED) {
		errorstate = CmdSendCID();		// Send CMD2 ALL_SEND_CID
		if (errorstate != SDMMC_ERROR_NONE) {
			return errorstate;
		}

		// Get Card identification number data
		CID[0] = SDMMC1->RESP1;
		CID[1] = SDMMC1->RESP2;
		CID[2] = SDMMC1->RESP3;
		CID[3] = SDMMC1->RESP4;

		// Send CMD3 SET_REL_ADDR with argument 0; SD Card publishes its RCA.
		while (sd_rca == 0U) {
			errorstate = CmdSetRelAdd(&sd_rca);
			if (errorstate != SDMMC_ERROR_NONE) {
				return errorstate;
			}
			if ((SysTickVal - tickstart) >= SDMMC_CMDTIMEOUT) {
				return SDMMC_ERROR_TIMEOUT;
			}
		}

		RelCardAdd = sd_rca;		// Get the SD card RCA

		// Send CMD9 SEND_CSD with argument as card's RCA
		errorstate = CmdSendCSD(RelCardAdd << 16U);
		if (errorstate != SDMMC_ERROR_NONE) {
			return errorstate;
		}

		// Get Card Specific Data
		CSD[0] = SDMMC1->RESP1;
		CSD[1] = SDMMC1->RESP2;
		CSD[2] = SDMMC1->RESP3;
		CSD[3] = SDMMC1->RESP4;
	}


	Class = (SDMMC1->RESP2 >> 20U);		// Get the Card Class

	if (GetCardCSD() != 0) {	// Get CSD parameters
		return SDMMC_ERROR_UNSUPPORTED_FEATURE;
	}

	// Select the Card
	errorstate = CmdSelDesel(RelCardAdd << 16);
	if (errorstate != SDMMC_ERROR_NONE) {
		return errorstate;
	}

	// All cards are initialized
	return SDMMC_ERROR_NONE;
}


uint32_t SDCard::GetCardCSD()
{
	// Parse the raw CSD data - this does not actually seem to be used except to store some block info
	parsedCSD.CSDStruct = 		(uint8_t)((CSD[0] & 0xC0000000U) >> 30U);
	parsedCSD.SysSpecVersion = 	(uint8_t)((CSD[0] & 0x3C000000U) >> 26U);
	parsedCSD.Reserved1 = 		(uint8_t)((CSD[0] & 0x03000000U) >> 24U);
	parsedCSD.TAAC = 			(uint8_t)((CSD[0] & 0x00FF0000U) >> 16U);
	parsedCSD.NSAC = 			(uint8_t)((CSD[0] & 0x0000FF00U) >> 8U);
	parsedCSD.MaxBusClkFrec = 	(uint8_t)(CSD[0] & 0x000000FFU);
	parsedCSD.CardComdClasses = (uint16_t)((CSD[1] & 0xFFF00000U) >> 20U);
	parsedCSD.RdBlockLen = 		(uint8_t)((CSD[1] & 0x000F0000U) >> 16U);
	parsedCSD.PartBlockRead   = (uint8_t)((CSD[1] & 0x00008000U) >> 15U);
	parsedCSD.WrBlockMisalign = (uint8_t)((CSD[1] & 0x00004000U) >> 14U);
	parsedCSD.RdBlockMisalign = (uint8_t)((CSD[1] & 0x00002000U) >> 13U);
	parsedCSD.DSRImpl = 		(uint8_t)((CSD[1] & 0x00001000U) >> 12U);
	parsedCSD.Reserved2 = 0U;

	if (CardType == CARD_SDSC) {
		parsedCSD.DeviceSize = (((CSD[1] & 0x000003FFU) << 2U) | ((CSD[2] & 0xC0000000U) >> 30U));
		parsedCSD.MaxRdCurrentVDDMin = (uint8_t)((CSD[2] & 0x38000000U) >> 27U);
		parsedCSD.MaxRdCurrentVDDMax = (uint8_t)((CSD[2] & 0x07000000U) >> 24U);
		parsedCSD.MaxWrCurrentVDDMin = (uint8_t)((CSD[2] & 0x00E00000U) >> 21U);
		parsedCSD.MaxWrCurrentVDDMax = (uint8_t)((CSD[2] & 0x001C0000U) >> 18U);
		parsedCSD.DeviceSizeMul = (uint8_t)((CSD[2] & 0x00038000U) >> 15U);

		BlockNbr  = (parsedCSD.DeviceSize + 1U) ;
		BlockNbr *= (1UL << ((parsedCSD.DeviceSizeMul & 0x07U) + 2U));
		BlockSize = (1UL << (parsedCSD.RdBlockLen & 0x0FU));

		LogBlockNbr = (BlockNbr) * ((BlockSize) / 512U);
		LogBlockSize = 512U;
	} else if (CardType == CARD_SDHC_SDXC) {
		parsedCSD.DeviceSize = (((CSD[1] & 0x0000003FU) << 16U) | ((CSD[2] & 0xFFFF0000U) >> 16U));

		BlockNbr = ((parsedCSD.DeviceSize + 1U) * 1024U);
		LogBlockNbr = BlockNbr;
		BlockSize = 512U;
		LogBlockSize = BlockSize;
	} else {
		ClearStaticFlags();
		ErrorCode |= SDMMC_ERROR_UNSUPPORTED_FEATURE;
		State = HAL_SD_STATE_READY;
		return 1;
	}

	parsedCSD.EraseGrSize = (uint8_t)((CSD[2] & 0x00004000U) >> 14U);
	parsedCSD.EraseGrMul = (uint8_t)((CSD[2] & 0x00003F80U) >> 7U);
	parsedCSD.WrProtectGrSize = (uint8_t)(CSD[2] & 0x0000007FU);
	parsedCSD.WrProtectGrEnable = (uint8_t)((CSD[3] & 0x80000000U) >> 31U);
	parsedCSD.ManDeflECC = (uint8_t)((CSD[3] & 0x60000000U) >> 29U);
	parsedCSD.WrSpeedFact = (uint8_t)((CSD[3] & 0x1C000000U) >> 26U);
	parsedCSD.MaxWrBlockLen = (uint8_t)((CSD[3] & 0x03C00000U) >> 22U);
	parsedCSD.WriteBlockPaPartial = (uint8_t)((CSD[3] & 0x00200000U) >> 21U);
	parsedCSD.Reserved3 = 0;
	parsedCSD.ContentProtectAppli = (uint8_t)((CSD[3] & 0x00010000U) >> 16U);
	parsedCSD.FileFormatGroup = (uint8_t)((CSD[3] & 0x00008000U) >> 15U);
	parsedCSD.CopyFlag = (uint8_t)((CSD[3] & 0x00004000U) >> 14U);
	parsedCSD.PermWrProtect = (uint8_t)((CSD[3] & 0x00002000U) >> 13U);
	parsedCSD.TempWrProtect = (uint8_t)((CSD[3] & 0x00001000U) >> 12U);
	parsedCSD.FileFormat = (uint8_t)((CSD[3] & 0x00000C00U) >> 10U);
	parsedCSD.ECC = (uint8_t)((CSD[3] & 0x00000300U) >> 8U);
	parsedCSD.CSD_CRC = (uint8_t)((CSD[3] & 0x000000FEU) >> 1U);
	parsedCSD.Reserved4 = 1;

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
		ClearStaticFlags();
		ErrorCode |= errorstate;
		State = HAL_SD_STATE_READY;
		status = 1;
	} else {
		pStatus->DataBusWidth = (uint8_t)((sd_status[0] & 0xC0U) >> 6U);
		pStatus->SecuredMode = (uint8_t)((sd_status[0] & 0x20U) >> 5U);
		pStatus->CardType = (uint16_t)(((sd_status[0] & 0x00FF0000U) >> 8U) | ((sd_status[0] & 0xFF000000U) >> 24U));
		pStatus->ProtectedAreaSize = (((sd_status[1] & 0xFFU) << 24U)    | ((sd_status[1] & 0xFF00U) << 8U) |
				((sd_status[1] & 0xFF0000U) >> 8U) | ((sd_status[1] & 0xFF000000U) >> 24U));
		pStatus->SpeedClass = (uint8_t)(sd_status[2] & 0xFFU);
		pStatus->PerformanceMove = (uint8_t)((sd_status[2] & 0xFF00U) >> 8U);
		pStatus->AllocationUnitSize = (uint8_t)((sd_status[2] & 0xF00000U) >> 20U);
		pStatus->EraseSize = (uint16_t)(((sd_status[2] & 0xFF000000U) >> 16U) | (sd_status[3] & 0xFFU));
		pStatus->EraseTimeout = (uint8_t)((sd_status[3] & 0xFC00U) >> 10U);
		pStatus->EraseOffset = (uint8_t)((sd_status[3] & 0x0300U) >> 8U);
		pStatus->UhsSpeedGrade = (uint8_t)((sd_status[3] & 0x00F0U) >> 4U);
		pStatus->UhsAllocationUnitSize = (uint8_t)(sd_status[3] & 0x000FU) ;
		pStatus->VideoSpeedClass = (uint8_t)((sd_status[4] & 0xFF000000U) >> 24U);
	}

	// Set Block Size for Card
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearStaticFlags();
		ErrorCode |= errorstate;
		State = HAL_SD_STATE_READY;
		status = 1;
	}


	return status;
}


uint32_t SDCard::SendSDStatus(uint32_t *pSDstatus)
{
	SDMMC_DataInitTypeDef config;
	uint32_t errorstate;
	uint32_t tickstart = SysTickVal;
	uint32_t count;
	uint32_t *pData = pSDstatus;

	// Check SD response
	if ((SDMMC1->RESP1 & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED) {
		return SDMMC_ERROR_LOCK_UNLOCK_FAILED;
	}

	// Set block size for card if it is not equal to current block size for card
	errorstate = CmdBlockLength(64);
	if (errorstate != 0) {
		ErrorCode |= errorstate;
		return errorstate;
	}

	// Send CMD55
	errorstate = CmdAppCommand(RelCardAdd << 16U);
	if (errorstate != 0) {
		ErrorCode |= errorstate;
		return errorstate;
	}
	// Configure the SD DPSM (Data Path State Machine)
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 64U;
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
			for (count = 0U; count < 8U; count++) {
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

	while (SDMMC1->STA & SDMMC_STA_DPSMACT) {
		*pData = SDMMC1->FIFO;
		pData++;

		if ((SysTickVal - tickstart) >= SDMMC_DATATIMEOUT) {
			return SDMMC_ERROR_TIMEOUT;
		}
	}

	ClearStaticFlags();

	return 0;
}


uint32_t SDCard::ConfigWideBusOperation()
{
	uint32_t errorstate;
	uint32_t error = 0;

	State = HAL_SD_STATE_BUSY;

	errorstate = WideBus_Enable();
	ErrorCode |= errorstate;

	if (ErrorCode != 0) {
		ClearStaticFlags();
		error = 1;
	} else {
		// Configure the SDMMC peripheral - FIXME may want to set Clock save power here

		// Set the clock divider to full speed
		SDMMC1->CLKCR = SDMMC_CLKCR_WIDBUS_0;			// 01: 4-bit wide bus mode: SDMMC_D[3:0] used
	}

	// Set Block Size for Card
	errorstate = CmdBlockLength(blockSize);
	if (errorstate != 0) {
		ClearStaticFlags();
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


uint32_t SDCard::FindSCR(uint32_t *pSCR)
{
	uint32_t errorstate;
	uint32_t tickstart = SysTickVal;
	uint32_t tempscr[2] = {0, 0};
	uint32_t *scr = pSCR;

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
	config.DataLength    = 8U;
	config.DataBlockSize = SDMMC_DATABLOCK_SIZE_8B;
	config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
	config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDMMC_DPSM_ENABLE;
	ConfigData(&config);

	errorstate = CmdSendSCR();	// Send ACMD51 SD_APP_SEND_SCR with argument as 0
	if (errorstate != 0)  {
		return errorstate;
	}

	uint32_t index = 0;
	while ((SDMMC1->STA & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND)) == 0) {

		if ((SDMMC1->STA & SDMMC_STA_RXFIFOE) == 0 && (index == 0U)) {
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
	} else  {

		ClearStaticFlags();

		*scr = (((tempscr[1] & SDMMC_0TO7BITS) << 24)  | ((tempscr[1] & SDMMC_8TO15BITS) << 8) | \
				((tempscr[1] & SDMMC_16TO23BITS) >> 8) | ((tempscr[1] & SDMMC_24TO31BITS) >> 24));
		scr++;
		*scr = (((tempscr[0] & SDMMC_0TO7BITS) << 24)  | ((tempscr[0] & SDMMC_8TO15BITS) << 8) | \
				((tempscr[0] & SDMMC_16TO23BITS) >> 8) | ((tempscr[0] & SDMMC_24TO31BITS) >> 24));

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



