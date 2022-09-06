#include <avr/io.h>
#include <avr/interrupt.h>
#include "SerialInterf.h"
#include "UPDI.h"
#include "Timeout.h"
#include "NVM.h"

#include <util/delay.h>

#define checkTimeout() if(g_timeout_flag) goto _UPDI_TIMEOUT
#define waitForACK() UPDI_startReceive();\
if(UPDI_receive() != UPDI_ACK) {checkTimeout(); goto _UPDI_NO_ACK;}

uint8_t open;
volatile uint16_t serialBytesToSend;

uint8_t lastError;

bool nvmWrite;
int main(void)
{
	Serial::init(SERIAL_DEFAULT_BAUD);
	Timeout_init();

	serialBytesToSend = 0;
	
	sei();

	while (1)
	{
		_MAINLOOP:
		open = false;
		uint8_t recvd;
		recvd = Serial::receive();
		//Unconnected
		
		if(recvd == CMD_START)
		{
			#pragma region START
			_START:
			// Receive signature address
			Serial::ACK();

			nvmWrite = false;
			lastError = 0;
			serialBytesToSend = 1;
			UPDI_init(UPDI_DEFAULT_BAUD); //Set baud to initial low speed

			// Generate START condition
			UPDI_UART.UCSRB = 0; //Disable RX and TX
			UPDI_DDR = BM(UPDI_PIN); //Set as UPDI pin output, pulling it low
			_delay_us(40); //Hold low
			UPDI_DDR = 0; //Change to input (release)
			
			Timeout_start();
			while(!(UPDI_IN_REG & BM(UPDI_PIN)) && !g_timeout_flag){} //Wait until UPDI initializes on-chip (signaled by chip releasing UPDI line, which returns to high)
			Timeout_stop();
			checkTimeout();

			//First sync and command
			UPDI_UART.UCSRB = BM(TXEN0); //Enable TX
			UPDI_cmd(STCS(CTRLA));
			//UPDI_send(0x6); //Set target-side guard time to minimum
			UPDI_send(0x5); //Set target-side guard time to 4 cycles
			UPDI_wait_transmit_complete();
			UPDI_idle();
			//Get initial status to check if target is responding at all

			Serial::send(UPDI_LDCS(STATUSB));
			serialBytesToSend = 0;
			checkTimeout();

			//For some reason the UPDI sometimes doesn't function properly unless in programming mode
			UPDI_enter_progmode();
			checkTimeout();

			//Signal that system is now in programming mode
			Serial::ACK();

			//Receive signature address from Serial
			uint8_t sigA0, sigA1, sigA2;
			sigA0 = Serial::receive();
			sigA1 = Serial::receive();
			sigA2= Serial::receive();
			serialBytesToSend = 3;

			//Set address pointer to signature address
			UPDI_startSend();
			UPDI_cmd(ST(PTR_SET, ASZ_3BYTE));
			UPDI_send(sigA0);
			UPDI_send(sigA1);
			UPDI_send(sigA2);
			waitForACK();
			
			//Read and send signature
			UPDI_repeat(2);
			UPDI_startSend();
			UPDI_cmd(LD(PTR_INC, DSZ_BYTE));
			UPDI_startReceive();
			serialBytesToSend = 3;
			for(; serialBytesToSend; serialBytesToSend--)
			{
				Serial::send(UPDI_receive()); //Send signature
			}
			UPDI_idle();
			checkTimeout();

			UPDI_STCS(ASI_CTRLA,0x00); //Set UPDI clock speed to max (32MHz)
			checkTimeout();
			Serial::send(REPL_OK);
			open = true;
			#pragma endregion START
			//UPDI started

			_RECEIVE_CMD:
			recvd = Serial::receive();
			while(recvd != CMD_STOP)
			{
				switch(recvd)
				{
					case CMD_SENDBLOCK:
					{
						//Read size to write
						Serial::ACK();
						uint16_t bytesLeft;
						bytesLeft = Serial::receive() | (Serial::receive() << 8);

						#ifdef ENABLE_BLOCK_FASTMODE
						//Enable fast mode (disable ack response)
						UPDI_STCS(CTRLA,_fast_UPDI_CTRLA);
						#endif

						UPDI_startSend();
						while(bytesLeft)
						{
							uint8_t repeats;
							if(bytesLeft > 256)
							{
								//Send a full 256-byte segment
								repeats = 255;
								bytesLeft -= 256;
							}
							else if (bytesLeft > 1)
							{
								//Send all remaining bytes
								repeats = bytesLeft - 1;
								bytesLeft = 0;
							}
							else
							{
								//Send last byte (with no repeats)
								bytesLeft = 0;
								repeats = 0;
								goto _SEND_BYTES; //Skip repeat instruction
							}
							//Send REPEAT instruction
							UPDI_repeat(repeats);
							UPDI_wait_transmit_complete();
							UPDI_wait_guardtime();

							_SEND_BYTES:


							//Start async buffered receive
							Serial::start_buffered_receive();
							Serial::ACK(); //Signal as ready to accept new data
							//Send Store instruction

							UPDI_cmd(ST(PTR_INC,DSZ_BYTE));
							//Send (repeats+1) bytes
							{
								uint8_t i = 255 - repeats;
								volatile uint8_t* sendPos = Serial::rxBuffer;
								do
								{
									//Wait until data is received and buffered from serial
									// Need to do this complicated assembly loop because the compiler thinks that
									// Serial::rxBuffer cannot change in the ISR and will optimize away the rest
									// of the code after the wait loop
									_WAIT_FOR_SERIAL_DATA:
									uint8_t bufPos_temp[2];
									uint8_t rxBufPosH = (uint16_t)Serial::rxBufPos >> 8;
									asm volatile(
									"lds %0, %2 \r\n"
									"lds %1, %3 \r\n"
									:"=r"(bufPos_temp[0]), "=r" (bufPos_temp[1]) : "p"(&Serial::rxBufPos), "p" (((uint8_t*)&Serial::rxBufPos)+1));
									uint16_t bufPos = (bufPos_temp[0] | bufPos_temp[1] << 8);
									if((uint16_t)sendPos == bufPos) goto _WAIT_FOR_SERIAL_DATA;

									//if(*sendPos == 128 && sendPos == Serial::rxBuffer)
									//_delay_ms(5000);

									UPDI_send(*(sendPos++)); //Send data from buffer and increment pointer
									
									#ifdef ENABLE_BLOCK_FASTMODE
									UPDI_wait_transmit_complete();
									//UPDI_wait_guardtime();
									#else
									waitForACK();
									UPDI_startSend();
									#endif
									i++;

								} while (i);
							}

						}
						Serial::end_buffered_receive();
						UPDI_idle();
						#ifdef ENABLE_BLOCK_FASTMODE
						//Disable fast mode
						UPDI_STCS(CTRLA,_normal_UPDI_CTRLA);
						#endif
					}
					break;

					case CMD_GETBLOCK:
					{
						//Read size to read
						Serial::ACK();
						serialBytesToSend = Serial::receive() | (Serial::receive() << 8);
						Serial::ACK(); //Signal ACK to confirm valid read

						while(serialBytesToSend)
						{
							UPDI_startSend();
							uint8_t repeats;
							if(serialBytesToSend > 256)
							{
								//Read a full 256-byte segment
								repeats = 255;
							}
							else if (serialBytesToSend > 1)
							{
								//Read all remaining bytes
								repeats = serialBytesToSend - 1;
							}
							else
							{
								//Read last byte (with no repeats)
								repeats = 0;
								goto _READ_BYTES; //Skip repeat instruction
							}
							//Send REPEAT instruction
							UPDI_repeat(repeats);
							UPDI_wait_transmit_complete();
							UPDI_wait_guardtime();

							_READ_BYTES:

							//Send Load instruction

							UPDI_cmd(LD(PTR_INC,DSZ_BYTE));
							UPDI_startReceive();
							//Read (repeats+1) bytes
							{
								uint8_t i = 255 - repeats;
								do
								{
									//Receive data and check for errors
									Timeout_reset();
									UPDI_wait_for_data();
									uint8_t status = UPDI_UART.UCSRA;
									if(status & (BM(FE0) | BM(PE0) | BM(DOR0))) //Check for transmission errors
									{
										lastError = status;
										goto _UPDI_RX_ERROR;
									}
									Serial::send(UPDI_UART.UDR); //Read data and send to serial
									serialBytesToSend--;
									checkTimeout();
									i++;

								} while (i);
							}

						}
						UPDI_idle();

					}
					break;

					case CMD_GETBYTE:
					{
						serialBytesToSend = 1;
						Serial::ACK();
						UPDI_startSend();
						UPDI_cmd(LDS(ASZ_3BYTE,DSZ_BYTE));
						//Send address
						for(uint8_t c = 3; c; c--)
						{
							UPDI_send(Serial::receive());
						}
						Serial::ACK();
						UPDI_wait_transmit_complete();
						UPDI_startReceive();
						Serial::send(UPDI_receive());
						UPDI_idle();
						serialBytesToSend = 0;
						checkTimeout();
					}
					break;

					case CMD_SENDBYTE:
					{
						Serial::ACK();
						UPDI_startSend();
						UPDI_cmd(STS(ASZ_3BYTE,DSZ_BYTE));
						//Send address
						for(uint8_t c = 3; c; c--)
						{
							UPDI_send(Serial::receive());
						}
						Serial::ACK();
						UPDI_wait_transmit_complete();
						waitForACK();
						//Send data
						UPDI_startSend();
						UPDI_send(Serial::receive());
						Serial::ACK();
						UPDI_wait_transmit_complete();
						waitForACK();
						UPDI_idle();

						if(nvmWrite)
						{
							//Wait until previous operation finishes
							while(UPDI_LDS(NVMCTRL_STATUS) & NVMCTRL_BUSY_gc){}
						}
					}
					break;

					case CMD_SETADDR:
					{
						Serial::ACK();

						uint8_t addrb[3];
						for(uint8_t i = 0; i < 3; i++)
						addrb[i] = Serial::receive();
						Serial::ACK();

						UPDI_startSend();
						UPDI_cmd(ST(PTR_SET, ASZ_3BYTE));
						//Send address
						for(uint8_t i = 0; i < 3; i++)
						{
							UPDI_send(addrb[i]);
						}
						waitForACK();
						UPDI_idle();
					}
					break;

					case CMD_FLASH_PAGE_ERASE:
					{
						Serial::ACK();

						uint8_t addrb[3];
						for(uint8_t i = 0; i < 3; i++)
						addrb[i] = Serial::receive();
						__uint24 addr = *((__uint24*)addrb);

						Serial::ACK();

						uint8_t sizeb[2];
						for(uint8_t i = 0; i < 2; i++)
						sizeb[i] = Serial::receive();
						uint16_t size = *((uint16_t*)sizeb);

						Serial::ACK();

						uint8_t stepb[2];
						for(uint8_t i = 0; i < 2; i++)
						stepb[i] = Serial::receive();
						uint16_t step = *((uint16_t*)stepb);

						//Unlock writing to NVM command
						if(!UPDI_STS(CCP,CCP_SPM_gc))goto _GENERAL_ERROR;

						//Clear current NVM command
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_NOOP_gc))goto _GENERAL_ERROR;

						//Set NVM command to flash page erase
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_FLPER_gc))goto _GENERAL_ERROR;
						
						//Erase pages
						for(; size; size--){
							//Dummy write
							UPDI_startSend();
							UPDI_cmd(STS(ASZ_3BYTE ,DSZ_BYTE));
							UPDI_send(addr);
							UPDI_send(addr >> 8);
							UPDI_send(addr >> 16);
							waitForACK();
							UPDI_startSend();
							UPDI_send(0xff);
							waitForACK();
							UPDI_idle();
							addr += step;
							
							//Wait until previous operation finishes
							while(UPDI_LDS(NVMCTRL_STATUS) & NVMCTRL_BUSY_gc){}
							
							Serial::ACK(); //Notify controller that the erasure is still ongoing...

						}
						//Unlock writing to NVM command
						if(!UPDI_STS(CCP,CCP_SPM_gc))goto _GENERAL_ERROR;

						//Clear current NVM command
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_NOOP_gc))goto _GENERAL_ERROR;
						
					}

					break;

					case CMD_EEPROM_WR_EN:
					{
						Serial::ACK();

						//Unlock writing to NVM command
						if(!UPDI_STS(CCP,CCP_SPM_gc))goto _GENERAL_ERROR;

						//Set NVM command to FLWR
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_EEERWR_gc))goto _GENERAL_ERROR;

						nvmWrite = true;
						
					}
					break;

					case CMD_FLASH_WR_EN:
					{
						Serial::ACK();

						#ifdef VERIFY_PROGMODE
						//Verify that target is in programming mode
						if(!((UPDI_LDCS(ASI_SYS_STATUS) & BM(UPDI_NVMPROG_ENABLED))))
						goto _GENERAL_ERROR;
						#endif

						//Unlock writing to NVM command
						if(!UPDI_STS(CCP,CCP_SPM_gc))goto _GENERAL_ERROR;

						//Set NVM command to NOOP
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_NOOP_gc))goto _GENERAL_ERROR;
						//Set NVM command to FLWR
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_FLWR_gc))goto _GENERAL_ERROR;

						nvmWrite = true;
						
					}
					break;

					case CMD_NVM_WR_DIS:
					{
						Serial::ACK();

						//Unlock writing to NVM command
						if(!UPDI_STS(CCP,CCP_SPM_gc))goto _GENERAL_ERROR;

						//Set NVM command to NONE
						if(!UPDI_STS(NVMCTRL_CTRLA, NVMCTRL_CMD_NONE_gc))goto _GENERAL_ERROR;

						nvmWrite = false;
						
					}
					break;

					case CMD_SETBAUD:
					{
						Serial::ACK();

						uint8_t baudb[4];
						baudb[3] = Serial::receive();
						baudb[2] = Serial::receive();
						Serial::ACK();
						baudb[1] = Serial::receive();
						baudb[0] = Serial::receive();
						Serial::ACK();

						uint32_t baud = 0;
						for (uint8_t i = 0; i < 4; i++)
						{
							baud <<= 8;
							baud |= baudb[i];
						}

						UPDI_init(baud);

						//Check status
						if(UPDI_LDCS(STATUSB)) goto _GENERAL_ERROR;
						checkTimeout();
					}
					break;

					case CMD_CHIPERASE:
					{
						Serial::ACK();
						
						//Send key
						UPDI_send_key(&Key_CHER);

						//Check if key accepted
						if(!(UPDI_LDCS(ASI_KEY_STATUS) & BM(UPDI_CHIPERASE)))
						goto _GENERAL_ERROR;

						//Reset
						UPDI_system_reset();

						//Wait until lock flag is cleared
						uint8_t status;
						do
						{
							status = UPDI_LDCS(ASI_SYS_STATUS);
						} while (status & BM(UPDI_LOCKSTATUS));

						//Check ERASE_FAIL flag and throw an error if set
						if(status & BM(UPDI_ERASE_FAILED))
						goto _ERASE_FAIL;

						UPDI_idle();
						UPDI_enter_progmode(); //Re-enter progmode

					}
					break;

					case CMD_RESETTGT:
					{
						Serial::ACK();

						//Reset target
						UPDI_system_reset();

						//Check status
						if(UPDI_LDCS(STATUSB)) goto _GENERAL_ERROR;
						checkTimeout();
					}
					break;

					case CMD_START:
					goto _START; //Restart

					default:
					goto _INVALID_CMD;
				}
				Serial::send(REPL_OK); //Send OK - command finished
				recvd = Serial::receive(); //Receive next command
			}
			//STOP received:
			Serial::ACK();
			UPDI_system_reset();
			UPDI_disable();
			Serial::send(REPL_OK);
		}
		else
		Serial::send(REPL_INVALID);
	}

	_INVALID_CMD:
	Serial::send(REPL_INVALID);
	goto _RECEIVE_CMD;

	_UPDI_NO_ACK:
	for(; serialBytesToSend; serialBytesToSend--)
	Serial::send(0x00); //Send requested bytes as zeros to preserve communication scheme
	Serial::send(REPL_UPDINOACK);
	goto _RECEIVE_CMD;

	_GENERAL_ERROR:
	checkTimeout();
	for(; serialBytesToSend; serialBytesToSend--)
	Serial::send(0x00); //Send requested bytes as zeros to preserve communication scheme
	Serial::send(REPL_ERROR);
	goto _RECEIVE_CMD;

	_ERASE_FAIL:
	for(; serialBytesToSend; serialBytesToSend--)
	Serial::send(0x00); //Send requested bytes as zeros to preserve communication scheme
	Serial::send(REPL_ERASEFAIL);
	goto _RECEIVE_CMD;

	_UPDI_RX_ERROR:
	for(; serialBytesToSend; serialBytesToSend--)
	Serial::send(0x00); //Send requested bytes as zeros to preserve communication scheme
	Serial::send(REPL_UPDIRXERR);
	goto _RECEIVE_CMD;

	_UPDI_TIMEOUT:
	for(; serialBytesToSend; serialBytesToSend--)
	Serial::send(0x00); //Send requested bytes as zeros to preserve communication scheme
	Serial::send(REPL_TIMEOUT);
	g_timeout_flag = 0;
	if(open) goto _RECEIVE_CMD;
	else goto _MAINLOOP;
}



