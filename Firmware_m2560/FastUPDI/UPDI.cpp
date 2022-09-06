//
//
//

#include "UPDI.h"

volatile uint16_t gt_loops;



void UPDI_init(uint32_t baudrate)
{
	UPDI_UART.UCSRB = 0; //Disable RX and TX
	//Calculate number of cycles per 1 UART bit. Since the delay loop takes 3 CPU cycles, this ensures a guard time of at least 3 bits
	gt_loops = F_CPU / baudrate;
	uint16_t baud_setting = (F_CPU / (8* baudrate)) -1;
	UPDI_UART.UBRR = baud_setting;
	UPDI_UART.UCSRA = BM(U2X0); //Set speed to 2X
	UPDI_UART.UCSRC = BM(UPM01) | BM(USBS0) | BM(UCSZ00) | BM(UCSZ01); //Enable parity bit for even parity, 2 stop bits and set character size to 8-bit
}

void UPDI_wait_for_data()
{
	while (!(UPDI_UART.UCSRA & BM(RXC0)) && !g_timeout_flag){} //Wait for data
}
