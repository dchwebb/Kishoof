#include <WaveTable.h>
#include "stm32h743xx.h"
#include "initialisation.h"
#include "GpioPin.h"


// Clock overview:
// Main clock 4MHz: 8MHz (HSE) / 2 (M) * 200 (N) / 2 (P) = 400MHz
// SDRAM: 200MHz: Defaulted to D1 domain AHB prescaler (RCC_D1CFGR_HPRE_3) ie main clock /2 = 200MHz (AKA HCLK3)
// I2S: Peripheral Clock (AKA per_ck) set to HSI = 64MHz
// ADC: Peripheral Clock (AKA per_ck) set to HSI = 64MHz

#if CPUCLOCK == 400
// 8MHz (HSE) / 2 (M) * 200 (N) / 2 (P) = 400MHz
#define PLL_M1 2
#define PLL_N1 200
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 4			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 200 (N) / 4 (Q) = 200MHz
#define PLL_R1 2
#elif CPUCLOCK == 320
// 8MHz (HSE) / 2 (M) * 160 (N) / 2 (P) = 320MHz
#define PLL_M1 2
#define PLL_N1 160
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 2			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 160 (N) / 2 (Q) = 320MHz
#define PLL_R1 2

#elif CPUCLOCK == 280
// 8MHz (HSE) / 2 (M) * 140 (N) / 2 (P) = 280MHz
#define PLL_M1 2
#define PLL_N1 140
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 2			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 140 (N) / 2 (Q) = 280MHz
#define PLL_R1 2
#elif CPUCLOCK == 240
// 8MHz (HSE) / 2 (M) * 120 (N) / 2 (P) = 240MHz
#define PLL_M1 2
#define PLL_N1 120
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 2			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 120 (N) / 2 (Q) = 240MHz
#define PLL_R1 2
#elif CPUCLOCK == 200
// 8MHz (HSE) / 2 (M) * 100 (N) / 2 (P) = 200MHz
#define PLL_M1 2
#define PLL_N1 100
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 2			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 100 (N) / 2 (Q) = 200MHz FIXME - problems using this PLL divider and FMC
#define PLL_R1 2
#else
// 8MHz (HSE) / 2 (M) * 240 (N) / 2 (P) = 480MHz
#define PLL_M1 2
#define PLL_N1 240
#define PLL_P1 1			// 0000001: pll1_p_ck = vco1_ck / 2
#define PLL_Q1 4			// This is used for I2S clock: 8MHz (HSE) / 2 (M) * 240 (N) / 4 (Q) = 240MHz
#define PLL_R1 2
#endif

#ifdef PLL2ON
// PLL2R used for SDRAM clock - not currently used as was interfering with timing on I2S
// 8MHz (HSE) / 4 (M) * 200 (N) / 2 (R) = 200MHz (Maximum speed successfully tested 230MHz)
// PPL2P used for ADC: 8MHz / 4 (M) * 200 (N) / 5 (P) = 80MHz
#define PLL_M2 4
#define PLL_N2 200
#define PLL_R2 1			// 0000001: pll2_r_ck = vco2_ck / 2
#define PLL_P2 4			// 0000100: pll2_p_ck = vco2_ck / 5
#endif

void InitClocks()
{

	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;			// Enable System configuration controller clock

	// Voltage scaling - see Datasheet page 113. VOS1 > 300MHz; VOS2 > 200MHz; VOS3 < 200MHz
	PWR->CR3 &= ~PWR_CR3_SCUEN;						// Supply configuration update enable - this must be deactivated or VOS ready does not come on
#if CPUCLOCK <= 200
	PWR->D3CR |= PWR_D3CR_VOS_0;					// Configure voltage scaling level 2 (0b10 = VOS2)
	PWR->D3CR &= ~PWR_D3CR_VOS_1;
#elif CPUCLOCK <= 280
	PWR->D3CR |= PWR_D3CR_VOS_1;					// Configure voltage scaling level 2 (0b10 = VOS2)
	PWR->D3CR &= ~PWR_D3CR_VOS_0;
#else
	PWR->D3CR |= PWR_D3CR_VOS;						// Configure voltage scaling level 1 before engaging overdrive (0b11 = VOS1)
#endif
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
	                 PLL_M1 << RCC_PLLCKSELR_DIVM1_Pos;

	// PLL 1 dividers
	RCC->PLL1DIVR = (PLL_N1 - 1) << RCC_PLL1DIVR_N1_Pos |
			        PLL_P1 << RCC_PLL1DIVR_P1_Pos |
			        (PLL_Q1 - 1) << RCC_PLL1DIVR_Q1_Pos |
			        (PLL_R1 - 1) << RCC_PLL1DIVR_R1_Pos;

	RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2;			// 10: The PLL1 input (ref1_ck) clock range frequency is between 4 and 8 MHz (Will be 4MHz for 8MHz clock divided by 2)
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL;		// 0: Wide VCO range:192 to 836 MHz (default after reset)	1: Medium VCO range:150 to 420 MHz
	RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_DIVQ1EN;		// Enable divider outputs

	RCC->CR |= RCC_CR_PLL1ON;						// Turn on main PLL
	while ((RCC->CR & RCC_CR_PLL1RDY) == 0);		// Wait till PLL is ready

#ifdef PLL2ON
	RCC->PLLCKSELR |= PLL_M2 << RCC_PLLCKSELR_DIVM2_Pos;

	// PLL 2 dividers
	RCC->PLL2DIVR = (PLL_N2 - 1) << RCC_PLL2DIVR_N2_Pos |
			        PLL_R2 << RCC_PLL2DIVR_R2_Pos |
					PLL_P2 << RCC_PLL2DIVR_P2_Pos;

	RCC->PLLCFGR |= RCC_PLLCFGR_PLL2RGE_1;			// 01: The PLL2 input (ref1_ck) clock range frequency is between 2 and 4 MHz (Will be 2MHz for 8MHz clock divided by 4)
	RCC->PLLCFGR |= RCC_PLLCFGR_PLL2VCOSEL;			// 1: Medium VCO range:150 to 420 MHz
	RCC->PLLCFGR |= RCC_PLLCFGR_DIVR2EN;			// Enable R divider output for SDRAM clock
	RCC->PLLCFGR |= RCC_PLLCFGR_DIVP2EN;			// Enable P divider output for ADC clock

	RCC->CR |= RCC_CR_PLL2ON;						// Turn on PLL 2
	while ((RCC->CR & RCC_CR_PLL2RDY) == 0);		// Wait till PLL is ready
#endif

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

	SystemCoreClockUpdate();						// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL

}


void InitHardware()
{
	InitSysTick();
	InitCache();
	InitIO();										// Initialise switches and LEDs
	InitMDMA();
	InitDAC();			// Only needed on prototype hardware to activate VCA
	InitADC();
	InitDebugTimer();
	InitI2STimer();
	InitDisplaySPI();

	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
	RNG->CR |= RNG_CR_RNGEN;
}


void InitCache()
{
	// Use the Memory Protection Unit (MPU) to set up a region of memory with data caching disabled for use with DMA buffers
	MPU->RNR = 0;									// Memory region number
	MPU->RBAR = reinterpret_cast<uint32_t>(&adc);	// Store the address of the ADC_array into the region base address register

	MPU->RASR = (0b11  << MPU_RASR_AP_Pos)   |		// All access permitted
				(0b001 << MPU_RASR_TEX_Pos)  |		// Type Extension field: See truth table on p228 of Cortex M7 programming manual
				(1     << MPU_RASR_S_Pos)    |		// Shareable: provides data synchronization between bus masters. Eg a processor with a DMA controller
				(0     << MPU_RASR_C_Pos)    |		// Cacheable
				(0     << MPU_RASR_B_Pos)    |		// Bufferable (ignored for non-cacheable configuration)
				(17    << MPU_RASR_SIZE_Pos) |		// 256KB - D2 is actually 288K (size is log 2(mem size) - 1 ie 2^18 = 256k)
				(1     << MPU_RASR_ENABLE_Pos);		// Enable MPU region


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


void InitDisplaySPI()
{
	// SPI123 uses per_ck by default; per_ck configured in CKPERSEL in RCC_D1CCIPR (64MHz HSI RC clock by default)
	RCC->APB1LENR |= RCC_APB1LENR_SPI3EN;

	GpioPin::Init(GPIOC, 10, GpioPin::Type::AlternateFunction, 6, GpioPin::DriveStrength::VeryHigh);		// PC10: SPI3_SCK AF6
	GpioPin::Init(GPIOC, 12, GpioPin::Type::AlternateFunction, 6, GpioPin::DriveStrength::VeryHigh);		// PC12: SPI3_MOSI AF6

	// Configure SPI
	SPI3->CFG2 |= SPI_CFG2_COMM_0;					// 00: full-duplex, *01: simplex transmitter, 10: simplex receiver, 11: half-duplex
	SPI3->CFG2 |= SPI_CFG2_SSM;						// Software slave management: When SSM bit is set, NSS pin input is replaced with the value from the SSI bit
	SPI3->CR1 |= SPI_CR1_SSI;						// Internal slave select
	SPI3->CFG2 |= SPI_CFG2_SSOM;					// SS output management in master mode
	SPI3->CFG1 |= SPI_CFG1_MBR_1;					// Master Baud rate p2238: 010: ck/8; *011: ck/16; 100: ck/32; 101: ck/64
	SPI3->CFG2 |= SPI_CFG2_MASTER;					// Master selection
	SPI3->CR1 |= SPI_CR1_SPE;						// Enable SPI

	// Configure DMA
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	DMA1_Stream0->CR |= DMA_SxCR_MSIZE_0;			// Memory size 16 bit [8 bit; 01 = 16 bit; 10 = 32 bit]
	DMA1_Stream0->CR |= DMA_SxCR_PSIZE_0;			// Peripheral size 16 bit
	DMA1_Stream0->CR |= DMA_SxCR_DIR_0;				// data transfer direction: 00: peripheral-to-memory; 01: memory-to-peripheral; 10: memory-to-memory
	DMA1_Stream0->CR |= DMA_SxCR_PL_0;				// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High
	DMA1_Stream0->CR |= DMA_SxCR_MINC;				// Memory in increment mode
	DMA1_Stream0->PAR = (uint32_t)(&(SPI3->TXDR));	// Configure the peripheral data register address

	DMAMUX1_Channel0->CCR |= 62; 					// DMA request MUX input 62 = spi3_tx_dma (See p.695)
	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF0; // Clear synchronization overrun event flag
}


// Only needed on prototype hardware to activate VCA
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


void InitAdcPins(ADC_TypeDef* ADC_No, std::initializer_list<uint8_t> channels) {
	uint8_t sequence = 1;

	for (auto channel: channels) {
		// NB reset mode of GPIO pins is 0b11 = analog mode so shouldn't need to change
		ADC_No->PCSEL |= 1 << channel;					// ADC channel preselection register

		// Set conversion sequence to order ADC channels are passed to this function
		if (sequence < 5) {
			ADC_No->SQR1 |= channel << (sequence * 6);
		} else if (sequence < 10) {
			ADC_No->SQR2 |= channel << ((sequence - 5) * 6);
		} else if (sequence < 15)  {
			ADC_No->SQR3 |= channel << ((sequence - 10) * 6);
		} else {
			ADC_No->SQR4 |= channel << ((sequence - 15) * 6);
		}

		// 000: 1.5 ADC clock cycles; 001: 2.5 cycles; 010: 8.5 cycles;	011: 16.5 cycles; 100: 32.5 cycles; 101: 64.5 cycles; 110: 387.5 cycles; 111: 810.5 cycles
		if (channel < 10)
			ADC_No->SMPR1 |= 0b010 << (3 * channel);
		else
			ADC_No->SMPR2 |= 0b010 << (3 * (channel - 10));

		sequence++;
	}
}

// Settings used for both ADC1 and ADC2
void InitADC()
{
	// Configure clocks
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;			// GPIO port clock
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;

	RCC->AHB1ENR |= RCC_AHB1ENR_ADC12EN;

	// 00: pll2_p_ck default, 01: pll3_r_ck clock, 10: per_ck clock*
	RCC->D3CCIPR |= RCC_D3CCIPR_ADCSEL_1;			// ADC clock selection: 10: per_ck clock (hse_ck, hsi_ker_ck or csi_ker_ck according to CKPERSEL in RCC->D1CCIPR p.353)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	ADC12_COMMON->CCR |= ADC_CCR_PRESC_0;			// Set prescaler to ADC clock divided by 2 (if 64MHz = 32MHz)

	InitADC1();
	InitADC2();
}


/*-----------------------------------------------------------------------------------------------------------------
ADC Channels are distributed across ADC1 and ADC2 for faster conversions. Audio channels on ADC1 for better quality.
Both insert data into ADC_array at different offsets.
 ADC1:
	0	PA3 ADC12_INP15		AUDIO_IN_R
	1	PA2 ADC12_INP14		AUDIO_IN_L
	2	PA1 ADC1_INP17		DELAY_POT_R
	3	PA0 ADC1_INP16		DELAY_CV_SCALED_L
ADC2:
	0	PC5 ADC12_INP8		WET_DRY_MIX
	1	PB1 ADC12_INP5		DELAY_POT_L
	2	PA6 ADC12_INP3 		DELAY_CV_SCALED_R
	3	PB0 ADC12_INP9		FEEDBACK_POT
	4	PC4 ADC12_INP4		FEEDBACK_CV_SCALED
	5	PC1 ADC123_INP11 	FILTER_POT
*/

void InitADC1()
{
	// Initialize ADC peripheral
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
//	DMA1_Stream1->CR |= DMA_SxCR_CIRC;				// Circular mode to keep refilling buffer
	DMA1_Stream1->CR |= DMA_SxCR_MINC;				// Memory in increment mode
	DMA1_Stream1->CR |= DMA_SxCR_PSIZE_0;			// Peripheral size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream1->CR |= DMA_SxCR_MSIZE_0;			// Memory size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream1->CR |= DMA_SxCR_PL_0;				// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	DMA1_Stream1->FCR &= ~DMA_SxFCR_FTH;			// Disable FIFO Threshold selection
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF1_Pos;		// clear all five interrupts for this stream

	DMAMUX1_Channel1->CCR |= 9; 					// DMA request MUX input 9 = adc1_dma (See p.695)
	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF1; // Channel 1 Clear synchronization overrun event flag

	ADC1->CR &= ~ADC_CR_DEEPPWD;					// Deep powDMA1_Stream2own: 0: ADC not in deep-power down	1: ADC in deep-power-down (default reset state)
	ADC1->CR |= ADC_CR_ADVREGEN;					// Enable ADC internal voltage regulator

	// Wait until voltage regulator settled
	volatile uint32_t wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}
	while ((ADC1->CR & ADC_CR_ADVREGEN) != ADC_CR_ADVREGEN) {}

	ADC1->CFGR |= ADC_CFGR_CONT;					// 1: Continuous conversion mode for regular conversions
	ADC1->CFGR |= ADC_CFGR_OVRMOD;					// Overrun Mode 1: ADC_DR register is overwritten with the last conversion result when an overrun is detected.
	ADC1->CFGR |= ADC_CFGR_DMNGT_0;					// Data Management configuration 01: DMA One Shot Mode selected, 11: DMA Circular Mode selected

	// Boost mode 1: Boost mode on. Must be used when ADC clock > 20 MHz.
	ADC1->CR |= ADC_CR_BOOST_1;						// Note this sets reserved bit according to SFR - HAL has notes about silicon revision

	// For scan mode: set number of channels to be converted
	ADC1->SQR1 |= (ADC1_BUFFER_LENGTH - 1);

	// Start calibration
	ADC1->CR |= ADC_CR_ADCALLIN;					// Activate linearity calibration (as well as offset calibration)
	ADC1->CR |= ADC_CR_ADCAL;
	while ((ADC1->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL) {};


	/*--------------------------------------------------------------------------------------------
	Configure ADC Channels to be converted:
	0	PA3 ADC12_INP15		AUDIO_IN_L
	1	PA2 ADC12_INP14		AUDIO_IN_R
	2	PA1 ADC1_INP17		DELAY_POT_R
	3	PA0 ADC1_INP16		DELAY_CV_SCALED_R
	*/
	InitAdcPins(ADC1, {15, 14, 17, 16});


	// Enable ADC
	ADC1->CR |= ADC_CR_ADEN;
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0) {}

	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF1; // Channel 1 Clear synchronization overrun event flag
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF1_Pos;		// clear all five interrupts for this stream

	DMA1_Stream1->NDTR |= ADC1_BUFFER_LENGTH;		// Number of data items to transfer (ie size of ADC buffer)
	DMA1_Stream1->PAR = (uint32_t)(&(ADC1->DR));	// Configure the peripheral data register address 0x40022040
	DMA1_Stream1->M0AR = (uint32_t)(&adc);		// Configure the memory address (note that M1AR is used for double-buffer mode) 0x24000040

	DMA1_Stream1->CR |= DMA_SxCR_EN;				// Enable DMA and wait
	wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
	  wait_loop_index--;
	}

	ADC1->CR |= ADC_CR_ADSTART;						// Start ADC
}



void TriggerADC1()
{
	ADC1->CR |= ADC_CR_ADSTART;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1;
	DMA1_Stream1->CR |= DMA_SxCR_EN;
}


void InitADC2()
{
	// Initialize ADC peripheral
	DMA1_Stream2->CR &= ~DMA_SxCR_EN;
	DMA1_Stream2->CR |= DMA_SxCR_CIRC;				// Circular mode to keep refilling buffer
	DMA1_Stream2->CR |= DMA_SxCR_MINC;				// Memory in increment mode
	DMA1_Stream2->CR |= DMA_SxCR_PSIZE_0;			// Peripheral size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream2->CR |= DMA_SxCR_MSIZE_0;			// Memory size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream2->CR |= DMA_SxCR_PL_0;				// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	DMA1_Stream2->FCR &= ~DMA_SxFCR_FTH;			// Disable FIFO Threshold selection
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF2_Pos;		// clear all five interrupts for this stream

	DMAMUX1_Channel2->CCR |= 10; 					// DMA request MUX input 10 = adc2_dma (See p.695)
	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF2; // Channel 2 Clear synchronization overrun event flag

	ADC2->CR &= ~ADC_CR_DEEPPWD;					// Deep power down: 0: ADC not in deep-power down	1: ADC in deep-power-down (default reset state)
	ADC2->CR |= ADC_CR_ADVREGEN;					// Enable ADC internal voltage regulator

	// Wait until voltage regulator settled
	volatile uint32_t wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}
	while ((ADC2->CR & ADC_CR_ADVREGEN) != ADC_CR_ADVREGEN) {}

	ADC2->CFGR |= ADC_CFGR_CONT;					// 1: Continuous conversion mode for regular conversions
	ADC2->CFGR |= ADC_CFGR_OVRMOD;					// Overrun Mode 1: ADC_DR register is overwritten with the last conversion result when an overrun is detected.
	ADC2->CFGR |= ADC_CFGR_DMNGT;					// Data Management configuration 11: DMA Circular Mode selected

	// Boost mode 1: Boost mode on. Must be used when ADC clock > 20 MHz.
	ADC2->CR |= ADC_CR_BOOST_1;						// Note this sets reserved bit according to SFR - HAL has notes about silicon revision
	ADC2->SQR1 |= (ADC2_BUFFER_LENGTH - 1);			// For scan mode: set number of channels to be converted

	// Start calibration
	ADC2->CR |= ADC_CR_ADCALLIN;					// Activate linearity calibration (as well as offset calibration)
	ADC2->CR |= ADC_CR_ADCAL;
	while ((ADC2->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL) {};

	/* Configure ADC Channels to be converted:
	0	PC5 ADC12_INP8		WET_DRY_MIX
	1	PB1 ADC12_INP5		DELAY_POT_L
	2	PA6 ADC12_INP3 		DELAY_CV_SCALED_R
	3	PB0 ADC12_INP9		FEEDBACK_POT
	4	PC4 ADC12_INP4		FEEDBACK_CV_SCALED
	5	PC1 ADC123_INP11 	FILTER_POT
	*/
	InitAdcPins(ADC2, {8, 5, 3, 9, 4, 11});

	// Enable ADC
	ADC2->CR |= ADC_CR_ADEN;
	while ((ADC2->ISR & ADC_ISR_ADRDY) == 0) {}

	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF2; // Channel 2 Clear synchronization overrun event flag
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF2_Pos;		// clear all five interrupts for this stream

	DMA1_Stream2->NDTR |= ADC2_BUFFER_LENGTH;		// Number of data items to transfer (ie size of ADC buffer)
	DMA1_Stream2->PAR = reinterpret_cast<uint32_t>(&(ADC2->DR));	// Configure the peripheral data register address 0x40022040
	DMA1_Stream2->M0AR = reinterpret_cast<uint32_t>(&adc.Mix);		// Configure the memory address (note that M1AR is used for double-buffer mode) 0x24000040

	DMA1_Stream2->CR |= DMA_SxCR_EN;				// Enable DMA and wait
	wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}

	ADC2->CR |= ADC_CR_ADSTART;						// Start ADC
}




void InitI2S() {
	/* Available I2S2 pins on AF5
	PB9  I2S2_WS
	PC3  I2S2_SDO
	PD3  I2S2_CK
	*/

	RCC->APB1LENR |= RCC_APB1LENR_SPI2EN;			//	Enable SPI clocks

	GpioPin::Init(GPIOB, 9, GpioPin::Type::AlternateFunction, 5);		// PB9: I2S2_WS [alternate function AF5]
	GpioPin::Init(GPIOD, 3, GpioPin::Type::AlternateFunction, 5);		// PD3 I2S2_CK [alternate function AF5]
	GpioPin::Init(GPIOC, 3, GpioPin::Type::AlternateFunction, 5);		// PC3 I2S2_SDO [alternate function AF5]

	// Configure SPI (Shown as SPI2->CGFR in SFR)
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SMOD;			// I2S Mode
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SCFG_1;			// I2S configuration mode: 00=Slave transmit; 01=Slave receive; 10=Master transmit*; 11=Master receive

	SPI2->I2SCFGR |= SPI_I2SCFGR_DATLEN_1;			// Data Length 00=16-bit; 01=24-bit; *10=32-bit
	SPI2->I2SCFGR |= SPI_I2SCFGR_CHLEN;				// Channel Length = 32bits

	SPI2->CFG1 |= SPI_CFG1_UDRCFG_1;				// In the event of underrun resend last transmitted data frame
	SPI2->CFG1 |= 0x1f << SPI_CFG1_DSIZE_Pos;		// Data size to 32 bits (FIFO holds 16 bytes = 4 x 32 bit words)
	SPI2->CFG1 |= 3 << SPI_CFG1_FTHLV_Pos;			// FIFO threshold level. 0001: 2-data; 0010: 3 data; 0011: 4 data

	/* I2S Clock
	000: pll1_q_ck clock selected as SPI/I2S1,2 and 3 kernel clock (default after reset)
	001: pll2_p_ck clock selected as SPI/I2S1,2 and 3 kernel clock
	010: pll3_p_ck clock selected as SPI/I2S1,2 and 3 kernel clock
	011: I2S_CKIN clock selected as SPI/I2S1,2 and 3 kernel clock
	100: per_ck clock selected as SPI/I2S1,2 and 3 kernel clock
	//RCC->D2CCIP1R |= RCC_D2CCIP1R_SPI123SEL;

	I2S Prescaler Clock calculations:
	FS = I2SxCLK / [(32*2)*((2*I2SDIV)+ODD))]

	I2S Clock = 200MHz:			200000000 / (32*2  * ((2 * 32) + 1)) = 48076.92
	I2S Clock = 240MHz:			240000000 / (32*2  * ((2 * 39) + 0)) = 48076.92
	I2S Clock = 300MHz:			280000000 / (32*2  * ((2 * 45) + 1)) = 48076.92
	I2S Clock = 320MHz: 		320000000 / (32*2  * ((2 * 52) + 0)) = 48076.92
	PER_CLK = 64MHz				64000000  / (32*2  * ((2 * 10) + 1)) = 47619.05

	Note timing problems experienced using both pll1_q_ck and pll2_p_ck when FMC controller is using PLL2
	*/
#define I2S_PER_CLK
#ifdef I2S_PLL2P_CLK
	// Use PLL2 clock - configured to 200MHz
	RCC->D2CCIP1R |= RCC_D2CCIP1R_SPI123SEL_0;
	SPI2->I2SCFGR |= (32 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 45 with Odd factor prescaler
	SPI2->I2SCFGR |= SPI_I2SCFGR_ODD;
#endif

#ifdef I2S_PER_CLK
	// Use peripheral clock - configured to internal HSI at 64MHz
	RCC->D2CCIP1R |= RCC_D2CCIP1R_SPI123SEL_2;
	SPI2->I2SCFGR |= (10 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 45 with Odd factor prescaler
	SPI2->I2SCFGR |= SPI_I2SCFGR_ODD;
#endif

#ifdef I2S_DEFAULT
#if CPUCLOCK == 200 | CPUCLOCK == 400
	SPI2->I2SCFGR |= (32 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 32 with Odd factor prescaler
	SPI2->I2SCFGR |= SPI_I2SCFGR_ODD;
#elif CPUCLOCK == 280
	SPI2->I2SCFGR |= (45 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 45 with Odd factor prescaler
	SPI2->I2SCFGR |= SPI_I2SCFGR_ODD;
#elif CPUCLOCK == 320
	SPI2->I2SCFGR |= (52 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 52 with no Odd factor prescaler
#else
	SPI2->I2SCFGR |= (39 << SPI_I2SCFGR_I2SDIV_Pos);// Set I2SDIV to 39 with no Odd factor prescaler
#endif
#endif

	SPI2->IER |= (SPI_IER_TXPIE | SPI_IER_UDRIE);	// Enable interrupt when FIFO has free slot or underrun occurs
	NVIC_SetPriority(SPI2_IRQn, 2);					// Lower is higher priority
	NVIC_EnableIRQ(SPI2_IRQn);

	SPI2->CR1 |= SPI_CR1_SPE;						// Enable I2S

	SPI2->TXDR = (int32_t)0;						// Preload the FIFO
	SPI2->TXDR = (int32_t)0;
	SPI2->TXDR = (int32_t)0;
	SPI2->TXDR = (int32_t)0;

	SPI2->CR1 |= SPI_CR1_CSTART;					// Start I2S
}



void InitI2STimer()
{
	// Timer to try and control issues with I2S interrupt firing early
	// Uses APB1 timer clock which is Main Clock / 2 [280MHz / 2 = 140MHz] Tick is 2 * 7.14nS = 14.28nS
	RCC->APB1LENR |= RCC_APB1LENR_TIM4EN;
	TIM4->ARR = 65535;
	TIM4->PSC = 1;
}


void StartI2STimer()
{
	TIM4->EGR |= TIM_EGR_UG;
	TIM4->CR1 |= TIM_CR1_CEN;
}


uint32_t ReadI2STimer()
{
	return TIM4->CNT;
}

void InitDebugTimer()
{
	// Configure timer to use in internal debug timing - uses APB1 timer clock which is Main Clock / 2 [400MHz / 2 = 200MHz]
	// Each tick is 5ns with PSC 10nS - full range is 655.350 uS
	RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
	TIM3->ARR = 65535;
	TIM3->PSC = 1;

}


void StartDebugTimer()
{
	TIM3->EGR |= TIM_EGR_UG;
	TIM3->CR1 |= TIM_CR1_CEN;
}


float StopDebugTimer()
{
	// Return time running in microseconds
	const uint32_t count = TIM3->CNT;
	TIM3->CR1 &= ~TIM_CR1_CEN;
	return 0.01f * count;
}



void InitIO()
{
	GpioPin::Init(GPIOA, 7, GpioPin::Type::Input);				// PA7: tempo clock

	GpioPin::Init(GPIOC, 10, GpioPin::Type::Output);			// PC10: SCLK [ex LED1]
	GpioPin::Init(GPIOC, 12, GpioPin::Type::Output);			// PC12: MOSI [ex Delay Chorus]
	GpioPin::Init(GPIOC, 11, GpioPin::Type::Output);			// PC11: DC [ex LED2]


}


void DelayMS(uint32_t ms)
{
	// Crude delay system
	const uint32_t now = SysTickVal;
	while (now + ms > SysTickVal) {};
}


uint32_t __attribute__((section (".ram_d1_data"))) blankData;


void InitMDMA()
{
	// Initialises MDMA to background transfers of data to draw buffers for off-cpu blanking
	RCC->AHB3ENR |= RCC_AHB3ENR_MDMAEN;
	MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
	MDMA_Channel0->CCR |= MDMA_CCR_PL_0;			// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	MDMA_Channel0->CTCR |= MDMA_CTCR_DSIZE_1;		// Destination data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_SSIZE_1;		// Source data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_DINC_1;		// 10: Destination address pointer is incremented after each data transfer
	//MDMA_Channel0->CTCR |= MDMA_CTCR_SINC_1;		// 10: Source address pointer is incremented after each data transfer
	MDMA_Channel0->CTCR |= MDMA_CTCR_DINCOS_1;		// Destination increment - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	//MDMA_Channel0->CTCR |= MDMA_CTCR_SINCOS_1;		// Source increment - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_BWM;			// Bufferable Write Mode
	MDMA_Channel0->CTCR |= MDMA_CTCR_SWRM;			// Software Request Mode
	MDMA_Channel0->CTCR |= MDMA_CTCR_TRGM;			// 01: Each MDMA request triggers a block transfer

	MDMA_Channel0->CTBR &= ~MDMA_CTBR_SBUS;			// Source: AXI Bus used for SRAM
	//MDMA_Channel0->CTBR |= MDMA_CTBR_DBUS;			// Destination: AHB Bus used for addresses starting 0x20xxxxxx (DTCMRAM)

	blankData = 0;
	MDMA_Channel0->CSAR = (uint32_t)&blankData;		// Configure the source address

	MDMA_Channel0->CCR |= MDMA_CCR_BTIE;			// Enable Block Transfer complete interrupt

	NVIC_SetPriority(MDMA_IRQn, 0x3);				// Lower is higher priority
	NVIC_EnableIRQ(MDMA_IRQn);
}


void MDMATransfer(const uint16_t* destAddr, const uint32_t bytes)
{
	MDMA_Channel0->CTCR |= ((bytes - 1) << MDMA_CTCR_TLEN_Pos);	// Transfer length in bytes - 1
	MDMA_Channel0->CBNDTR |= (bytes << MDMA_CBNDTR_BNDT_Pos);	// Number of bytes in a block

	MDMA_Channel0->CDAR = (uint32_t)destAddr;		// Configure the destination address

	MDMA_Channel0->CCR |= MDMA_CCR_EN;				// Enable DMA
	MDMA_Channel0->CCR |= MDMA_CCR_SWRQ;			// Software Activate the request (fires interrupt when complete)
}

