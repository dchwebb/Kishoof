#pragma once

#include "stm32h7xx.h"
#include "mpu_armv7.h"		// Memory protection unit for selectively disabling cache for DMA transfers
#include <algorithm>
#include <cstdlib>
#include <cmath>

extern volatile uint32_t SysTickVal;

//#define SAMPLE_RATE 48000
#define SYSTICK 1000						// Set in uS so 1000uS = 1ms
#define ADC_OFFSET_DEFAULT 33800
#define CPUCLOCK 280

constexpr uint32_t sampleRate = 48000;
constexpr float intToFloatMult = 1.0f / std::pow(2.0f, 31.0f);		// Multiple to convert 32 bit int to -1.0 - 1.0 float
constexpr float floatToIntMult = std::pow(2.0f, 31.0f);				// Multiple to convert -1.0 - 1.0 float to 32 bit int
constexpr float pi = std::numbers::pi;

static constexpr uint32_t ADC1_BUFFER_LENGTH = 6;
static constexpr uint32_t ADC2_BUFFER_LENGTH = 6;

struct ADCValues {
	uint16_t Wavetable_Pos_B_Pot;
	uint16_t Wavetable_Pos_A_Trm;
	uint16_t AudioIn;
	uint16_t WarpCV;
	uint16_t Pitch_CV;
	uint16_t WavetablePosB_CV;
	uint16_t VcaCV;
	uint16_t WavetablePosA_CV;
	uint16_t Warp_Type_Pot;
	uint16_t Wavetable_Pos_A_Pot;
	uint16_t Warp_Amt_Trm;
	uint16_t Warp_Amt_Pot;
};


extern volatile ADCValues adc;
extern int32_t adcZeroOffset[2];

enum channel {left = 0, right = 1};

void InitClocks();
void InitHardware();
void InitCache();
void InitSysTick();
void InitDisplaySPI();
void InitADC();
void InitADC1();
void InitADC2();
void InitI2S();
void InitIO();
void StartI2STimer();
uint32_t ReadI2STimer();
void InitI2STimer();
void InitDebugTimer();
void StartDebugTimer();
float StopDebugTimer();
void DelayMS(uint32_t ms);
void InitMDMA();
void MDMATransfer(MDMA_Channel_TypeDef* channel, const uint8_t* srcAddr, const uint8_t* destAddr, const uint32_t bytes);
void InitEncoders();
void InitOctoSPI();
