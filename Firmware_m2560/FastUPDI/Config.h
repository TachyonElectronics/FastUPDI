/*
 * System configuration file
 * Change settings here to adapt to other boards or adjust system functionality
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include "USART.h"

#define UPDI_DEFAULT_BAUD 56000
#define UPDI_RUN_BAUD 500000
#define SERIAL_DEFAULT_BAUD 1000000

// Uncomment to enable fast mode during block send/receive. 
// This disables ACK response from target and speeds up transmission, at the cost of not having any confirmation from target that it's receiving the data
#define ENABLE_BLOCK_FASTMODE

// Uncomment to enable programming mode verification when writing to NVM
// This ensures that NVM can be written to before issuing write commands, but lower the writing speed slightly due to the check's overhead
#define VERIFY_PROGMODE

// How long the target must not respond for a timeout to be triggered (in ms). Must be an integer multiple of 8
#define UPDI_TIMEOUT_MS 320


#define SERIAL_UART USART0
#define SERIAL_UART_RXINT_VECTOR USART0_RX_vect

#define UPDI_UART USART1

#define UPDI_PORT PORTD
#define UPDI_PIN 2
#define UPDI_IN_REG PIND
#define UPDI_DDR DDRD


#endif /* CONFIG_H_ */