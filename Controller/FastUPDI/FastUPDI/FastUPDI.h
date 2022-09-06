#pragma once

#include <stdint.h>
#include <stdio.h>
#include "Serial.h"
#include "Config.h"
#include "global.h"
#include <iomanip>
#include <iostream>

#define COUTHEX(val, width) std::hex << std::setw(width) << (int)val

#define I16TOBUF(bufname, val) uint8_t bufname[2] = {val, val >> 8} //Create an LSB-first buffer from int16
#define I24TOBUF(bufname, val) uint8_t bufname[3] = {val, val >> 8, val >> 16} //Create an LSB-first buffer from int24
#define I32TOBUF(bufname, val) uint8_t bufname[4] = {val, val >> 8, val >> 16, val >> 24} //Create an LSB-first buffer from int32

#define PROGRESS_BAR_SIZE 32

class FastUPDI
{
	Serial* ser;

public:
	uint16_t data_packet_size = 256;
	Device_t* device;
	uint32_t normalBaudrate = 115200;

	//Write image to memory with specified offset (base address)
	bool write_image(image_t* image, uint32_t offset = 0, EMemoryType memtype = EMemoryType::RAM);
	//Write image to specified memory
	bool write_image(image_t* image, const std::string& memoryName);

	//Verify data in memory (with specified offset) against image
	bool verify_image(image_t* image, uint32_t offset = 0);
	//Verify data in specified memory
	bool verify_image(image_t* image, const std::string& memoryName);

	//Create image from memory at specified offset (base address)
	bool read_image(image_t* outImage, uint32_t length, uint32_t offset = 0, EOutputMode readMode = EOutputMode::FULL);
	//Create image from specified memory
	bool read_image(image_t* outImage, const std::string& memoryName, EOutputMode readMode = EOutputMode::FULL);


	//Read single byte
	bool read_byte(uint32_t address, uint8_t* outData);

	//Write single byte
	bool write_byte(uint32_t address, uint8_t data);

	//Verify single byte
	bool verify_byte(uint32_t address, uint8_t data);


	//Set starting address for block read/write
	bool set_block_address(uint32_t address);

	//Read block starting at current address (use set_block_address() to set). Address will be moved to the end of the block after reading.
	bool read_block(uint8_t* outData, uint16_t size);

	//Write block starting at current address (use set_block_address() to set). Address will be moved to the end of the block after reading.
	bool write_block(uint8_t* data, uint16_t size);

	bool enable_flash_write();
	bool enable_eeprom_write();
	bool disable_nvm_write();

	bool chip_erase();

	bool flash_page_erase(uint32_t startAddress, uint16_t nPages);

	//Create start condition (open link) and read chip signature
	bool start(Serial* port, Device_t* _device);

	//Disable link
	bool stop();

	bool set_baud(uint32_t normalBaudrate);

	//Create BREAK condition on UPDI and extract status (error code)
	bool recover();

	bool reset_target();


	//Serial interface utilities
	static std::string decode_error_reply(uint8_t reply);
	static std::string decode_cmd(CMD _cmd);

protected:
	CMD currentCMD;

	//Serial interface utilities
	inline bool SerialWriteByte(uint8_t byte) { return ser->Write(&byte, 1); }

	bool initiate_cmd(CMD cmd);
	bool wait_cmd_ACK();
	bool check_cmd_result();

	bool progressBarActive = false;
	uint8_t progressBarSegments = 0xff;
	void print_progress_bar(float percentage = 0.0f);
	void update_progress_bar(float percentage);
	inline void end_progress_bar();
};