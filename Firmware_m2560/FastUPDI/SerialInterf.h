#ifndef _SERIALINTERF_h
#define _SERIALINTERF_h

#include <avr/io.h>
#include <avr/interrupt.h>
#include "Config.h"
#include "global.h"
#include "SerialProtocol.h"

//T_CPU * (ISR_cycles + Intermediate_cycles) > Serial_T_bit * Serial_bits_per_frame
// >> (F_CPU * Serial_bits_per_frame) / (ISR_cycles + Intermediate_cycles) = MAX_BAUD
#define SERIAL_MAX_BAUD ((F_CPU * 10) / (35+5))

namespace Serial
{
	extern volatile uint8_t rxBuffer[1024];
	extern volatile uint8_t* rxBufPos;

	#define NORMAL_UCSRB (BM(RXEN0) | BM(TXEN0)) //Enable Tx & Rx

	void init(uint32_t baudrate);

	FORCEINLINE void send(uint8_t data)
	{
		while (!(SERIAL_UART.UCSRA & BM(UDRE0))) {} //Wait until previous transmission finishes

		SERIAL_UART.UDR = data;
	}


	FORCEINLINE uint8_t receive()
	{
		while (!(SERIAL_UART.UCSRA & BM(RXC0))) {} //Wait until data is received
		return SERIAL_UART.UDR;
	};

	FORCEINLINE void ACK()
	{
		Serial::send(REPL_ACK);
	}

	FORCEINLINE void start_buffered_receive()
	{
		//Need this asm block to force compiler to reset the buffer pos at this exact moment
		uint8_t rxBufH = ((uint16_t)rxBuffer) >> 8;
		asm volatile(
		"sts %0, %2 \r\n"
		"sts %1, %3 \r\n"
		::"p"(&Serial::rxBufPos), "p"(((uint8_t*)&Serial::rxBufPos)+1), "r"((uint16_t)Serial::rxBuffer), "r"(((uint16_t)Serial::rxBuffer) >> 8));
		//rxBufPos = rxBuffer;
		SERIAL_UART.UCSRB = NORMAL_UCSRB | BM(RXCIE0);
	}
	FORCEINLINE void end_buffered_receive()
	{
		SERIAL_UART.UCSRB = NORMAL_UCSRB;
	}

}

#endif

