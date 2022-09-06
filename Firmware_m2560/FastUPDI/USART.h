/*
* UART.h
*
* Created: 24.8.2022 14:36:25
*  Author: DELTA-PC
*/


#ifndef USART_H_
#define USART_H_

#include <stdint.h>

typedef volatile uint8_t register8_t;
typedef volatile uint16_t register16_t;

#define _WORDREGISTER(regname)   \
__extension__ union \
{ \
	register16_t regname; \
	struct \
	{ \
		register8_t regname ## L; \
		register8_t regname ## H; \
	}; \
}

// ATmega2560
typedef struct USART_struct{
	register8_t UCSRA; /* Control and status register A */
	register8_t UCSRB; /* Control and status register B */
	register8_t UCSRC; /* Control and status register C */
	register8_t _reserved_1[1];
	_WORDREGISTER(UBRR); /* Baud rate register */
	register8_t UDR;	/* I/O Data register */
} USART_t;


#define USART0                (*(USART_t *) 0xC0) /* USART */
#define USART1                (*(USART_t *) 0xC8) /* USART */
#define USART2                (*(USART_t *) 0xD0) /* USART */
#define USART3                (*(USART_t *) 0x130) /* USART */
#endif /* UART_H_ */