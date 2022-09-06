#include <avr/interrupt.h>
#include "Timeout.h"

volatile uint8_t g_timeout_flag;

ISR(TIMER1_COMPA_vect)
{
	Timeout_stop();
	g_timeout_flag = 1;
}