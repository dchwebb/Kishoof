#include "SDCard.h"

SDCard sdCard;

static uint32_t SDMMC_GetCmdError()
{
  // 8 is the number of required instructions cycles for loop.  SDMMC_CMDTIMEOUT in ms
  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / 1000U);

  do {
	  if (count-- == 0) {
		  return SDMMC_ERROR_TIMEOUT;
	  }
  } while ((SDMMC1->STA & SDMMC_STA_CMDSENT) == 0);

  SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;  // Clear all the static flags
  return SDMMC_ERROR_NONE;
}


uint32_t SDMMC_GetCmdResp7()
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





uint32_t SDMMC_GetCmdResp1(SDMMC_TypeDef *SDMMCx, uint8_t SD_CMD, uint32_t Timeout)
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
		sta_reg = SDMMCx->STA;
	} while (((sta_reg & (SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT |
			SDMMC_STA_BUSYD0END)) == 0U) || ((sta_reg & SDMMC_STA_CPSMACT) != 0U));

	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) != 0) {
		SDMMC1->ICR = SDMMC_STA_CTIMEOUT;
		return SDMMC_ERROR_CMD_RSP_TIMEOUT;
	} else if ((SDMMC1->STA & SDMMC_STA_CCRCFAIL) != 0) {
		SDMMC1->ICR = SDMMC_STA_CCRCFAIL;
		return SDMMC_ERROR_CMD_CRC_FAIL;
	}

	SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;  	// Clear all the static flags

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

	return SDMMC_GetCmdError();
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

	return SDMMC_GetCmdResp7();
}


//	Verify that that the next command is an application specific com-mand rather than a standard command
uint32_t SDCard::CmdAppCommand(uint32_t argument)
{
	sdCmd.Argument         = argument;
	sdCmd.CmdIndex         = SDMMC_CMD_APP_CMD;
	sdCmd.Response         = SDMMC_RESPONSE_SHORT;
	sdCmd.WaitForInterrupt = 0;
	sdCmd.CPSM             = SDMMC_CMD_CPSMEN;
	sdCmd.Send();

	// If error is a MMC card; otherwise SD card: SD card 2.0 (voltage range mismatch) or SD card 1.x
	return SDMMC_GetCmdResp1(SDMMCx, SDMMC_CMD_APP_CMD, SDMMC_CMDTIMEOUT);
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
		errorstate = SDMMC_CmdAppOperCommand(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY);
		if (errorstate) {
			return SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}

		// Get command response
		response = SDMMC_GetResponse(SDMMC_RESP1);

		// Get operating voltage
		validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);

		count++;
	}

	if (count >= SDMMC_MAX_VOLT_TRIAL) {
		return SDMMC_ERROR_INVALID_VOLTRANGE;
	}

	CardType = CARD_SDSC;	// Set default card type

	if ((response & SDMMC_HIGH_CAPACITY) == SDMMC_HIGH_CAPACITY) {
		CardType = CARD_SDHC_SDXC;
	}

	return SDMMC_ERROR_NONE;
}
