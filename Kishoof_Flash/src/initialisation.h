#pragma once

#include "stm32h7xx.h"
#include "mpu_armv7.h"		// Memory protection unit for selectively disabling cache for DMA transfers
#include <algorithm>
#include <cstdlib>
#include <cmath>

extern volatile uint32_t SysTickVal;


#define ADC1_BUFFER_LENGTH 8
#define ADC2_BUFFER_LENGTH 7
#define SYSTICK 1000						// Set in uS so 1000uS = 1ms
#define CPUCLOCK 400

constexpr double pi = 3.14159265358979323846;
constexpr float intToFloatMult = 1.0f / std::pow(2.0f, 31.0f);		// Multiple to convert 32 bit int to -1.0 - 1.0 float
constexpr float floatToIntMult = std::pow(2.0f, 31.0f);				// Multiple to convert -1.0 - 1.0 float to 32 bit int

static constexpr uint32_t sampleRate = 48000;
static constexpr float systemMaxFreq = 22000.0f;

extern volatile uint16_t ADC_array[ADC1_BUFFER_LENGTH + ADC2_BUFFER_LENGTH];
extern uint32_t i2sUnderrun;					// Debug counter for I2S underruns

struct ADCValues {
	uint16_t ADC_KickAttack;
	uint16_t ADC_KickDecay;
	uint16_t ADC_KickLevel;
	uint16_t ADC_SnareFilter;
	uint16_t ADC_SnareTuning;
	uint16_t ADC_SnareLevel;
	uint16_t ADC_SampleALevel;
	uint16_t ADC_SampleBLevel;
	uint16_t ADC_Tempo;
	uint16_t ADC_HiHatDecay;
	uint16_t ADC_HiHatLevel;
	uint16_t ADC_SampleAVoice;
	uint16_t ADC_SampleASpeed;
	uint16_t ADC_SampleBVoice;
	uint16_t ADC_SampleBSpeed;
};
extern volatile ADCValues adc;

enum channel {left = 0, right = 1};

void InitClocks();
void InitHardware();
void InitCache();
void InitSysTick();
void InitADC();
void InitADC1();
void InitADC2();
void InitDAC();
void InitI2S();
void suspendI2S();
void resumeI2S();
void InitIO();
void InitDebugTimer();
void InitQSPI();
void InitMDMA();
void MDMATransfer(const uint8_t* srcAddr, const uint8_t* destAddr, uint32_t bytes);
void InitMidiUART();
void InitRNG();
void InitPWMTimer();
void DelayMS(uint32_t ms);
void Reboot();
void StartDebugTimer();
float StopDebugTimer();
