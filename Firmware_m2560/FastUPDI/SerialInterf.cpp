//
//
//


#include "SerialInterf.h"

volatile uint8_t Serial::rxBuffer[1024];
volatile uint8_t* Serial::rxBufPos;


void Serial::init(uint32_t baudrate)
{
	SERIAL_UART.UCSRB = 0; //Disable USART
	uint16_t baud_setting = (F_CPU / (8*baudrate)) -1;
	SERIAL_UART.UBRR = baud_setting;
	SERIAL_UART.UCSRA = BM(U2X0); //Set speed to 2X
	SERIAL_UART.UCSRC = 0b00000110; //Set to 8-bit mode
	SERIAL_UART.UCSRB = NORMAL_UCSRB; //Enable Tx & Rx
}

// ISR(SERIAL_UART_RXINT_VECTOR, __attribute__((naked)))
// {
// /* Entry: 5 cycles*/
// asm volatile(
// "push __tmp_reg__ \r\n" /* 1 cycle */
// "lds __tmp_reg__, %0 \r\n" /* max 3 cycles*/
// "st Y+, __tmp_reg__ \r\n" /* max 2 cycles*/
// "pop __tmp_reg__ \r\n" /* max 3 cycles*/
// "reti" /* max 5 cycles*/
// :: "p" (&SERIAL_UART.UDR));
// //Total: 19 cycles
// }

// ISR(SERIAL_UART_RXINT_VECTOR)
// {
// //This is over 30 instructions! way too much
// *(Serial::rxBufPos++) = SERIAL_UART.UDR;
// }
#include <util/delay.h>
ISR(SERIAL_UART_RXINT_VECTOR, __attribute__((naked)))
{
	// Entry: 5 cycles
	asm volatile(
	"push __tmp_reg__ \r\n" // 1 cycle
	"push r26 \r\n" // 1 cycle
	"push r27 \r\n" // 1 cycle
	"lds r26, %0 \r\n" // max 3 cycles
	"lds r27, %1 \r\n" // max 3 cycles
	"lds __tmp_reg__, %2 \r\n" // max 3 cycles
	"st X+, __tmp_reg__ \r\n" // max 2 cycles
	"sts %0, r26 \r\n" // max 2 cycles
	"sts %1, r27 \r\n" // max 2 cycles
	::"p"(&Serial::rxBufPos), "p" (((uint8_t*)&Serial::rxBufPos)+1), "p" (&SERIAL_UART.UDR));
	
	asm volatile("pop r27 \r\n" // max 3 cycles
	"pop r26 \r\n" // max 3 cycles
	"pop __tmp_reg__ \r\n" // max 3 cycles
	"reti"); // max 5 cycles
	//Total: 35 cycles
}
