#ifndef _UPDI_h
#define _UPDI_h

#include "Config.h"
#include "global.h"
#include "Timeout.h"
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

//Registers
#define STATUSA 0x0
#define STATUSB 0x1
#define CTRLA 0x2
#define CTRLB 0x3
#define ASI_KEY_STATUS 0x7
#define ASI_RESET_REQ 0x8
#define ASI_CTRLA 0x9
#define ASI_SYS_CTRLA 0xA
#define ASI_SYS_STATUS 0xB
#define ASI_CRC_STATUS 0xC

//Register bits
#define UPDI_UPDIREV_bp 4 /* bits 7-4 in STATUSA */
#define UPDI_IBDLY 7
#define UPDI_PARD 5
#define UPDI_DTD 4
#define UPDI_RSD 3
#define UPDI_NACKDIS 4
#define UPDI_CCDETDIS 3
#define UPDI_UPDIDIS 2
#define UPDI_UROWWRITE 5
#define UPDI_NVMPROG_KEY_STATUS 4 /* NVMPROG bit in ASI_KEY_STATUS */
#define UPDI_CHIPERASE 3
#define UPDI_UROWDONE 1
#define UPDI_CLKREQ 0
#define UPDI_ERASE_FAILED 6
#define UPDI_SYSRST 5
#define UPDI_INSLEEP 4
#define UPDI_NVMPROG_ENABLED 3 /* NVMPROG bit in ASI_SYS_STATUS */
#define UPDI_UROWPROG 2
#define UPDI_LOCKSTATUS 0

#define RSTREQ_RUN 0x00
#define RSTREQ_RESET 0x59

//Special frames
#define UPDI_SYNC 0x55
#define UPDI_ACK 0x40

//Opcodes
#define UPDI_OPC_LDS 0x00
#define UPDI_OPC_STS 0x40
#define UPDI_OPC_LD 0x20
#define UPDI_OPC_ST 0x60
#define UPDI_OPC_LDCS 0x80
#define UPDI_OPC_STCS 0xC0
#define UPDI_OPC_REPEAT 0xA0
#define UPDI_OPC_KEY 0xE0

//Instruction operands
#define ASZ_BYTE 0x0
#define ASZ_WORD 0x1
#define ASZ_3BYTE 0x2

#define DSZ_BYTE 0x0
#define DSZ_WORD 0x1

#define PTR_STATIC 0x0 /* *(ptr) */
#define PTR_INC 0x1 /* *(ptr++) */
#define PTR_SET 0x2 /* ptr */

#define KEY_GETSIB 0x5 /* Receive System Information Block */
#define KEY_SEND 0x0 /* Send Key */

//Instructions
#define LDS(addrsize, datasize) (UPDI_OPC_LDS | (addrsize << 2)  | datasize)
#define STS(addrsize, datasize) (UPDI_OPC_STS | (addrsize << 2)  | datasize)
#define LD(ptr, sz) (UPDI_OPC_LD | (ptr << 2)  | sz)
#define ST(ptr, sz) (UPDI_OPC_ST | (ptr << 2)  | sz)
#define LDCS(csaddr) (UPDI_OPC_LDCS | (csaddr & 0xf))
#define STCS(csaddr) (UPDI_OPC_STCS | (csaddr & 0xf))
#define REPEAT() (UPDI_OPC_REPEAT)
#define KEY(operand) (UPDI_OPC_KEY | operand) /* Use with KEY_SEND or KEY_GETSIB */

#define _normal_UCSRA (BM(U2X0)) /*Normal operation state of UPDI_UART UCSRA */
#define _normal_UPDI_CTRLA (0x6) /*Normal operation state of UPDI CTRLA */
#define _fast_UPDI_CTRLA (0x6 | BM(UPDI_RSD)) /*Fast-mode operation state of UPDI CTRLA */

const PROGMEM uint64_t Key_CHER = 0x4E564D4572617365;
const PROGMEM uint64_t Key_NVMPROG = 0x4E564D50726F6720;
const PROGMEM uint64_t Key_USERROWWRITE = 0x4E564D5573267465;

extern volatile uint16_t gt_loops; //Guard time loops

void UPDI_init(uint32_t baudrate);

FORCEINLINE void UPDI_idle()
{
	Timeout_stop();
	//UPDI_UART.UCSRB = 0;
	UPDI_UART.UCSRB = BM(TXEN0); //Enable TX
}
FORCEINLINE void UPDI_sync()
{
	UPDI_UART.UDR = UPDI_SYNC;
}

FORCEINLINE void UPDI_wait_guardtime()
{
	///Guard time delay
	uint16_t cycles = gt_loops;
	_WAIT_GT:
	//Each iteration: 3 CPU cycles
	asm volatile ("sbiw %0, 1" :"+e" (cycles));
	asm goto ("brne %l0" :::: _WAIT_GT);
}
//Wait until previous transmission finishes}
FORCEINLINE void UPDI_wait_transmit_complete()
{
	while (!(UPDI_UART.UCSRA & BM(TXC0))) {}
}
FORCEINLINE void UPDI_wait_txbuf_empty() {while (!(UPDI_UART.UCSRA & BM(UDRE0))) {}} //Wait until Tx buffer is ready to receive new data}

FORCEINLINE void UPDI_startSend()
{
	Timeout_stop();
	UPDI_wait_transmit_complete();
	UPDI_wait_guardtime();
	//UPDI_UART.UCSRB = 0; //Disable RX and TX
	UPDI_UART.UCSRB = BM(TXEN0); //Enable TX
}
FORCEINLINE void UPDI_send(uint8_t data)
{
	UPDI_wait_txbuf_empty(); //Wait until previous transmission finishes
	UPDI_UART.UDR = data;	//Send data to buffer
	UPDI_UART.UCSRA = _normal_UCSRA | BM(TXC0); //Clear transmit complete flag
}
//Send sync character followed by command
FORCEINLINE void UPDI_cmd(uint8_t cmd)
{
	UPDI_sync();
	UPDI_send(cmd);
}
FORCEINLINE void UPDI_startReceive()
{
	UPDI_wait_transmit_complete();
	UPDI_UART.UCSRB = 0; //Disable RX and TX
	UPDI_UART.UCSRB = BM(RXEN0); //Enable RX
	Timeout_start();
}

void UPDI_wait_for_data();


FORCEINLINE uint8_t UPDI_receive()
{
	Timeout_reset();
	UPDI_wait_for_data();
	return UPDI_UART.UDR;
}
FORCEINLINE bool UPDI_wait_for_ACK()
{
	UPDI_startReceive();
	return UPDI_receive() == UPDI_ACK;
}

FORCEINLINE void UPDI_repeat(uint8_t n)
{
	UPDI_startSend();
	UPDI_cmd(REPEAT());
	UPDI_send(n);
}


FORCEINLINE void UPDI_send_key(const uint64_t* key_pgmptr)
{
	UPDI_startSend();
	UPDI_cmd(KEY(KEY_SEND));
	for(uint8_t i = 0; i != 8; i++)
	UPDI_send(pgm_read_byte(((uint8_t*)key_pgmptr)+i));
	UPDI_wait_transmit_complete();
	UPDI_idle();
}

//Create a break condition on the line
FORCEINLINE void UPDI_break()
{
	UPDI_UART.UCSRB = 0; //Disable RX and TX
	UPDI_DDR = BM(UPDI_PIN); //Set as UPDI pin output, pulling it low
	_delay_ms(24); //Hold low
	UPDI_DDR = 0; //Change to input (release)
	UPDI_wait_guardtime();
}

FORCEINLINE uint8_t UPDI_LDCS(const uint8_t reg)
{
	UPDI_startSend();
	UPDI_cmd(LDCS(reg));
	UPDI_startReceive();
	uint8_t value = UPDI_receive();
	UPDI_idle();
	return value;
}
FORCEINLINE void UPDI_STCS(const uint8_t reg, uint8_t value)
{
	UPDI_startSend();
	UPDI_cmd(STCS(reg));
	UPDI_send(value);
	UPDI_wait_transmit_complete();
	UPDI_idle();
}
FORCEINLINE uint8_t UPDI_LDS(uint16_t address)
{
	UPDI_startSend();
	UPDI_cmd(LDS(ASZ_3BYTE, DSZ_BYTE));
	UPDI_send(address);
	UPDI_send(address >> 8);
	UPDI_send(0);
	UPDI_startReceive();
	uint8_t value = UPDI_receive();
	UPDI_idle();
	return value;
}
//Returns false if ACK wasn't received
FORCEINLINE bool UPDI_STS(uint16_t address, uint8_t data)
{
	UPDI_startSend();
	UPDI_cmd(STS(ASZ_3BYTE, DSZ_BYTE));
	UPDI_send(address);
	UPDI_send(address >> 8);
	UPDI_send(0x00);
	UPDI_startReceive();
	if(UPDI_receive() != UPDI_ACK) return false;
	UPDI_startSend();
	UPDI_send(data);
	UPDI_startReceive();
	if(UPDI_receive() != UPDI_ACK) return false;
	UPDI_idle();
	return true;
}

FORCEINLINE void UPDI_system_reset()
{
	//Initiate reset
	UPDI_startSend();
	UPDI_cmd(STCS(ASI_RESET_REQ));
	UPDI_send(RSTREQ_RESET);
	
	//Clear reset
	UPDI_wait_transmit_complete();
	UPDI_wait_guardtime();
	_delay_us(100);
	UPDI_cmd(STCS(ASI_RESET_REQ));
	UPDI_send(RSTREQ_RUN);
	UPDI_wait_transmit_complete();
	_delay_us(100);

	//Wait until system reset is complete
	uint8_t status;
	do{
		status = UPDI_LDCS(ASI_SYS_STATUS);
		_delay_us(100);

	} while(status & BM(UPDI_SYSRST));
}

FORCEINLINE void UPDI_disable()
{
	UPDI_startSend();
	UPDI_cmd(STCS(0x3));
	UPDI_send(1 << 2 /*UPDIDIS*/); //Disable UPDI
}

FORCEINLINE void UPDI_enter_progmode()
{
	Timeout_start();
	//Reset to prevent resetting from WDT during transmission
	UPDI_system_reset();

	UPDI_send_key(&Key_NVMPROG);
	UPDI_system_reset(); //System reset to activate key

	//Wait until target enters programming mode
	while(!(UPDI_LDCS(ASI_SYS_STATUS) & BM(UPDI_NVMPROG_ENABLED)) && !g_timeout_flag){}
	Timeout_stop();
}


#endif

