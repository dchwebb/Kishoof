#include "stm32h743xx.h"
#include "initialisation.h"
#include "extFlash.h"
#include "GpioPin.h"
#include <cstring>
#include "FatTools.h"

// Clock overview:
// [Main clock (Nucleo): 8MHz (HSE) / 2 (M) * 200 (N) / 2 (P) = 400MHz]
// Main clock (v1): 12MHz (HSE) / 3 (M) * 200 (N) / 2 (P) = 400MHz
// I2S: Peripheral Clock (AKA per_ck) set to HSI = 64MHz
// ADC: Peripheral Clock (AKA per_ck) set to HSI = 64MHz

#if CPUCLOCK == 400
// 8MHz (HSE) / 2 (M) * 200 (N) / 2 (P) = 400MHz
// 12MHz (HSE) / 3 (M) * 200 (N) / 2 (P) = 400MHz
#define PLL_M1 2
#define PLL_N1 200
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 4			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 200 (N) / 4 (Q) = 200MHz
#define PLL_R1 2

#define PLL_M2 2
#define PLL_N2 200
#define PLL_P2 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_R2 16


#else
// 8MHz (HSE) / 2 (M) * 240 (N) / 2 (P) = 480MHz
#define PLL_M1 2
#define PLL_N1 240
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 4			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 240 (N) / 4 (Q) = 240MHz
#define PLL_R1 2
#endif


void InitClocks()
{
	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;			// Enable System configuration controller clock

	// Voltage scaling - see Datasheet page 113. VOS1 > 300MHz; VOS2 > 200MHz; VOS3 < 200MHz
	PWR->CR3 &= ~PWR_CR3_SCUEN;						// Supply configuration update enable - this must be deactivated or VOS ready does not come on
	PWR->D3CR |= PWR_D3CR_VOS;						// Configure voltage scaling level 1 before engaging overdrive (0b11 = VOS1)
	while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0);	// Check Voltage ready 1= Ready, voltage level at or above VOS selected level

#if CPUCLOCK > 400
	// Activate the LDO regulator overdrive mode. Must be written only in VOS1 voltage scaling mode.
	// This activates VSO0 mode for use in clock speeds from 400MHz to 480MHz
	SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
	while ((SYSCFG->PWRCR & SYSCFG_PWRCR_ODEN) == 0);
#endif

	RCC->CR |= RCC_CR_HSEON;						// Turn on external oscillator
	while ((RCC->CR & RCC_CR_HSERDY) == 0);			// Wait till HSE is ready

	// Clock source to High speed external and main (M) dividers
	RCC->PLLCKSELR = RCC_PLLCKSELR_PLLSRC_HSE |
	                 PLL_M1 << RCC_PLLCKSELR_DIVM1_Pos |
					 PLL_M2 << RCC_PLLCKSELR_DIVM2_Pos;

	// PLL 1 and 2 dividers
	RCC->PLL1DIVR = (PLL_N1 - 1) << RCC_PLL1DIVR_N1_Pos |
			        PLL_P1 << RCC_PLL1DIVR_P1_Pos |
			        (PLL_Q1 - 1) << RCC_PLL1DIVR_Q1_Pos |
			        (PLL_R1 - 1) << RCC_PLL1DIVR_R1_Pos;

	RCC->PLL2DIVR = (PLL_N2 - 1) << RCC_PLL2DIVR_N2_Pos |		// FIXME - don't need this for SD card - can just divide PPL1Q
			        PLL_P2 << RCC_PLL2DIVR_P2_Pos |
			        (PLL_R2 - 1) << RCC_PLL2DIVR_R2_Pos;

	RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2;			// 10: The PLL1 input (ref1_ck) clock range frequency is between 4 and 8 MHz (Will be 4MHz for 8MHz clock divided by 2)
	RCC->PLLCFGR |= RCC_PLLCFGR_PLL2RGE_2;			// 10: The PLL2 input (ref1_ck) clock range frequency is between 4 and 8 MHz (Will be 4MHz for 8MHz clock divided by 2)
	RCC->PLLCFGR |= RCC_PLLCFGR_PLL1VCOSEL;			// 0: Wide VCO range:192 to 836 MHz (default after reset)	1: Medium VCO range:150 to 420 MHz
	RCC->PLLCFGR |= RCC_PLLCFGR_PLL2VCOSEL;			// 0: Wide VCO range:192 to 836 MHz (default after reset)	1: Medium VCO range:150 to 420 MHz
	RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_DIVQ1EN | RCC_PLLCFGR_DIVR2EN;		// Enable divider outputs

	RCC->CR |= RCC_CR_PLL1ON;						// Turn on main PLL
	while ((RCC->CR & RCC_CR_PLL1RDY) == 0);		// Wait till PLL is ready

	RCC->CR |= RCC_CR_PLL2ON;						// Turn on PLL 2
	while ((RCC->CR & RCC_CR_PLL2RDY) == 0);		// Wait till PLL is ready

	// Peripheral scalers
	RCC->D1CFGR |= RCC_D1CFGR_HPRE_3;				// D1 domain AHB prescaler - divide 400MHz by 2 for 200MHz - this is then divided for all APB clocks below
	RCC->D1CFGR |= RCC_D1CFGR_D1PPRE_2; 			// Clock divider for APB3 clocks - set to 4 for 100MHz: 100: hclk / 2
	RCC->D2CFGR |= RCC_D2CFGR_D2PPRE1_2;			// Clock divider for APB1 clocks - set to 4 for 100MHz: 100: hclk / 2
	RCC->D2CFGR |= RCC_D2CFGR_D2PPRE2_2;			// Clock divider for APB2 clocks - set to 4 for 100MHz: 100: hclk / 2
	RCC->D3CFGR |= RCC_D3CFGR_D3PPRE_2;				// Clock divider for APB4 clocks - set to 4 for 100MHz: 100: hclk / 2


	RCC->CFGR |= RCC_CFGR_SW_PLL1;					// System clock switch: 011: PLL1 selected as system clock
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != (RCC_CFGR_SW_PLL1 << RCC_CFGR_SWS_Pos));		// Wait until PLL has been selected as system clock source

	// By default Flash latency is set to 7 wait states - set to 4 for now but may need to increase
	FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;
	while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS);

	SystemCoreClockUpdate();						// Update SystemCoreClock (system clock frequency)
}


void InitSSD()
{
	RCC->D1CCIPR |= RCC_D1CCIPR_SDMMCSEL;
	RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC1EN;

	GpioPin::Init(GPIOC, 8,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PC8   SDMMC1_D0
	GpioPin::Init(GPIOC, 9,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PC9   SDMMC1_D1
	GpioPin::Init(GPIOC, 10,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PC10  SDMMC1_D2
	GpioPin::Init(GPIOC, 11,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PC11  SDMMC1_D3
	GpioPin::Init(GPIOC, 12,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PC12  SDMMC1_CK
	GpioPin::Init(GPIOD, 2,  GpioPin::Type::AlternateFunction, 12, GpioPin::DriveStrength::VeryHigh);	// PD2   SDMMC1_CMD

    NVIC_SetPriority(SDMMC1_IRQn, 2);
    NVIC_EnableIRQ(SDMMC1_IRQn);




}


void InitHardware()
{
	InitSysTick();
	InitDebugTimer();				// Timer 3 used for performance testing
	InitCache();					// Configure MPU to not cache memory regions where DMA buffers reside
	InitSSD();
}

void InitCache()
{
	// Use the Memory Protection Unit (MPU) to set up a region of memory with data caching disabled for use with DMA buffers
	MPU->RNR = 0;									// Memory region number
	MPU->RBAR = reinterpret_cast<uint32_t>(&fatTools);	// Store the address of the ADC_array into the region base address register

	MPU->RASR = (0b11  << MPU_RASR_AP_Pos)   |		// All access permitted
				(0b001 << MPU_RASR_TEX_Pos)  |		// Type Extension field: See truth table on p228 of Cortex M7 programming manual
				(1     << MPU_RASR_S_Pos)    |		// Shareable: provides data synchronization between bus masters. Eg a processor with a DMA controller
				(0     << MPU_RASR_C_Pos)    |		// Cacheable
				(0     << MPU_RASR_B_Pos)    |		// Bufferable (ignored for non-cacheable configuration)
				(10     << MPU_RASR_SIZE_Pos) |		// Size is log 2(mem size) - 1 ie 2^6 = 64
				(1     << MPU_RASR_ENABLE_Pos);		// Enable MPU region


	/*
	// Use the Memory Protection Unit (MPU) to set up a region of memory with data caching disabled for use with DMA buffers
	MPU->RNR = 0;									// Memory region number
	MPU->RBAR = reinterpret_cast<uint32_t>(&adc);	// Store the address of the ADC_array into the region base address register

	MPU->RASR = (0b11  << MPU_RASR_AP_Pos)   |		// All access permitted
				(0b001 << MPU_RASR_TEX_Pos)  |		// Type Extension field: See truth table on p228 of Cortex M7 programming manual
				(1     << MPU_RASR_S_Pos)    |		// Shareable: provides data synchronization between bus masters. Eg a processor with a DMA controller
				(0     << MPU_RASR_C_Pos)    |		// Cacheable
				(0     << MPU_RASR_B_Pos)    |		// Bufferable (ignored for non-cacheable configuration)
				(5     << MPU_RASR_SIZE_Pos) |		// Size is log 2(mem size) - 1 ie 2^6 = 64
				(1     << MPU_RASR_ENABLE_Pos);		// Enable MPU region



	MPU->RNR = 1;									// Memory region number
	MPU->RBAR = 0x90000000; 						// Address of the QSPI Flash

	MPU->RASR = (0b11  << MPU_RASR_AP_Pos)   |		// All access permitted
				(0b001 << MPU_RASR_TEX_Pos)  |		// Type Extension field: See truth table on p228 of Cortex M7 programming manual
				(1     << MPU_RASR_S_Pos)    |		// Shareable: provides data synchronization between bus masters. Eg a processor with a DMA controller
				(0     << MPU_RASR_C_Pos)    |		// Cacheable
				(0     << MPU_RASR_B_Pos)    |		// Bufferable (ignored for non-cacheable configuration)
				(17    << MPU_RASR_SIZE_Pos) |		// 256KB - D2 is actually 288K (size is log 2(mem size) - 1 ie 2^18 = 256k)
				(1     << MPU_RASR_ENABLE_Pos);		// Enable MPU region

*/
	MPU->CTRL |= (1 << MPU_CTRL_PRIVDEFENA_Pos) |	// Enable PRIVDEFENA - this allows use of default memory map for memory areas other than those specific regions defined above
				 (1 << MPU_CTRL_ENABLE_Pos);		// Enable the MPU

	// Enable data and instruction caches
	SCB_EnableDCache();
	SCB_EnableICache();
}


void InitSysTick()
{
	SysTick_Config(SystemCoreClock / SYSTICK);		// gives 1ms
	NVIC_SetPriority(SysTick_IRQn, 0);
}


void InitDAC()
{
	// DAC1_OUT2 on PA5
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;			// GPIO port clock
	RCC->APB1LENR |= RCC_APB1LENR_DAC12EN;			// Enable DAC Clock

	DAC1->CR |= DAC_CR_EN1;							// Enable DAC using PA4 (DAC_OUT1)
	DAC1->MCR &= DAC_MCR_MODE1_Msk;					// Mode = 0 means Buffer activated, Connected to external pin

	DAC1->CR |= DAC_CR_EN2;							// Enable DAC using PA5 (DAC_OUT2)
	DAC1->MCR &= DAC_MCR_MODE2_Msk;					// Mode = 0 means Buffer activated, Connected to external pin
}






void InitDebugTimer()
{
	// Configure timer to use in internal debug timing - uses APB1 timer clock which is 200MHz -
	// Each tick is 5ns with PSC 10nS - full range is 655.350 uS
	RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
	TIM3->ARR = 65535;
	TIM3->PSC = 1;

}


void StartDebugTimer() {
	TIM3->EGR |= TIM_EGR_UG;
	TIM3->CR1 |= TIM_CR1_CEN;
}


float StopDebugTimer() {
	// Return time running in microseconds
	const uint32_t count = TIM3->CNT;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	return 0.01f * count;
}


void InitIO()
{
	GpioPin::Init(GPIOC, 6,  GpioPin::Type::InputPullup);		// PC6: Seq Select
	GpioPin::Init(GPIOE, 1,  GpioPin::Type::InputPullup);		// PE1: MIDI Learn
	GpioPin::Init(GPIOB, 6,  GpioPin::Type::InputPullup);		// PB6: Kick button
	GpioPin::Init(GPIOB, 5,  GpioPin::Type::InputPullup);		// PB5: Snare button
	GpioPin::Init(GPIOE, 11, GpioPin::Type::InputPullup);		// PE11: HiHat button
	GpioPin::Init(GPIOB, 7,  GpioPin::Type::InputPullup);		// PB7: Sample A button
	GpioPin::Init(GPIOG, 13, GpioPin::Type::InputPullup);		// PG13: Sample B button
	GpioPin::Init(GPIOD, 1,  GpioPin::Type::Input);				// PD1: Kick
	GpioPin::Init(GPIOD, 3,  GpioPin::Type::Input);				// PD3: Snare
	GpioPin::Init(GPIOG, 10, GpioPin::Type::Input);				// PG10: Closed Hihat
	GpioPin::Init(GPIOD, 7,  GpioPin::Type::Input);				// PD7: Open Hihat
	GpioPin::Init(GPIOE, 14, GpioPin::Type::Input);				// PE14: Sampler A
	GpioPin::Init(GPIOE, 15, GpioPin::Type::Input);				// PE15: Sampler B
	GpioPin::Init(GPIOD, 9,  GpioPin::Type::Output);			// PD9: Tempo out
}


void InitQSPI()
{
	RCC->D1CCIPR &= ~RCC_D1CCIPR_QSPISEL;			// 00: hsi_ker_ck clock selected as per_ck clock
	RCC->AHB3ENR |= RCC_AHB3ENR_QSPIEN;				// Enable QSPI clock

	// NB - use v high drive strength or does not work
	GpioPin::Init(GPIOB, 2,  GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PB2:  CLK Flash 1
	GpioPin::Init(GPIOD, 11, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PD11: IO0 Flash 1
	GpioPin::Init(GPIOD, 12, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PD12: IO1 Flash 1
	GpioPin::Init(GPIOD, 13, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PD13: IO3 Flash 1
	GpioPin::Init(GPIOE, 2,  GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PE2:  IO2 Flash 1
	GpioPin::Init(GPIOG, 6,  GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);		// PG6:  NCS Flash 1

	GpioPin::Init(GPIOE, 7,  GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);		// PE7:  IO0 Flash 2
	GpioPin::Init(GPIOE, 8,  GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);		// PE8:  IO1 Flash 2
	GpioPin::Init(GPIOE, 9,  GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);		// PE9:  IO2 Flash 2
	GpioPin::Init(GPIOE, 10, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);		// PE10: IO3 Flash 2
	GpioPin::Init(GPIOC, 11, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PC11: NCS Flash 2

	QUADSPI->CR |= 1 << QUADSPI_CR_PRESCALER_Pos;	// Set prescaler to n + 1 => 200MHz / 2 = 100MHz

	if (dualFlashMode) {
		QUADSPI->DCR |= 24 << QUADSPI_DCR_FSIZE_Pos;// Set bytes in Flash memory to 2^(FSIZE + 1) = 2^25 = 32 Mbytes
		QUADSPI->CR |= QUADSPI_CR_DFM;				// Enable dual flash mode
	} else {
		QUADSPI->DCR |= 23 << QUADSPI_DCR_FSIZE_Pos;// Set bytes in Flash memory to 2^(FSIZE + 1) = 2^24 = 16 Mbytes
		QUADSPI->CR |= QUADSPI_CR_FSEL;				// Flash memory selection: 0: FLASH 1 selected; 1: FLASH 2 selected
	}
}




void InitRNG()
{
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;				// Enable clock
	RNG->CR |= RNG_CR_RNGEN;						// Enable random number generator
}



void DelayMS(uint32_t ms)
{
	// Crude delay system
	const uint32_t now = SysTickVal;
	while (now + ms > SysTickVal) {};
}


void Reboot()
{
	__disable_irq();
	__DSB();
	NVIC_SystemReset();
}
