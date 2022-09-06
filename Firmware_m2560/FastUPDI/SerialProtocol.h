#ifndef SERIALPROTOCOL_H_
#define SERIALPROTOCOL_H_

#include <stdint.h>

enum CMD : uint8_t
{
	CMD_NONE = 0x00,

	//UPDI Data transfer
	CMD_SENDBYTE = 0xA1,
	CMD_SENDBLOCK = 0xA2,
	CMD_GETBYTE = 0xA8,
	CMD_GETBLOCK = 0xA9,
	CMD_SETADDR = 0xA0,

	//Erase commands
	CMD_CHIPERASE = 0x96,
	CMD_FLASH_PAGE_ERASE = 0x2B,		//Erase flash pages

	//Programming / Unlock commands
	CMD_FLASH_WR_EN = 0x71,	//Enable writing to Flash
	CMD_UROWUNLOCK = 0x72,	//NOT IMPLEMENTED
	CMD_EEPROM_WR_EN = 0x73,	//Enable writing to EEPROM
	CMD_NVM_WR_DIS = 0x78,		//Disable writing to NVM

	//System commands
	CMD_RECOVER = 0xB2,
	CMD_START = 0xC1,
	CMD_SETBAUD = 0xC2,		//Set UPDI baudrate
	CMD_RESETTGT = 0xCE,	//Reset target
	CMD_STOP = 0xCF
};

enum REPL : uint8_t
{
	REPL_OK = 0xD0,				//Command performed successfully
	REPL_INVALID = 0xE1,		//Invalid operation
	REPL_UNDEFINED = 0xE2,		//Undefined operation
	REPL_ERROR = 0xE3,			//Unspecified error
	REPL_TIMEOUT = 0xE8,		//UPDI response timed out
	REPL_UPDINOACK = 0xE9,		//UPDI did not ACK
	REPL_UPDIRXERR = 0xEA,		//UPDI Receive error
	REPL_ERASEFAIL = 0xEF,		//Erase failed

	REPL_ACK = 0xA5,			//Data acknowledged
	REPL_READY = 0xCC			//Ready 
};


#endif /* SERIALPROTOCOL_H_ */