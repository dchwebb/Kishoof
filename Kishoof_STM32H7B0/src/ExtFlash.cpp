#include "ExtFlash.h"

ExtFlash extFlash;

static constexpr bool dtr = false;			// Double data rate

static constexpr uint32_t dummyCycles = 6;	// Configured according to device speed

static constexpr uint32_t dtrCCR = dtr ? (OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_DDTR | OCTOSPI_CCR_DQSE) : OCTOSPI_CCR_DQSE;

static constexpr uint32_t octoCCR =
		OCTOSPI_CCR_ADMODE_2 |				// Address: 100: Eight lines
		OCTOSPI_CCR_ADSIZE |				// Address size: 32 bits
		OCTOSPI_CCR_IMODE_2 |				// Instruction: 100: Eight lines
		OCTOSPI_CCR_ISIZE_0 |				// Instruction size: 16 bits
		OCTOSPI_CCR_DMODE_2 |				// Data: 100: Eight lines
		dtrCCR;								// Double data rate settings if configured



void ExtFlash::Init()
{
	// In DTR mode, it is recommended to set DHQC of OCTOSPI_TCR, to shift the outputs by a quarter of cycle and avoid holding issues on the memory side.
	OCTOSPI1->TCR |= OCTOSPI_TCR_DHQC;		// Delay hold quarter cycle
	Reset();
	SetOctoMode();
	MemoryMapped();
	return;

}


void ExtFlash::SetOctoMode()
{
	MemMappedOff();
	WriteCfg2(0x300, 0b111);								// Set dummy cycles to 6 - minimum for use at 66MHz (see p28)
	WriteCfg2(0x200, dtr ? 0b00 : 0b11);					// DQS on STR mode | DTR DQS pre-cycle [both needed for STR DQS mode]
	WriteCfg2(0x000, dtr ? 0b10 : 0b01);					// Switch to octal mode: 00 = SPI; 01 = STR OPI enable; 10 = DTR OPI enable
	octalMode = true;
}


void ExtFlash::WriteCfg2(uint32_t address, uint32_t data)
{
	WriteEnable();

	OCTOSPI1->DLR = 0;										// Write 1 byte
	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;						// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode

	if (octalMode) {
		OCTOSPI1->CCR = (octoCCR & ~OCTOSPI_CCR_DQSE);
		OCTOSPI1->IR = writeCfgReg2;						// Convert instruction to SPI mode
	} else {
		OCTOSPI1->CCR = (OCTOSPI_CCR_ADMODE_0 |				// Address: 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
						OCTOSPI_CCR_ADSIZE |				// Address size 32 bits
						OCTOSPI_CCR_IMODE_0 |				// Instruction: 000: None; 001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
						OCTOSPI_CCR_DMODE_0);				// Data: 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
		OCTOSPI1->IR = (writeCfgReg2 >> 8);					// Convert instruction to SPI mode
	}

	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = address;									// Set configuration register address
	OCTOSPI1->DR = data;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI

}


void ExtFlash::Reset()
{
	MemMappedOff();
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Set number of dummy cycles
	OCTOSPI1->DLR = 0;										// Return 1 byte
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable QSPI
	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;						// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode

	if (octalMode) {
		OCTOSPI1->CCR = OCTOSPI_CCR_IMODE_2 |				// 100: Eight lines
						OCTOSPI_CCR_ISIZE_0 |				// Instruction size 16 bits
						dtrCCR;

		OCTOSPI1->IR = enableReset;
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		OCTOSPI1->IR = resetDevice;

	} else {
		OCTOSPI1->CCR = OCTOSPI_CCR_IMODE_0;				// 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
		OCTOSPI1->IR = (enableReset >> 8);
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		OCTOSPI1->IR = (resetDevice >> 8);
	}
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable QSPI

	octalMode = false;
}


void ExtFlash::WriteEnable()
{
	MemMappedOff();
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Dummy cycles
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable QSPI
	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;						// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
	if (octalMode) {
		OCTOSPI1->CCR = OCTOSPI_CCR_ISIZE_0 |				// Instruction size 16 bits
						OCTOSPI_CCR_IMODE_2 |				// 100: Eight lines
						dtrCCR;
		OCTOSPI1->IR = writeEnable;
	} else {
		OCTOSPI1->CCR = OCTOSPI_CCR_IMODE_0;				// 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
		OCTOSPI1->IR = (writeEnable >> 8);					// Convert instruction to SPI mode
	}
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable QSPI
}


uint32_t ExtFlash::GetID(bool forceOctalMode)
{
	MemMappedOff();
	OCTOSPI1->DLR = 2;										// Return 3 bytes
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// 00: Indirect write mode; *01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode

	if (forceOctalMode || octalMode) {
		OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles
		OCTOSPI1->TCR |= (4 << OCTOSPI_TCR_DCYC_Pos);		// Set number of dummy cycles
		OCTOSPI1->CCR = octoCCR & ~OCTOSPI_CCR_DDTR;
		OCTOSPI1->IR = getID;								// Set instruction to write enable
		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->AR = 0x0;									// Set address register

	} else {
		OCTOSPI1->CCR = (OCTOSPI_CCR_IMODE_0 |				// Instruction: 000: None; 001: One line; 010: Two lines; 011: Four lines; *100: Eight lines
						OCTOSPI_CCR_DMODE_0);				// Data: 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines

		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->IR = (getID >> 8);						// Convert instruction to SPI mode
	}
	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	const uint32_t ret = OCTOSPI1->DR;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles
	MemoryMapped();
	return ret;
}


uint32_t ExtFlash::ReadCfgReg(uint32_t address)
{
	MemMappedOff();
	OCTOSPI1->FCR = OCTOSPI_FCR_CTCF | OCTOSPI_FCR_CTEF;	// Clear errors
	OCTOSPI1->DLR = 0;										// Return 1 byte
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// 00: Indirect write mode; *01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode

	if (octalMode) {
		OCTOSPI1->TCR |= (4 << OCTOSPI_TCR_DCYC_Pos);		// Set number of dummy cycles
		OCTOSPI1->CCR = (octoCCR & ~OCTOSPI_CCR_DDTR);
		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->IR = readCfgReg2;							// Set instruction register
	} else {
		OCTOSPI1->CCR = (OCTOSPI_CCR_ADMODE_0 |				// Address: 000: None; 001: One line; 010: Two lines; 011: Four lines; *100: Eight lines
			OCTOSPI_CCR_ADSIZE |							// Address size 32 bits
			OCTOSPI_CCR_DMODE_0 |							// Data: 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines
			OCTOSPI_CCR_IMODE_0);
		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->IR = (readCfgReg2 >> 8);					// Set instruction register
	}
	OCTOSPI1->AR = address;									// Set address register

	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	const uint32_t ret = OCTOSPI1->DR;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles
	return ret;
}


uint32_t ExtFlash::ReadStatusReg()
{
	MemMappedOff();
	OCTOSPI1->DLR = 1;										// Return 2 bytes
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// 00: Indirect write mode; *01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode

	if (octalMode) {
		OCTOSPI1->TCR |= (4 << OCTOSPI_TCR_DCYC_Pos);		// Set number of dummy cycles
		OCTOSPI1->CCR = octoCCR;
		OCTOSPI1->IR = readStatusReg;						// Set address register
		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->AR = 0;									// Set address register
	} else {
		OCTOSPI1->CCR = (OCTOSPI_CCR_IMODE_0 |				// 001: One line
						OCTOSPI_CCR_DMODE_0);				// 001: One line

		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->IR = (readStatusReg >> 8);				// Set address register
	}
	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	const uint32_t ret = OCTOSPI1->DR;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles
	return ret;
}


uint32_t ExtFlash::ReadReg(uint32_t address)
{
	MemMappedOff();
	OCTOSPI1->DLR = 0;										// Return 1 byte
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// 00: Indirect write mode; *01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
	OCTOSPI1->CCR = (OCTOSPI_CCR_ADMODE_0 |					// Address: 000: None; 001: One line; 010: Two lines; 011: Four lines; *100: Eight lines
					OCTOSPI_CCR_DMODE_0);					// Data: 000: None; *001: One line; 010: Two lines; 011: Four lines; 100: Eight lines

	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = address;									// Set address register

	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	const uint32_t ret = OCTOSPI1->DR;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
	return ret;
}


uint32_t ExtFlash::Read(uint32_t address)
{
	MemMappedOff();
	OCTOSPI1->DLR = 3;										// Read 4 bytes
	OCTOSPI1->TCR |= (dummyCycles << OCTOSPI_TCR_DCYC_Pos);	// Set number of dummy cycles
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// 00: Indirect write mode; *01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
	OCTOSPI1->CCR = octoCCR;
	OCTOSPI1->IR = dtr ? octaReadDTR : octaRead;
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = address;									// Set address register

	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	const uint32_t ret = OCTOSPI1->DR;
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles
	return ret;

}


void ExtFlash::Write(uint32_t address, uint32_t* val)
{
	// Writes 256 bytes at a time
	WriteEnable();

	OCTOSPI1->DLR = 255;									// Write 256 bytes
	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;						// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Dummy cycles
	OCTOSPI1->CCR = octoCCR;
	OCTOSPI1->IR = writePage;
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = address;									// Set address register

	for (uint32_t i = 0; i < 64; ++i) {
		while ((OCTOSPI1->SR & OCTOSPI_SR_FTF) == 0) {};	// Wait until there is space in the FIFO
		OCTOSPI1->DR = *val++;								// Set data register
	}

	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
}


void ExtFlash::SectorErase(const uint32_t address)
{
	// Erase a 4k sector
	WriteEnable();
	OCTOSPI1->DLR = 0;										// Write 1 byte
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear Dummy cycles
	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;						// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
	OCTOSPI1->CCR = OCTOSPI_CCR_ADMODE_2 |					// Address: 100: Eight lines
					OCTOSPI_CCR_ADSIZE |					// Address size: 32 bits
					OCTOSPI_CCR_IMODE_2 |					// Instruction: 100: Eight lines
					OCTOSPI_CCR_ISIZE_0 |					// Instruction size: 16 bits
					dtrCCR;

	OCTOSPI1->IR = sectorErase;
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = address;									// Set address register

	while ((OCTOSPI1->SR & OCTOSPI_SR_TCF) == 0) {};		// Wait until transfer complete
	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable OCTOSPI
}


void ExtFlash::MemoryMapped()
{
	// Activate memory mapped mode: 0x90000000
	if (!memMapMode) {
		//SCB_InvalidateDCache_by_Addr(flashAddress, 10000);// Ensure cache is refreshed after write or erase
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		//CheckBusy();										// Check chip is not still writing data

		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable QSPI
		OCTOSPI1->TCR |= (dummyCycles << OCTOSPI_TCR_DCYC_Pos);	// Set number of dummy cycles
		OCTOSPI1->CR |= OCTOSPI_CR_FMODE;					// 11: Memory-mapped mode
		OCTOSPI1->CCR = octoCCR;
		OCTOSPI1->IR = dtr ? octaReadDTR : octaRead;

		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		memMapMode = true;
	}
}


void ExtFlash::MemMappedOff()
{
	if (memMapMode) {
		OCTOSPI1->CR &= ~OCTOSPI_CR_EN;						// Disable OCTOSPI
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		memMapMode = false;
	}
}

