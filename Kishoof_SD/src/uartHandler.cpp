#include "uartHandler.h"
#include "usb.h"
#include "GpioPin.h"
#include "fattools.h"

UART uart;

void UART::Init() {
	// Debug UART pins: PD8 (USART3 TX) and PD9 (USART3 RX)

	RCC->APB1LENR |= RCC_APB1LENR_USART3EN;			// USART7 clock enable

	GpioPin::Init(GPIOD, 8, GpioPin::Type::AlternateFunction, 7);
	GpioPin::Init(GPIOD, 9, GpioPin::Type::AlternateFunction, 7);

	// By default clock source is muxed to peripheral clock 1 which is system clock / 4 (change clock source in RCC->D2CCIP2R)
	// Calculations depended on oversampling mode set in CR1 OVER8. Default = 0: Oversampling by 16
	int USARTDIV = (SystemCoreClock / 4) / 230400;	// clk / desired_baud
	USART3->BRR |= USARTDIV & USART_BRR_DIV_MANTISSA_Msk;
	USART3->CR1 &= ~USART_CR1_M;						// 0: 1 Start bit, 8 Data bits, n Stop bit; 1: 1 Start bit, 9 Data bits, n Stop bit
	USART3->CR1 |= USART_CR1_RE;						// Receive enable
	USART3->CR1 |= USART_CR1_TE;						// Transmitter enable

	// Set up interrupts
	USART3->CR1 |= USART_CR1_RXNEIE;
	NVIC_SetPriority(USART3_IRQn, 6);				// Lower is higher priority
	NVIC_EnableIRQ(USART3_IRQn);

	USART3->CR1 |= USART_CR1_UE;						// UART Enable
}


void UART::SendChar(char c) {
	while ((USART3->ISR & USART_ISR_TXE_TXFNF) == 0);
	USART3->TDR = c;
}

void UART::SendString(const char* s) {
	char c = s[0];
	uint8_t i = 0;
	while (c) {
		while ((USART3->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART3->TDR = c;
		c = s[++i];
	}
}

void UART::SendString(const std::string& s) {
	for (char c : s) {
		while ((USART3->ISR & USART_ISR_TXE_TXFNF) == 0);
		USART3->TDR = c;
	}
}


void UART::ProcessCommand()
{
	if (!commandReady) {
		return;
	}
	std::string_view cmd {command};

	if (cmd.compare("dirdetails\n") == 0) {				// Get detailed FAT directory info
		//fatTools.PrintDirInfo();

	} else if (cmd.compare("format\n") == 0) {				// Get detailed FAT directory info
		SendString("Formatting ...\r\n");
		fatTools.Format();


#if (USB_DEBUG)
	} else if (cmd.compare("printdebug\n") == 0) {
		usb.OutputDebug();

	} else if (cmd.compare("debugon\n") == 0) {
		extern volatile bool debugStart;
		debugStart = true;
		SendString("Debug activated\r\n");
#endif

	} else {
		SendString("Unrecognised command\r\n");
	}

	commandReady = false;
}


extern "C" {

// USART Decoder
void USART3_IRQHandler() {
	if (!uart.commandReady) {
		const uint32_t recData = USART3->RDR;					// Note that 32 bits must be read to clear the receive flag
		uart.command[uart.cmdPos] = (char)recData; 				// accessing RDR automatically resets the receive flag
		if (uart.command[uart.cmdPos] == 10) {
			uart.commandReady = true;
			uart.cmdPos = 0;
		} else {
			uart.cmdPos++;
		}
	}
}
}
