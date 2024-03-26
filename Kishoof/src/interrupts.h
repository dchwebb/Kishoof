void OTG_FS_IRQHandler(void) {
	usb.InterruptHandler();
}

void __attribute__((optimize("O0"))) TinyDelay() {
	for (int x = 0; x < 200; ++x);
}

uint32_t underrun = 0;

// I2S Interrupt
void SPI2_IRQHandler()
{
	if ((SPI2->SR & SPI_SR_UDR) == SPI_SR_UDR) {		// Check for Underrun condition
		SPI2->IFCR |= SPI_IFCR_UDRC;					// Clear underrun condition
		++underrun;
		return;
	}

	wavetable.CalcSample();

	// NB It appears we need something here to add a slight delay or the interrupt sometimes fires twice
	TinyDelay();

	TriggerADC1();
}


void MDMA_IRQHandler()
{
	// fires when MDMA Flash to memory transfer has completed
	if (MDMA->GISR0 & MDMA_GISR0_GIF0) {
		MDMA_Channel0->CIFCR |= MDMA_CIFCR_CBTIF;		// Clear transfer complete interrupt flag
		wavetable.bufferClear = true;
	}
}

// System interrupts
void NMI_Handler(void) {}

void HardFault_Handler(void) {
	while (1) {}
}
void MemManage_Handler(void) {
	while (1) {}
}
void BusFault_Handler(void) {
	while (1) {}
}
void UsageFault_Handler(void) {
	while (1) {}
}

void SVC_Handler(void) {}

void DebugMon_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {
	++SysTickVal;
}

/*
// USART Decoder
void USART3_IRQHandler() {
	//if ((USART3->ISR & USART_ISR_RXNE_RXFNE) != 0 && !uartCmdRdy) {
	if (!uartCmdRdy) {
		uartCmd[uartCmdPos] = USART3->RDR; 				// accessing RDR automatically resets the receive flag
		if (uartCmd[uartCmdPos] == 10) {
			uartCmdRdy = true;
			uartCmdPos = 0;
		} else {
			uartCmdPos++;
		}
	}
}*/
