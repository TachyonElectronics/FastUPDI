#ifndef NVM_H_
#define NVM_H_
/*
 * Copyright (C) 2020, Microchip Technology Inc. and its subsidiaries ("Microchip")
 * All rights reserved.
 *
 * This software is developed by Microchip Technology Inc. and its subsidiaries ("Microchip").
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this list of
 *        conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice, this list
 *        of conditions and the following disclaimer in the documentation and/or other
 *        materials provided with the distribution. Publication is not required when
 *        this file is used in an embedded application.
 *
 *     3. Microchip's name may not be used to endorse or promote products derived from this
 *        software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICROCHIP "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MICROCHIP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING BUT NOT LIMITED TO
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWSOEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 #include <stdint.h>

 typedef volatile uint8_t register8_t;
 typedef volatile uint16_t register16_t;
  typedef volatile uint32_t register32_t;

 #ifdef _WORDREGISTER
 #undef _WORDREGISTER
 #endif
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

 #ifdef _DWORDREGISTER
 #undef _DWORDREGISTER
 #endif
 #define _DWORDREGISTER(regname)  \
 __extension__ union \
 { \
	 register32_t regname; \
	 struct \
	 { \
		 register8_t regname ## 0; \
		 register8_t regname ## 1; \
		 register8_t regname ## 2; \
		 register8_t regname ## 3; \
	 }; \
 }

/*
--------------------------------------------------------------------------
NVMCTRL - Non-volatile Memory Controller
--------------------------------------------------------------------------
*/

/* Command select */
typedef enum NVMCTRL_CMD_enum
{
	NVMCTRL_CMD_NONE_gc = (0x00<<0),  /* No Command */
	NVMCTRL_CMD_NOOP_gc = (0x01<<0),  /* No Operation */
	NVMCTRL_CMD_FLWR_gc = (0x02<<0),  /* Flash Write */
	NVMCTRL_CMD_FLPER_gc = (0x08<<0),  /* Flash Page Erase */
	NVMCTRL_CMD_FLMPER2_gc = (0x09<<0),  /* Flash Multi-Page Erase 2 pages */
	NVMCTRL_CMD_FLMPER4_gc = (0x0A<<0),  /* Flash Multi-Page Erase 4 pages */
	NVMCTRL_CMD_FLMPER8_gc = (0x0B<<0),  /* Flash Multi-Page Erase 8 pages */
	NVMCTRL_CMD_FLMPER16_gc = (0x0C<<0),  /* Flash Multi-Page Erase 16 pages */
	NVMCTRL_CMD_FLMPER32_gc = (0x0D<<0),  /* Flash Multi-Page Erase 32 pages */
	NVMCTRL_CMD_EEWR_gc = (0x12<<0),  /* EEPROM Write */
	NVMCTRL_CMD_EEERWR_gc = (0x13<<0),  /* EEPROM Erase and Write */
	NVMCTRL_CMD_EEBER_gc = (0x18<<0),  /* EEPROM Byte Erase */
	NVMCTRL_CMD_EEMBER2_gc = (0x19<<0),  /* EEPROM Multi-Byte Erase 2 bytes */
	NVMCTRL_CMD_EEMBER4_gc = (0x1A<<0),  /* EEPROM Multi-Byte Erase 4 bytes */
	NVMCTRL_CMD_EEMBER8_gc = (0x1B<<0),  /* EEPROM Multi-Byte Erase 8 bytes */
	NVMCTRL_CMD_EEMBER16_gc = (0x1C<<0),  /* EEPROM Multi-Byte Erase 16 bytes */
	NVMCTRL_CMD_EEMBER32_gc = (0x1D<<0),  /* EEPROM Multi-Byte Erase 32 bytes */
	NVMCTRL_CMD_CHER_gc = (0x20<<0),  /* Chip Erase Command */
	NVMCTRL_CMD_EECHER_gc = (0x30<<0),  /* EEPROM Erase Command */
} NVMCTRL_CMD_t;

/* Write error select */
typedef enum NVMCTRL_ERROR_enum
{
	NVMCTRL_ERROR_NOERROR_gc = (0x00<<4),  /* No Error */
	NVMCTRL_ERROR_ILLEGALCMD_gc = (0x01<<4),  /* Write command not selected */
	NVMCTRL_ERROR_ILLEGALSADDR_gc = (0x02<<4),  /* Write to section not allowed */
	NVMCTRL_ERROR_DOUBLESELECT_gc = (0x03<<4),  /* Selecting new write command while write command already seleted */
	NVMCTRL_ERROR_ONGOINGPROG_gc = (0x04<<4),  /* Starting a new programming operation before previous is completed */
} NVMCTRL_ERROR_t;

/* Flash Mapping in Data space select */
typedef enum NVMCTRL_FLMAP_enum
{
	NVMCTRL_FLMAP_SECTION0_gc = (0x00<<4),  /* Flash section 0 */
	NVMCTRL_FLMAP_SECTION1_gc = (0x01<<4),  /* Flash section 1 */
	NVMCTRL_FLMAP_SECTION2_gc = (0x02<<4),  /* Flash section 2 */
	NVMCTRL_FLMAP_SECTION3_gc = (0x03<<4),  /* Flash section 3 */
} NVMCTRL_FLMAP_t;

#define NVMCTRL_FBUSY_bm 0x01 /*NVMCTRL Flash Busy flag (in STATUS) */
#define NVMCTRL_EEBUSY_bm 0x02 /*NVMCTRL EEPROM Busy flag (in STATUS) */
#define NVMCTRL_BUSY_gc 0x03 /*NVMCTRL Flash or EEPROM Busy (in STATUS) */

#define NVMCTRL 0x1000 /* Non-volatile Memory Controller */
#define NVMCTRL_CTRLA (NVMCTRL+0x00) /* NVMCTRL Control A */
#define NVMCTRL_STATUS (NVMCTRL+0x02) /* NVMCTRL Status register */

#define CCP 0x0034  /* Configuration Change Protection register*/
/* CCP signature select */
typedef enum CCP_enum
{
	CCP_SPM_gc = (0x9D<<0),  /* SPM Instruction Protection */
	CCP_IOREG_gc = (0xD8<<0),  /* IO Register Protection */
} CCP_t;


#endif /* NVM_H_ */