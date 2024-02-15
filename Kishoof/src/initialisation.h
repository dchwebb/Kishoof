#pragma once

#include "stm32h7xx.h"
#include "mpu_armv7.h"		// Memory protection unit for selectively disabling cache for DMA transfers
#include <algorithm>
#include <cstdlib>
#include <cmath>

extern volatile uint32_t SysTickVal;

#define ADC1_BUFFER_LENGTH 4
#define ADC2_BUFFER_LENGTH 7
#define SAMPLE_BUFFER_LENGTH 1048576		// Currently 2^20 (4MB of 16MB)
//#define SAMPLE_RATE 48000
#define SYSTICK 1000						// Set in uS so 1000uS = 1ms
#define ADC_OFFSET_DEFAULT 33800
#define CPUCLOCK 400

constexpr uint32_t sampleRate = 48000;
constexpr float intToFloatMult = 1.0f / std::pow(2.0f, 31.0f);		// Multiple to convert 32 bit int to -1.0 - 1.0 float
constexpr float floatToIntMult = std::pow(2.0f, 31.0f);				// Multiple to convert -1.0 - 1.0 float to 32 bit int

struct ADCValues {
	uint16_t audio_L;
	uint16_t audio_R;
	uint16_t DelayPot_R;
	uint16_t DelayCV_R;
	uint16_t Mix;
	uint16_t DelayPot_L;
	uint16_t DelayCV_L;
	uint16_t FeedbackPot;
	uint16_t FeedbackCV;
	uint16_t FilterCV;
	uint16_t FilterPot;
};

extern volatile ADCValues adc;
extern int32_t adcZeroOffset[2];

enum channel {left = 0, right = 1};

void InitClocks();
void InitHardware();
void InitCache();
void InitSysTick();
void uartSendChar(char c);
void uartSendString(const char* s);
void InitADC();
void InitADC1();
void TriggerADC1();
void InitADC2();
void InitDAC();
void InitI2S();
void suspendI2S();
void resumeI2S();
void InitTempoClock();
void InitIO();
void InitDebugTimer();
void StartDebugTimer();
float StopDebugTimer();
