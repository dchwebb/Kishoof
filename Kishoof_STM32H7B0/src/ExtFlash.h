#pragma once

#include "initialisation.h"

static uint8_t* const flashAddress = reinterpret_cast<uint8_t*>(0x90000000);			// Location that Flash storage will be accessed in memory mapped mode


class ExtFlash {
	friend class CDCHandler;
public:
	// Octal mode commands - SPI mode are first 8 bits of octal command
	enum ospiRegister : uint16_t {writeEnable = 0x06F9, getID = 0x9F60, octaRead = 0xEC13, octaReadDTR = 0xEE11, writePage = 0x12ED,
		readStatusReg = 0x05FA, readCfgReg = 0x15EA, readCfgReg2 = 0x718E, writeCfgReg2 = 0x728D,
		sectorErase = 0x21DE, chipErase = 0xC738, manufacturerID = 0x90, enableReset = 0x6699, resetDevice = 0x9966};

	void Init();
	uint32_t Read(uint32_t address);
	bool WriteData(uint32_t address, uint32_t* val, uint32_t words);
	uint32_t GetID(bool forceOctalMode = false);
	uint32_t ReadStatusReg();
	uint32_t ReadReg(uint32_t address);
	uint32_t ReadCfgReg(uint32_t address);
	void BlockErase(const uint32_t address);
	void FullErase();
	void SetOctoMode();
	void Reset();
	void MemoryMapped();

	bool errorState = false;
	bool flashCorrupt = false;
private:

	void MemMappedOff();
	void WriteEnable();
	void WriteCfg2(uint32_t address, uint32_t data);
	void CheckBusy();
	bool memMapMode = false;
	bool octalMode = true;
};

extern ExtFlash extFlash;
