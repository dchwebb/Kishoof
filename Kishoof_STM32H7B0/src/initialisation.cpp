#include "stm32h7b0xx.h"
#include "initialisation.h"



// Clock overview:
// I2S 				PLL2 P 				61.44 MHz
// SPI3 Display		PLL2 P
// ADC		 		Peripheral Clock 	8 MHz
// OCTOSPI			PPL2 R				153.6 MHz

// Main clock 8MHz (HSE) / 2 (M) * 140 (N) / 2 (P) = 280MHz
#define PLL_M1 2
#define PLL_N1 140
#define PLL_P1 2

// PPL2P used for I2S: 		8MHz / 5 (M) * 192 (N) / 5 (P) = 61.44 MHz
// PPL2R used for OctoSPI: 	8MHz / 5 (M) * 192 (N) / 3 (P) = 102.4 MHz
#define PLL_M2 5
#define PLL_N2 192
#define PLL_P2 5
#define PLL_R2 2


void InitClocks()
{
	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;			// Enable System configuration controller clock

	// Voltage scaling - see Datasheet page 78. VOS0 > 225MHz; VOS1 > 160MHz; VOS2 > 88MHz; VOS3 < 88MHz
	//PWR->CR3 &= ~PWR_CR3_SCUEN;						// Supply configuration update enable - this must be deactivated or VOS ready does not come on
	PWR->CR3 |= PWR_CR3_BYPASS;						// power management unit bypassed: Must be set before changing VOS

	PWR->SRDCR |= PWR_SRDCR_VOS;					// Configure voltage scaling level 0 (Highest)
	while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0);	// Check Voltage ready 1= Ready, voltage level at or above VOS selected level

	RCC->CR |= RCC_CR_HSEON;						// Turn on external oscillator
	while ((RCC->CR & RCC_CR_HSERDY) == 0);			// Wait till HSE is ready

	// Clock source to High speed external and main (M) dividers
	RCC->PLLCKSELR = RCC_PLLCKSELR_PLLSRC_HSE |
	                 PLL_M1 << RCC_PLLCKSELR_DIVM1_Pos;

	// PLL 1 dividers
	RCC->PLL1DIVR = (PLL_N1 - 1) << RCC_PLL1DIVR_N1_Pos |
			        (PLL_P1 - 1) << RCC_PLL1DIVR_P1_Pos;

	RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_1;			// 10: The PLL1 input (ref1_ck) clock range frequency is between 4 and 8 MHz (Will be 4MHz for 8MHz clock divided by 2)
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL;		// 0: Wide VCO range:128 to 560 MHz (default); 1: Medium VCO range: 150 to 420 MHz
	RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN;			// Enable P divider output

	RCC->CR |= RCC_CR_PLL1ON;						// Turn on main PLL
	while ((RCC->CR & RCC_CR_PLL1RDY) == 0);		// Wait till PLL is ready

	// PLL 2 dividers
	RCC->PLLCKSELR |= PLL_M2 << RCC_PLLCKSELR_DIVM2_Pos;

	RCC->PLL2DIVR = (PLL_N2 - 1) << RCC_PLL2DIVR_N2_Pos |
					(PLL_P2 - 1) << RCC_PLL2DIVR_P2_Pos |
					(PLL_R2 - 1) << RCC_PLL2DIVR_R2_Pos;

	RCC->PLLCFGR |= RCC_PLLCFGR_PLL2RGE_1 |			// 01: The PLL2 input (ref1_ck) clock range frequency is between 4 and 8 MHz
					RCC_PLLCFGR_PLL2VCOSEL |		// 1: Medium VCO range:150 to 420 MHz
					RCC_PLLCFGR_DIVP2EN |			// Enable P divider output
					RCC_PLLCFGR_DIVR2EN;			// Enable R divider output

	RCC->CR |= RCC_CR_PLL2ON;						// Turn on PLL 2
	while ((RCC->CR & RCC_CR_PLL2RDY) == 0);		// Wait till PLL is ready

	// Peripheral scalers
	RCC->CDCCIPR |= RCC_CDCCIPR_CKPERSEL_1;			// Peripheral clock to HSE (8MHz)
	RCC->CDCFGR1 |= RCC_CDCFGR1_CDPPRE_DIV2;		// APB3 Clocks
	RCC->CDCFGR2 |= RCC_CDCFGR2_CDPPRE1_DIV2;		// APB1 Clocks
	RCC->CDCFGR2 |= RCC_CDCFGR2_CDPPRE2_DIV2;		// APB2 Clocks
	RCC->SRDCFGR |= RCC_SRDCFGR_SRDPPRE_DIV2;		// APB4 Clocks

	// By default Flash latency is set to 6 wait states - See page 161
	FLASH->ACR = FLASH_ACR_WRHIGHFREQ |
				(FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_6WS;
	while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_6WS);

	RCC->CFGR |= RCC_CFGR_SW_PLL1;					// System clock switch: 011: PLL1 selected as system clock
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != (RCC_CFGR_SW_PLL1 << RCC_CFGR_SWS_Pos));		// Wait until PLL has been selected as system clock source


	SystemCoreClockUpdate();						// Update SystemCoreClock (system clock frequency) derived from settings of oscillators, prescalers and PLL
}


void InitHardware()
{
	InitSysTick();
	InitCache();
	InitIO();										// Initialise switches and LEDs
	InitMDMA();
	InitADC();
	InitDebugTimer();
	InitI2STimer();
	InitDisplaySPI();
	InitEncoders();
	InitOctoSPI();
}


void InitCache()
{
	// Use the Memory Protection Unit (MPU) to set up a region of memory with data caching disabled for use with DMA buffers
	MPU->RNR = 0;									// Memory region number
	extern uint32_t _dma_addr;						// Get the start of the dma buffer from the linker
	MPU->RBAR = reinterpret_cast<uint32_t>(&_dma_addr);	// Store the address of the ADC_array into the region base address register

	MPU->RASR = (0b11  << MPU_RASR_AP_Pos)   |		// All access permitted
				(0b001 << MPU_RASR_TEX_Pos)  |		// Type Extension field: See truth table on p228 of Cortex M7 programming manual
				(1     << MPU_RASR_S_Pos)    |		// Shareable: provides data synchronization between bus masters. Eg a processor with a DMA controller
				(0     << MPU_RASR_C_Pos)    |		// Cacheable
				(0     << MPU_RASR_B_Pos)    |		// Bufferable (ignored for non-cacheable configuration)
				(17    << MPU_RASR_SIZE_Pos) |		// 256KB - (size is log 2(mem size) - 1 ie 2^18 = 256k)
				(1     << MPU_RASR_ENABLE_Pos);		// Enable MPU region


	MPU->CTRL |= (1 << MPU_CTRL_PRIVDEFENA_Pos) |	// Enable PRIVDEFENA - this allows use of default memory map for memory areas other than those specific regions defined above
				 (1 << MPU_CTRL_ENABLE_Pos);		// Enable the MPU

	// Enable data and instruction caches
	SCB_EnableDCache();
	SCB_EnableICache();
}


void InitSysTick()
{
	SysTick_Config(SystemCoreClock / sysTickInterval);		// gives 1ms
	NVIC_SetPriority(SysTick_IRQn, 0);
}


void InitDisplaySPI()
{
	// SPI123 configured to use PLL2P 61.44 MHz (for I2S)
	// Maximum frequency should be 100MHz - see GC9A01 manual p. 189
	RCC->APB1LENR |= RCC_APB1LENR_SPI3EN;

	GpioPin::Init(GPIOC, 10, GpioPin::Type::AlternateFunction, 6, GpioPin::DriveStrength::High);		// PC10: SPI3_SCK AF6
	GpioPin::Init(GPIOC, 12, GpioPin::Type::AlternateFunction, 6, GpioPin::DriveStrength::High);		// PC12: SPI3_MOSI AF6
	//GpioPin::Init(GPIOA, 15, GpioPin::Type::AlternateFunction, 6, GpioPin::DriveStrength::High);			// PA15: SPI3_SS AF6

	// Configure SPI
	SPI3->CFG2 |= SPI_CFG2_COMM_0;					// 00: full-duplex, *01: simplex transmitter, 10: simplex receiver, 11: half-duplex
	SPI3->CFG2 |= SPI_CFG2_SSM;						// Software slave management: When SSM bit is set, NSS pin input is replaced with the value from the SSI bit
	SPI3->CR1 |= SPI_CR1_SSI;						// Internal slave select
	SPI3->CFG2 |= SPI_CFG2_SSOM;					// SS output management in master mode
	SPI3->CFG1 |= SPI_CFG1_MBR_0;					// Master Baud rate p2138: 001: ck/4; 010: ck/8; 011: ck/16; 100: ck/32; 101: ck/64
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

	DMAMUX1_Channel0->CCR |= 62; 					// DMA request MUX input 62 = SPI3_TX (See p.653)
	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF0; // Clear synchronization overrun event flag
}


void InitAdcPins(ADC_TypeDef* ADC_No, std::initializer_list<uint8_t> channels)
{
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


void InitADC()
{
	// Settings used for both ADC1 and ADC2

	// Configure clocks
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;			// GPIO port clock
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
	RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;

	RCC->AHB1ENR |= RCC_AHB1ENR_ADC12EN;

	RCC->SRDCCIPR |= RCC_SRDCCIPR_ADCSEL_1;			// ADC clock selection: 10: per_ck clock (HSE 8MHz)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	ADC12_COMMON->CCR |= ADC_CCR_PRESC_0;			// Set prescaler to ADC clock divided by 2 (if 8MHz = 4MHz)

	InitADC1();
	InitADC2();
}


/*-----------------------------------------------------------------------------------------------------------------
ADC1:
	PA0 ADC1_INP16 Wavetable_Pos_B_Pot
	PA1 ADC1_INP17 Wavetable_Pos_A_Trm
	PA2 ADC1_INP14 AudioIn
	PA3 ADC1_INP15 WarpCV
	PA4 ADC1_INP18 Pitch_CV_Scaled
	PA5 ADC1_INP19 WavetablePosB_CV
ADC2:
	PA6 ADC12_INP3 VcaCV
	PA7 ADC12_INP7 WavetablePosA_CV
	PB0 ADC12_INP9 Warp_Type_Pot
	PB1 ADC12_INP5 Wavetable_Pos_A_Pot
	PC0 ADC12_INP10 Warp_Amt_Trm
	PC1	ADC12_INP11	Warp_Amt_Pot
*/

void InitADC1()
{
	// Initialize ADC peripheral
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
	DMA1_Stream1->CR |= DMA_SxCR_CIRC;				// Circular mode to keep refilling buffer
	DMA1_Stream1->CR |= DMA_SxCR_MINC;				// Memory in increment mode
	DMA1_Stream1->CR |= DMA_SxCR_PSIZE_0;			// Peripheral size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream1->CR |= DMA_SxCR_MSIZE_0;			// Memory size: 8 bit; 01 = 16 bit; 10 = 32 bit
	DMA1_Stream1->CR |= DMA_SxCR_PL_0;				// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	DMA1_Stream1->FCR &= ~DMA_SxFCR_FTH;			// Disable FIFO Threshold selection
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF1_Pos;		// clear all five interrupts for this stream

	DMAMUX1_Channel1->CCR |= 9; 					// DMA request MUX input 9 = adc1_dma (See p.653)
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
	ADC1->CFGR |= ADC_CFGR_DMNGT;					// Data Management configuration 01: DMA One Shot Mode selected, 11: DMA Circular Mode selected

	//00: ADC clock ≤ 6.25 MHz; 01: 6.25 MHz < clk ≤ 12.5 MHz; 10: 12.5 MHz < clk ≤ 25.0 MHz; 11: 25.0 MHz < clock ≤ 50.0 MHz
	ADC1->CR |= ADC_CR_BOOST_0;
	ADC1->SQR1 |= (ADC1_BUFFER_LENGTH - 1);			// For scan mode: set number of channels to be converted

	// Start calibration
	ADC1->CR |= ADC_CR_ADCALLIN;					// Activate linearity calibration (as well as offset calibration)
	ADC1->CR |= ADC_CR_ADCAL;
	while ((ADC1->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL) {};


	/*--------------------------------------------------------------------------------------------
	Configure ADC Channels to be converted:
	PA0 ADC1_INP16 Wavetable_Pos_B_Pot
	PA1 ADC1_INP17 Wavetable_Pos_A_Trm
	PA2 ADC1_INP14 AudioIn
	PA3 ADC1_INP15 WarpCV
	PA4 ADC1_INP18 Pitch_CV_Scaled
	PA5 ADC1_INP19 WavetablePosB_CV
	*/
	InitAdcPins(ADC1, {16, 17, 14, 15, 18, 19});

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

	DMAMUX1_Channel2->CCR |= 10; 					// DMA request MUX input 10 = adc2_dma (See p.653)
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
	ADC2->CR |= ADC_CR_BOOST_0;						// Note this sets reserved bit according to SFR - HAL has notes about silicon revision
	ADC2->SQR1 |= (ADC2_BUFFER_LENGTH - 1);			// For scan mode: set number of channels to be converted

	// Start calibration
	ADC2->CR |= ADC_CR_ADCALLIN;					// Activate linearity calibration (as well as offset calibration)
	ADC2->CR |= ADC_CR_ADCAL;
	while ((ADC2->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL) {};

	/* Configure ADC Channels to be converted:
	PA6 ADC12_INP3 VcaCV
	PA7 ADC12_INP7 WavetablePosA_CV
	PB0 ADC12_INP9 Warp_Type_Pot
	PB1 ADC12_INP5 Wavetable_Pos_A_Pot
	PC0 ADC12_INP10 Warp_Amt_Trm
	PC1	ADC12_INP11	Warp_Amt_Pot
	*/
	InitAdcPins(ADC2, {3, 7, 9, 5, 10, 11});

	// Enable ADC
	ADC2->CR |= ADC_CR_ADEN;
	while ((ADC2->ISR & ADC_ISR_ADRDY) == 0) {}

	DMAMUX1_ChannelStatus->CFR |= DMAMUX_CFR_CSOF2; // Channel 2 Clear synchronization overrun event flag
	DMA1->LIFCR = 0x3F << DMA_LIFCR_CFEIF2_Pos;		// clear all five interrupts for this stream

	DMA1_Stream2->NDTR |= ADC2_BUFFER_LENGTH;		// Number of data items to transfer (ie size of ADC buffer)
	DMA1_Stream2->PAR = reinterpret_cast<uint32_t>(&(ADC2->DR));	// Configure the peripheral data register address 0x40022040
	DMA1_Stream2->M0AR = reinterpret_cast<uint32_t>(&adc.VcaCV);		// Configure the memory address (note that M1AR is used for double-buffer mode) 0x24000040

	DMA1_Stream2->CR |= DMA_SxCR_EN;				// Enable DMA and wait
	wait_loop_index = (SystemCoreClock / (100000UL * 2UL));
	while (wait_loop_index != 0UL) {
		wait_loop_index--;
	}

	ADC2->CR |= ADC_CR_ADSTART;						// Start ADC
}



void InitI2S()
{
	RCC->APB1LENR |= RCC_APB1LENR_SPI2EN;			//	Enable SPI clocks

	GpioPin::Init(GPIOB, 12, GpioPin::Type::AlternateFunction, 5);		// PB12: I2S2_WS [alternate function AF5]
	GpioPin::Init(GPIOB, 13, GpioPin::Type::AlternateFunction, 5);		// PB13: I2S2_CK [alternate function AF5]
	GpioPin::Init(GPIOB, 15, GpioPin::Type::AlternateFunction, 5);		// PB15: I2S2_SDO [alternate function AF5]

	// Configure SPI (Shown as SPI2->CGFR in SFR)
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SMOD;			// I2S Mode
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SCFG_1;			// I2S configuration mode: 00=Slave transmit; 01=Slave receive; 10=Master transmit*; 11=Master receive

	SPI2->I2SCFGR |= SPI_I2SCFGR_DATLEN_1;			// Data Length 00=16-bit; 01=24-bit; *10=32-bit
	SPI2->I2SCFGR |= SPI_I2SCFGR_CHLEN;				// Channel Length = 32bits

	SPI2->CFG1 |= SPI_CFG1_UDRCFG_1;				// In the event of underrun resend last transmitted data frame
	SPI2->CFG1 |= 0x1f << SPI_CFG1_DSIZE_Pos;		// Data size to 32 bits (FIFO holds 16 bytes = 4 x 32 bit words)
	SPI2->CFG1 |= 1 << SPI_CFG1_FTHLV_Pos;			// FIFO threshold level. 0001: 2-data; 0010: 3 data; 0011: 4 data

	/* I2S Clock
		PLL2: ((8MHz / 5) * 192 / 5) = 61.44 MHz
		I2S Clock: 61.44 MHz / (64  * ((2 * 10) + 0)) = 48 KHz
	*/

	RCC->CDCCIP1R |= RCC_CDCCIP1R_SPI123SEL_0;		// 001: pll2_p_ck clock selected as SPI/I2S1,2 and 3 kernel clock
	SPI2->I2SCFGR |= (10 << SPI_I2SCFGR_I2SDIV_Pos);	// Set I2SDIV to 2 with Odd factor prescaler
	//SPI2->I2SCFGR |= SPI_I2SCFGR_ODD;

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
	// Configure timer to use in internal debug timing - uses APB1 timer clock which is Main Clock [280MHz]
	// Each tick is 4ns with PSC 12nS - full range is 786.42 uS
	RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
	TIM3->ARR = 65535;
	TIM3->PSC = 2;
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
	return 0.0012 * count;
}



void InitIO()
{
	GpioPin::Init(GPIOE, 3, GpioPin::Type::Input);				// PE3: Warp_Polarity_Btn

}


void DelayMS(uint32_t ms)
{
	// Crude delay system
	const uint32_t now = SysTickVal;
	while (now + ms > SysTickVal) {};
}


void InitMDMA()
{
	// Initialises MDMA to background transfers of data to draw buffers for off-cpu blanking
	RCC->AHB3ENR |= RCC_AHB3ENR_MDMAEN;
	MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
	MDMA_Channel0->CCR |= MDMA_CCR_PL_0;			// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	MDMA_Channel0->CTCR |= MDMA_CTCR_DSIZE_1;		// Destination data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_SSIZE_1;		// Source data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_DINC_1;		// 10: Destination address pointer is incremented after each data transfer
	MDMA_Channel0->CTCR |= MDMA_CTCR_DINCOS_1;		// Destination increment - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel0->CTCR |= MDMA_CTCR_BWM;			// Bufferable Write Mode
	MDMA_Channel0->CTCR |= MDMA_CTCR_SWRM;			// Software Request Mode
	MDMA_Channel0->CTCR |= MDMA_CTCR_TRGM;			// 01: Each MDMA request triggers a block transfer

	// See p. 131: Main RAM is on AXI Bus
	MDMA_Channel0->CTBR &= ~MDMA_CTBR_SBUS;			// Source: 0* System/AXI bus; 1 AHB bus/TCM
	MDMA_Channel0->CTBR &= ~MDMA_CTBR_DBUS;			// Destination: 0* System/AXI bus; 1 AHB bus/TCM

	MDMA_Channel0->CCR |= MDMA_CCR_BTIE;			// Enable Block Transfer complete interrupt

	// Channel 1:  Uses MDMA to background transfers of data from octoSPI Flash to RAM
	MDMA_Channel1->CCR &= ~MDMA_CCR_EN;
	MDMA_Channel1->CCR |= MDMA_CCR_PL_0;			// Priority: 00 = low; 01 = Medium; 10 = High; 11 = Very High

	MDMA_Channel1->CTCR |= MDMA_CTCR_DSIZE_1;		// Destination data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel1->CTCR |= MDMA_CTCR_SSIZE_1;		// Source data size - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel1->CTCR |= MDMA_CTCR_DINC_1;		// 10: Destination address pointer is incremented after each data transfer
	MDMA_Channel1->CTCR |= MDMA_CTCR_SINC_1;		// 10: Source address pointer is incremented after each data transfer
	MDMA_Channel1->CTCR |= MDMA_CTCR_DINCOS_1;		// Destination increment - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel1->CTCR |= MDMA_CTCR_SINCOS_1;		// Source increment - 00: 8-bit, 01: 16-bit, *10: 32-bit, 11: 64-bit
	MDMA_Channel1->CTCR |= MDMA_CTCR_BWM;			// Bufferable Write Mode
	MDMA_Channel1->CTCR |= MDMA_CTCR_SWRM;			// Software Request Mode
	MDMA_Channel1->CTCR |= MDMA_CTCR_TRGM;			// 01: Each MDMA request triggers a block transfer

	MDMA_Channel1->CTBR &= ~MDMA_CTBR_SBUS;			// Source: 0* System/AXI bus; 1 AHB bus/TCM
	MDMA_Channel1->CTBR &= ~MDMA_CTBR_DBUS;			// Destination: 0* System/AXI bus; 1 AHB bus/TCM

	MDMA_Channel1->CCR |= MDMA_CCR_BTIE;			// Enable Block Transfer complete interrupt


	NVIC_SetPriority(MDMA_IRQn, 0x3);				// Lower is higher priority
	NVIC_EnableIRQ(MDMA_IRQn);
}


void MDMATransfer(MDMA_Channel_TypeDef* channel, const uint8_t* srcAddr, const uint8_t* destAddr, const uint32_t bytes)
{
	channel->CTCR |= ((bytes - 1) << MDMA_CTCR_TLEN_Pos);	// Transfer length in bytes - 1
	channel->CBNDTR |= (bytes << MDMA_CBNDTR_BNDT_Pos);	// Number of bytes in a block

	channel->CSAR = (uint32_t)srcAddr;				// Configure the source address
	channel->CDAR = (uint32_t)destAddr;				// Configure the destination address

	channel->CCR |= MDMA_CCR_EN;					// Enable DMA
	channel->CCR |= MDMA_CCR_SWRQ;					// Software Activate the request (fires interrupt when complete)
}


void InitEncoders()
{
	// Encoder: button on PE4, up/down on PC6 and PC7
	GpioPin::Init(GPIOE, 4, GpioPin::Type::InputPullup);				// PE4: Button
	GpioPin::Init(GPIOC, 6, GpioPin::Type::AlternateFunction, 3);		// PC6: TIM8_CH1 AF3
	GpioPin::Init(GPIOC, 7, GpioPin::Type::AlternateFunction, 3);		// PC7: TIM8_CH2 AF3

	RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;			// Enable system configuration clock: used to manage external interrupt line connection to GPIOs
	// Encoder using timer functionality - PC6 and PC7
	GPIOC->PUPDR |= GPIO_PUPDR_PUPD6_0 |			// Set encoder pins to pull up
					GPIO_PUPDR_PUPD7_0;

	RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;				// Enable Timer 8
	TIM8->PSC = 0;									// Set prescaler
	TIM8->ARR = 0xFFFF; 							// Set auto reload register to max
	TIM8->SMCR |= TIM_SMCR_SMS_0 |TIM_SMCR_SMS_1;	// SMS=011 for counting on both TI1 and TI2 edges
	TIM8->SMCR |= TIM_SMCR_ETF;						// Enable digital filter
	TIM8->CNT = 32000;								// Start counter at mid way point
	TIM8->CR1 |= TIM_CR1_CEN;

}


void InitOctoSPI()
{
	GpioPin::Init(GPIOB, 2, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PB2  OCTOSPI_CLK AF9
	GpioPin::Init(GPIOC, 9, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PC9  OCTOSPI_IO0 AF9
	GpioPin::Init(GPIOD, 12, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);	// PD12 OCTOSPI_IO1 AF9
	GpioPin::Init(GPIOC, 2, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);		// PC2  OCTOSPI_IO2 AF9
	GpioPin::Init(GPIOD, 13, GpioPin::Type::AlternateFunction, 9, GpioPin::DriveStrength::VeryHigh);	// PD13 OCTOSPI_IO3 AF9
	GpioPin::Init(GPIOE, 7, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh); 	// PE7  OCTOSPI_IO4 AF10
	GpioPin::Init(GPIOE, 8, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh); 	// PE8  OCTOSPI_IO5 AF10
	GpioPin::Init(GPIOE, 9, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);	// PE9  OCTOSPI_IO6 AF10
	GpioPin::Init(GPIOE, 10, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);	// PE10 OCTOSPI_IO7 AF10
	GpioPin::Init(GPIOE, 11, GpioPin::Type::AlternateFunction, 11, GpioPin::DriveStrength::VeryHigh);	// PE11 OCTOSPI_NCS AF11
	GpioPin::Init(GPIOC, 5, GpioPin::Type::AlternateFunction, 10, GpioPin::DriveStrength::VeryHigh);	// PC5  OCTOSPI_DQS AF10

	RCC->AHB3ENR |= RCC_AHB3ENR_OSPI1EN;
	RCC->CDCCIPR |= RCC_CDCCIPR_OCTOSPISEL_1;				// kernel clock 100MHz: 00 rcc_hclk3; 01 pll1_q_ck; *10 pll2_r_ck; 11 per_ck

	// Various settings below taken from AN5050
	OCTOSPI1->CR |= OCTOSPI_CR_FMODE_0;						// Mode: 00: Indirect-write; 01: Indirect-read; 10: Automatic status-polling; 11: Memory-mapped
	OCTOSPI1->DCR1 |= (28 << OCTOSPI_DCR1_DEVSIZE_Pos);		// No. of bytes = 2^(DEVSIZE+1): 512Mb = 2^29 bytes
	OCTOSPI1->DCR1 |= (0b01 << OCTOSPI_DCR1_MTYP_Pos);		// 001: Macronix mode; 100: HyperBus memory mode; 101: HyperBus register mode
	OCTOSPI1->DCR1 |= (3 << OCTOSPI_DCR1_CSHT_Pos);			// CSHT + 1: min no CLK cycles where NCS must stay high between commands (Min 10ns for reads 40ns for writes)
	OCTOSPI1->DCR1 &= ~OCTOSPI_DCR1_CKMODE;					// Clock mode 0: CLK is low when NCS high

	OCTOSPI1->DCR2 |= (1 << OCTOSPI_DCR2_PRESCALER_Pos);	// Set prescaler to n + 1 => 153.6 MHz / (n + 1) - tested to 76.8MHz

}


void JumpToBootloader()
{
	volatile uint32_t bootAddr = 0x1FF0A000;	// Set the address of the entry point to bootloader
	__disable_irq();							// Disable all interrupts
	SysTick->CTRL = 0;							// Disable Systick timer

	// Disable all peripheral clocks
	RCC->APB1LENR = 0;
	RCC->APB4ENR = 0;
	RCC->AHB1ENR = 0;
	RCC->APB2ENR = 0;
	RCC->AHB3ENR = 0;
	RCC->AHB4ENR = 0;

	for (uint32_t i = 0; i < 5; ++i) {			// Clear Interrupt Enable Register & Interrupt Pending Register
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	__enable_irq();								// Re-enable all interrupts
	void (*SysMemBootJump)() = (void(*)()) (*((uint32_t *) (bootAddr + 4)));	// Set up the jump to booloader address + 4
	__set_MSP(*(uint32_t *)bootAddr);			// Set the main stack pointer to the bootloader stack
	SysMemBootJump(); 							// Call the function to jump to bootloader location

	while (1) {
		// Code should never reach this loop
	}
}


bool vcaConnected = true;

void CheckVCA()
{
	// Routine to check if VCA is normalled to 3.3V (ie value of 3.3V measured for 10 ms)
	static uint32_t vcaNormalStart = 0;
	static uint32_t vcaNonNormalStart = 0;

	if (std::abs((int32_t)adc.VcaCV - 24000) > 100) {		// VCA adc is out of 3.3V range
		if (vcaNonNormalStart > 0 && SysTickVal - vcaNonNormalStart > 2) {
			vcaNormalStart = 0;
			vcaConnected = true;
		}
		if (vcaNonNormalStart == 0) {
			vcaNonNormalStart = SysTickVal;
		}
	} else  {
		if (vcaNormalStart > 0 && SysTickVal - vcaNormalStart > 10) {
			vcaNonNormalStart = 0;
			vcaConnected = false;
		}
		if (vcaNormalStart == 0) {
			vcaNormalStart = SysTickVal;
		}
	}
}




