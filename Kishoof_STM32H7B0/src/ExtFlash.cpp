#include "ExtFlash.h"
#include "FatTools.h"

ExtFlash extFlash;

static constexpr bool dtr = false;			// Double data rate

static constexpr uint32_t dummyCycles = 6;	// Configured according to device speed

static constexpr uint32_t dtrCCR = dtr ? (OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_DDTR | OCTOSPI_CCR_DQSE) : 0;		//OCTOSPI_CCR_DQSE

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
	OCTOSPI1->TCR |= OCTOSPI_TCR_DHQC;						// Delay hold quarter cycle
	Reset();
	SetOctoMode();
	if (GetID() != 0x003a85c2 || ReadCfgReg(0x300) != 0b111) {
		Reset();
		DelayMS(2);		// FIXME - use better timing
		SetOctoMode();
	}
	MemoryMapped();
	fatTools.InitFatFS();									// Initialise FatFS

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
	CheckBusy();
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
	OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;					// Clear number of dummy cycles

	if (forceOctalMode || octalMode) {
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


bool ExtFlash::WriteData(uint32_t address, uint32_t* writeBuff, uint32_t words)
{
	// Writes up to 256 bytes at a time aligned to 256 boundary
	bool eraseRequired = false;
	bool dataChanged = false;
	uint32_t* memAddr = (uint32_t*)(flashAddress + address);	// address is passed relative to zero - store the memory mapped address

	for (uint32_t i = 0; i < words; ++i) {
		if (memAddr[i] != writeBuff[i]) {
			dataChanged = true;
			if ((memAddr[i] & writeBuff[i]) != writeBuff[i]) {	// 'And' tests if any bits that need to be set are currently zero - ie needing an erase
				eraseRequired = true;
				break;
			}
		}
	}
	if (!dataChanged) {										// No difference between Flash contents and write data
		return false;
	}

	if (eraseRequired) {
		BlockErase(address & ~(fatEraseSectors - 1));		// Force address to 4096 byte (8192 in dual flash mode) boundary
	}

	uint32_t remainingWords = words;

	do {
		WriteEnable();

		// Mandated by Flash chip: Can write 256 bytes (64 words) at a time, and must be aligned to page boundaries (256 bytes)
		constexpr uint32_t pageShift = 8;					// Will divide words into pages 256 bytes
		const uint32_t startPage = (address >> pageShift);
		const uint32_t endPage = (((address + (remainingWords * 4)) - 1) >> pageShift);

		uint32_t writeSize = remainingWords * 4;			// Size of current write in bytes
		if (endPage != startPage) {
			writeSize = ((startPage + 1) << pageShift) - address;	// When crossing pages only write up to the 256 byte boundary
		}

		OCTOSPI1->DLR = writeSize - 1;						// number of bytes - 1 to transmit
		OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;					// *00: Indirect write mode; 01: Indirect read mode; 10: Automatic polling mode; 11: Memory-mapped mode
		OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;				// Dummy cycles
		OCTOSPI1->CCR = octoCCR;
		OCTOSPI1->IR = writePage;
		OCTOSPI1->CR |= OCTOSPI_CR_EN;						// Enable OCTOSPI
		OCTOSPI1->AR = address;								// Set address register

		for (uint32_t i = 0; i < 64; ++i) {
			while ((OCTOSPI1->SR & OCTOSPI_SR_FTF) == 0) {};// Wait until there is space in the FIFO
			OCTOSPI1->DR = *writeBuff++;					// Set data register
		}

		remainingWords -= (writeSize / 4);
		address += writeSize;

		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		OCTOSPI1->CR &= ~OCTOSPI_CR_EN;						// Disable OCTOSPI
	} while (remainingWords > 0);

	SCB_InvalidateDCache_by_Addr(memAddr, words * 4);		// Ensure cache is refreshed after write or erase

	MemoryMapped();											// Switch back to memory mapped mode

	return true;
}


void ExtFlash::BlockErase(const uint32_t address)
{
	// Erase a 4k sector (ie a FAT block) NB a 'Block' for the Flash is 64K
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
		CheckBusy();										// Check chip is not still writing data

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
		OCTOSPI1->TCR &= ~OCTOSPI_TCR_DCYC_Msk;				// Clear number of dummy cycles
		OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE;
		while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
		memMapMode = false;
	}
}


void ExtFlash::CheckBusy()
{
	// Use Automatic Polling mode to wait until Flash Busy flag is cleared
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;							// Disable
	OCTOSPI1->DLR = 1;										// Return 2 bytes
	OCTOSPI1->PSMKR = 0x01;									// Mask on bit 1 (Busy)
	OCTOSPI1->PSMAR = 0b00000000;							// Match Busy = 0
	OCTOSPI1->PIR = 0x10;									// Polling interval in clock cycles
	OCTOSPI1->CR |= OCTOSPI_CR_APMS;						// Set the 'auto-stop' bit to end transaction after a match.
	OCTOSPI1->CR = (OCTOSPI1->CR & ~OCTOSPI_CR_FMODE) |
					OCTOSPI_CR_FMODE_1;						// 10: Automatic polling mode

	OCTOSPI1->TCR |= (4 << OCTOSPI_TCR_DCYC_Pos);			// Set number of dummy cycles
	OCTOSPI1->CCR = octoCCR;
	OCTOSPI1->IR = readStatusReg;							// Set address register
	OCTOSPI1->CR |= OCTOSPI_CR_EN;							// Enable OCTOSPI
	OCTOSPI1->AR = 0;										// Set address register

	while (OCTOSPI1->SR & OCTOSPI_SR_BUSY) {};
	OCTOSPI1->FCR |= OCTOSPI_FCR_CSMF;						// Acknowledge status match flag
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;
}



