#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include <avr/io.h>
#include "global.h"
#include "Config.h"

#define TIMEOUT_CLKSRC_STOP 0 // Clock disconnected
#define TIMEOUT_CLKSRC_RUN 0b101 // CLKio/1024
#define TIMEOUT_CLK_PRESCALER_VALUE 1024
#define TIMEOUT_1MS_VALUE F_CPU / 1000 / TIMEOUT_CLK_PRESCALER_VALUE
#define TIMEOUT_TOTAL_VALUE (UPDI_TIMEOUT_MS * TIMEOUT_1MS_VALUE)

#if (F_CPU / 1000 != TIMEOUT_TOTAL_VALUE * TIMEOUT_CLK_PRESCALER_VALUE / UPDI_TIMEOUT_MS)
#error "Timeout(ms) does not produce an integer timing value"
#endif

extern volatile uint8_t g_timeout_flag;

FORCEINLINE void Timeout_init()
{
	TIMSK1 = BM(OCIE0A); //Enable output compare match interrupt
	TCCR1B = TIMEOUT_CLKSRC_STOP; //Setup clock source
	OCR1A = TIMEOUT_TOTAL_VALUE;
	g_timeout_flag = 0;
}

FORCEINLINE void Timeout_stop()
{
	TCCR1B = TIMEOUT_CLKSRC_STOP;
}

FORCEINLINE void Timeout_reset()
{
	TCNT1 = 0;
}

FORCEINLINE void Timeout_start()
{
	Timeout_reset();
	g_timeout_flag = 0;
	TCCR1B = TIMEOUT_CLKSRC_RUN;
}

#endif /* TIMEOUT_H_ */