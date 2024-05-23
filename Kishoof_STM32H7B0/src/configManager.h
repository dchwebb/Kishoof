#pragma once

#include "initialisation.h"
#include <vector>

// Struct added to classes that need settings saved
struct ConfigSaver {
	void* settingsAddress;
	uint32_t settingsSize;
	void (*validateSettings)(void);		// function pointer to method that will validate config settings when restored
};


class Config {
public:
	static constexpr uint8_t configVersion = 1;
	
	// STM32H7B0 has 128k Flash in 16 sectors of 8192k
	static constexpr uint32_t flashConfigPage = 14;		// Allow 3 sectors for config giving a config size of 24k before erase needed
	static constexpr uint32_t flashSectorSize = 8192;
	uint32_t* const flashConfigAddr = reinterpret_cast<uint32_t* const>(FLASH_BASE + flashSectorSize * (flashConfigPage - 1));

	bool scheduleSave = false;
	uint32_t saveBooked = false;

	// Constructor taking multiple config savers: Get total config block size from each saver
	Config(std::initializer_list<ConfigSaver*> initList) : configSavers(initList) {
		for (auto saver : configSavers) {
			settingsSize += saver->settingsSize;
		}
		// Ensure config size (plus 4 byte header) is aligned to 8 byte boundary
		settingsSize = AlignTo8Bytes(settingsSize + sizeof(ConfigHeader));
	}

	void ScheduleSave();				// called whenever a config setting is changed to schedule a save after waiting to see if any more changes are being made
	bool SaveConfig(bool forceSave = false);
	void EraseConfig();					// Erase flash page containing config
	void RestoreConfig();				// gets config from Flash, checks and updates settings accordingly

private:
	static constexpr uint32_t flashAllErrors = FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_STRBERR | FLASH_SR_INCERR | FLASH_SR_RDPERR | FLASH_SR_RDSERR | FLASH_SR_SNECCERR | FLASH_SR_DBECCERR | FLASH_SR_CRCRDERR;

	const std::vector<ConfigSaver*> configSavers;
	uint32_t settingsSize = 0;			// Size of all settings from each config saver module + size of config header

	const char ConfigHeader[4] = {'C', 'F', 'G', configVersion};
	int32_t currentSettingsOffset = -1;	// Offset within flash page to block containing active/latest settings

	void FlashUnlock();
	void FlashLock();
	void FlashEraseSector(uint8_t Sector);
	bool FlashWaitForLastOperation();
	bool FlashProgram(uint32_t* dest_addr, uint32_t* src_addr, size_t size);

	static const inline uint32_t AlignTo8Bytes(uint32_t val) {
		val += 7;
		val >>= 3;
		val <<= 3;
		return val;
	}
};

extern Config config;
