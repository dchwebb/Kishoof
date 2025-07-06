#include "uartHandler.h"
#include "usb.h"
#include "FatTools.h"

UART uart;

void UART::Init() {
	// Debug UART pins: PD5 (UART2 TX) and PD6 (UART2 RX)

	RCC->APB1LENR |= RCC_APB1LENR_USART2EN;			// USART2 clock enable

	GpioPin::Init(GPIOD, 5, GpioPin::Type::AlternateFunction, 7);		// Alternate function on PD5 for USART2_TX is AF7
	GpioPin::Init(GPIOD, 6, GpioPin::Type::AlternateFunction, 7);		// Alternate function on PD6 for USART2_RX is AF7

	// By default clock source is muxed to peripheral clock 1 which is system clock / 4 (change clock source in RCC->D2CCIP2R)
	// Calculations depended on oversampling mode set in CR1 OVER8. Default = 0: Oversampling by 16
	int USARTDIV = (SystemCoreClock / 2) / 230400;	//clk / desired_baud
	USART2->BRR |= USARTDIV & USART_BRR_DIV_MANTISSA_Msk;
	USART2->CR1 &= ~USART_CR1_M;					// 0: 1 Start bit, 8 Data bits, n Stop bit; 	1: 1 Start bit, 9 Data bits, n Stop bit
	USART2->CR1 |= USART_CR1_RE;					// Receive enable
	USART2->CR1 |= USART_CR1_TE;					// Transmitter enable

	// Set up interrupts
	USART2->CR1 |= USART_CR1_RXNEIE;
	NVIC_SetPriority(USART2_IRQn, 3);				// Lower is higher priority
	NVIC_EnableIRQ(USART2_IRQn);

	USART2->CR1 |= USART_CR1_UE;					// USART Enable
}


void UART::SendChar(char c) {
	while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
	USART2->TDR = c;
}

void UART::SendString(const char* s) {
	char c = s[0];
	uint8_t i = 0;
	while (c) {
		while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART2->TDR = c;
		c = s[++i];
	}
}

void UART::SendString(const std::string& s) {
	for (char c : s) {
		while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART2->TDR = c;
	}
}


void UART::ProcessCommand()
{
	if (!commandReady) {
		return;
	}
	std::string_view cmd {command};

	if (cmd.compare("dirdetails\n") == 0) {				// Get detailed FAT directory info
		fatTools.PrintDirInfo();

	} else if (cmd.compare("msc") == 0) {					// MSC debug out
		usb.msc.PrintDebug();

	} else if (cmd.compare("usb") == 0) {					// USB debug out
#if (USB_DEBUG)
		usb.OutputDebug();
#endif
	} else {
		SendString("Unrecognised command\r\n");
	}

	commandReady = false;
}


extern "C" {

// USART Decoder
void USART2_IRQHandler() {
	if (!uart.commandReady) {
		const uint32_t recData = USART2->RDR;					// Note that 32 bits must be read to clear the receive flag
		uart.command[uart.cmdPos] = (char)recData; 				// accessing RDR automatically resets the receive flag
		if (uart.command[uart.cmdPos] == 10) {
			uart.command[uart.cmdPos] = 0;
			uart.commandReady = true;
			uart.cmdPos = 0;
		} else {
			uart.cmdPos++;
		}
	}
}
}


