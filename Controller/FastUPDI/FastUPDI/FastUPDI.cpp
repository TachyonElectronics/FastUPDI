#include <sstream>
#include <chrono>
#include "FastUPDI.h"

//Read byte to buffer or
#define SerialReadByteOrReturn(outBuffer) if(!ser->Read(outBuffer, 1)) return false
#define SerialReadOrReturn(outBuffer, size) if(!ser->Read(outBuffer, size)) return false

#define CHECKED(statement) if(!statement) return false

#define _DEBUG_LISTEN() while(1) {uint8_t _dbg_repl; ser->Read(&_dbg_repl, 1); std::cout << "  DEBUG:" << COUTHEX(_dbg_repl, 2) <<std::endl;}


bool FastUPDI::write_image(image_t* image, uint32_t offset, EMemoryType memtype)
{
	switch (memtype)
	{
	case EMemoryType::FLASH:
		CHECKED(enable_flash_write());
		break;

	case EMemoryType::EEPROM:
		CHECKED(enable_eeprom_write());
		break;
	}

	int c = 1;
	uint8_t* dataPtr = &image->dataStream[0];
	for (memory_t section : image->sections)
	{
		uint32_t address = section.address + offset;

		std::cout << "Writing section " << std::dec << c << " (" << section.size << " B) ..." << std::endl;

		if (memtype != EMemoryType::EEPROM)
		{
			//Block write
			if (!set_block_address(address))return false;
			if (!write_block(dataPtr, section.size))return false;
		}
		else
		{
			//Bitwise write (for EEPROM we have to wait until erase/write finishes before sending another byte)
			for (uint16_t i = 0; i < section.size; i++)
			{
				if (!write_byte(address, dataPtr[i]))return false;
			}
		}
		dataPtr += section.size;
		c++;
	}
	std::cout << "Write complete" << std::endl;

	if (memtype != EMemoryType::RAM)
		CHECKED(disable_nvm_write());

	return true;
}

bool FastUPDI::write_image(image_t* image, const std::string& memoryName)
{
	if (!device->HasMemory(memoryName))
	{
		std::cout << "Device does not have memory '" << memoryName << "' " << std::endl;
		return false;
	}
	memory_t mem = device->memories[memoryName];

	uint32_t imageSize = image->GetTotalSize();
	if (mem.size < imageSize)
	{
		std::cout << "Device does not have enough memory '" << memoryName << "'" << std::endl;
		return false;
	}

	return write_image(image, mem.address, mem.type);
}

bool FastUPDI::verify_image(image_t* image, uint32_t offset)
{
	int c = 1;
	uint8_t* dataPtr = &image->dataStream[0];
	uint32_t mismatchCount = 0;
	for (memory_t section : image->sections)
	{

		uint32_t address = section.address + offset;

		std::cout << "Verifying section " << std::dec << c << " (" << section.size << " B) ..." << std::endl;

		if (!set_block_address(address))return false;

		std::vector<uint8_t> readData(section.size);

		if (!read_block(&readData[0], section.size))return false;


		for (uint16_t i = 0; i < section.size; i++)
		{
			if (dataPtr[i] != readData[i])
			{
				if (mismatchCount < 129)
					std::cout << "Mismatch at " << COUTHEX(address + i, 8) << ": Expected: " << COUTHEX(dataPtr[i], 2) << ", read: " << COUTHEX(readData[i], 2) << std::endl;
				else if (mismatchCount == 129)
					std::cout << " + more" << std::endl;
				(mismatchCount)++;
			}
		}

		dataPtr += section.size;
		c++;
	}
	if (mismatchCount)
	{
		std::cout << std::dec << mismatchCount << " mismatches total" << std::endl;
	}
	else
		std::cout << "Verify complete (match)" << std::endl;

	return true;
}

bool FastUPDI::verify_image(image_t* image, const std::string& memoryName)
{
	if (!device->HasMemory(memoryName))
	{
		std::cout << "Device does not have memory '" << memoryName << "' " << std::endl;
		return false;
	}
	memory_t mem = device->memories[memoryName];

	std::cout << "Verifying " << memoryName << " (" << std::dec << image->GetTotalSize() << " B) ..." << std::endl;

	return verify_image(image, mem.address);
}

constexpr uint8_t BlankData = 0xff;
bool FastUPDI::read_image(image_t* outImage, uint32_t length, uint32_t offset, EOutputMode readMode)
{

	uint32_t bytesLeft = length;
	uint32_t imageAddress = UINT32_MAX;

	std::cout << "Reading memory (" << std::dec << bytesLeft << " B) ..." << std::endl;
	if (!set_block_address(offset))return false; //Set address to start of offset (start of requested memory)

	print_progress_bar();

	uint32_t address = 0;
	outImage->dataStream.reserve(bytesLeft);
	while (bytesLeft)
	{
		uint16_t toRead = min(outImage->SectionMaxSize, bytesLeft);

		uint8_t* readData = new uint8_t[toRead];

		if (!read_block(readData, toRead)) { delete[] readData; end_progress_bar(); return false; }

		for (uint16_t i = 0; i < toRead; i++)
		{
			if (readMode != EOutputMode::SKIP || readData[i] != BlankData)
			{
				outImage->dataStream.push_back(readData[i]);
				bytesLeft--;

				if (address != imageAddress || outImage->sections.back().size >= outImage->SectionMaxSize)
				{
					//Add new section
					outImage->sections.push_back({ address, 0 });
				}
				outImage->sections.back().size++;
				imageAddress = address + 1;
			}
			address++;
		}

		float percent = ((float)(length - bytesLeft)) / length;
		update_progress_bar(percent);
	}
	end_progress_bar();
	if (readMode == EOutputMode::TRIMEND || readMode == EOutputMode::TRIMBOTH)
	{
		//Trim away blank bytes from end
		auto it = outImage->dataStream.end();
		while (!outImage->sections.empty())
		{
			uint32_t initialSize = outImage->sections.back().size;
			for (uint32_t toRemoveThisSection = 0; toRemoveThisSection < initialSize; toRemoveThisSection++)
			{
				it--;
				if (*it == BlankData)
				{
					outImage->sections.back().size--;
					if (outImage->sections.back().size == 0)
					{
						//Remove section if empty
						outImage->sections.pop_back();
					}
				}
				else
				{
					it++;
					goto _BREAK_TRIMEND;
				}

			}
		}
	_BREAK_TRIMEND:
		if (it != outImage->dataStream.end())
			outImage->dataStream.erase(it, outImage->dataStream.end());
	}

	if (readMode == EOutputMode::TRIMSTART || readMode == EOutputMode::TRIMBOTH)
	{
		//Trim away blank bytes from start
		auto it = outImage->dataStream.begin();
		while (!outImage->sections.empty())
		{
			uint32_t initialSize = outImage->sections.back().size;
			for (uint32_t toRemoveThisSection = 0; toRemoveThisSection < initialSize; toRemoveThisSection++)
			{
				if (*it == BlankData)
				{
					it++;
					outImage->sections.back().size--;
					outImage->sections.back().address++;
					if (outImage->sections.back().size == 0)
					{
						//Remove section if empty
						outImage->sections.pop_back();
					}
				}
				else goto _BREAK_TRIMSTART;

			}
		}
	_BREAK_TRIMSTART:
		if (it != outImage->dataStream.begin())
			outImage->dataStream.erase(outImage->dataStream.begin(), it);
	}

	std::cout << "Read complete" << std::endl;

	return true;
}

bool FastUPDI::read_image(image_t* outImage, const std::string& memoryName, EOutputMode readMode)
{
	if (!device->HasMemory(memoryName))
	{
		std::cout << "Device does not have memory '" << memoryName << "' " << std::endl;
		return false;
	}
	memory_t mem = device->memories[memoryName];

	std::cout << "Reading " << memoryName << " ..." << std::endl;

	return read_image(outImage, mem.size, mem.address, readMode);
}


bool FastUPDI::start(Serial* port, Device_t* _device)
{
	ser = port;
	device = _device;

	// Send START command
	int i = 0;
	while (1)
	{
		if (initiate_cmd(CMD_START))break;
		i++;
		if (i >= g_config.startAttempts) return false;
	}


	//Read initial status to check if the target is responding to communication
	uint8_t status;
	if (!ser->Read(&status, 1))
	{
		std::cout << "START failed: target is not responding" << std::endl;
		return false;
	}
	else if (status)
	{
		std::cout << "START failed: unexpected initial status: " << COUTHEX(status, 2) << std::endl;
		return false;
	}

	//Wait until target is in programming mode
	uint8_t repl;
	if (!ser->Read(&repl, 1))
	{
		std::cout << "START failed - target failed to enter programming mode: Timed out" << std::endl;
	}
	else if (repl != REPL_ACK)
	{
		std::cout << "START failed - target failed to enter programming mode: " << decode_error_reply(repl) << std::endl;
		return false;
	}

	//Send signature address
	I24TOBUF(addr, device->signatureAddress);
	ser->Write(addr, 3);

	//Read signature
	uint8_t recvd_signature[3];
	if (!ser->Read(recvd_signature, 3))
	{
		std::cout << "START failed - timeout while reading signature: " << std::endl;
		return false;
	}
	uint32_t signature = recvd_signature[2] | recvd_signature[1] << 8 | recvd_signature[0] << 16;

	//Check signature
	if (device->signature == signature)
		std::cout << "Read signature: " << COUTHEX(signature, 6) << " (match)" << std::endl;
	else {
		std::cout << "SIGNATURE MISMATCH! Read signature: " << COUTHEX(signature, 6) << ", expected: " << COUTHEX(device->signature, 6) << std::endl;
		return false;
	}

	//Check command result
	CHECKED(check_cmd_result());

	std::cout << "UPDI link started" << std::endl;
	return true;
}

bool FastUPDI::stop()
{
	//Send STOP command
	CHECKED(initiate_cmd(CMD_STOP));
	//Check command result
	CHECKED(check_cmd_result());
	return true;
}

bool FastUPDI::set_baud(uint32_t normalBaudrate)
{
	//Send command
	CHECKED(initiate_cmd(CMD_SETBAUD));

	//Send baudrate
	I32TOBUF(baudb, normalBaudrate);
	ser->Write(baudb, 2);
	CHECKED(wait_cmd_ACK());
	ser->Write(&baudb[2], 2);
	CHECKED(wait_cmd_ACK());

	//Check command result
	CHECKED(check_cmd_result());

	std::cout << "UPDI Baudrate set to: " << std::dec << normalBaudrate << std::endl;
	return true;
}

bool FastUPDI::recover()
{
	//Send command
	CHECKED(initiate_cmd(CMD_RECOVER));
	std::cout << "Recovering UPDI..." << std::endl;

	//Read status
	uint8_t repl;
	SerialReadByteOrReturn(&repl);
	std::cout << "Status: " << COUTHEX(repl, 2) << std::endl;

	//CCCheck command result
	CHECKED(check_cmd_result());
	return true;
}

bool FastUPDI::reset_target()
{
	//Send command
	CHECKED(initiate_cmd(CMD_RESETTGT));

	//Check command result
	CHECKED(check_cmd_result());

	std::cout << "Target reset" << std::endl;
	return true;
}

bool FastUPDI::read_byte(uint32_t address, uint8_t* outData)
{
	//Send command
	CHECKED(initiate_cmd(CMD_GETBYTE));

	//Send address
	I24TOBUF(addrb, address);
	ser->Write(addrb, 3);
	CHECKED(wait_cmd_ACK());

	//Read data
	SerialReadByteOrReturn(outData);

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::write_byte(uint32_t address, uint8_t data)
{
	//Send command
	CHECKED(initiate_cmd(CMD_SENDBYTE));

	//Send address
	I24TOBUF(addrb, address);
	ser->Write(addrb, 3);
	CHECKED(wait_cmd_ACK());

	//Send data
	ser->Write(&data, 1);
	CHECKED(wait_cmd_ACK());

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::verify_byte(uint32_t address, uint8_t data)
{
	std::cout << "Verifying byte..." << std::endl;
	uint8_t readData;
	CHECKED(read_byte(address, &readData));

	if (readData == data)
		std::cout << "Match" << std::endl;
	else
		std::cout << "Mismatch: expected: " << COUTHEX(data, 2) << ", read: " << COUTHEX(readData, 2) << std::endl;

	return true;
}

bool FastUPDI::set_block_address(uint32_t address)
{
	//Send command
	CHECKED(initiate_cmd(CMD_SETADDR));

	//Send address
	I24TOBUF(addrb, address);
	ser->Write(addrb, 3);
	CHECKED(wait_cmd_ACK());

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::read_block(uint8_t* outData, uint16_t size)
{
	//Send command
	CHECKED(initiate_cmd(CMD_GETBLOCK));

	//Send size
	I16TOBUF(sizeb, size);
	ser->Write(sizeb, 2);

	//Check reply (ACK if command is valid and programmer is ready to start sending data)
	CHECKED(wait_cmd_ACK());

	//Receive data
	if (!ser->Read(outData, size))
	{
		std::cout << "GETBLOCK failed: timeout while reading data" << std::endl;
		return false;
	}

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::write_block(uint8_t* data, uint16_t size)
{
	//Send command
	CHECKED(initiate_cmd(CMD_SENDBLOCK));

	//Send size
	I16TOBUF(sizeb, size);
	ser->Write(sizeb, 2);

	//Create progress bar
	print_progress_bar();

	//Send data
	uint16_t bytesLeft = size;
	while (bytesLeft)
	{
		uint16_t toSend = min(data_packet_size, bytesLeft);

		//Wait for programmer to signal ready to accept new data
		uint8_t repl;
		if (!ser->Read(&repl, 1))
		{
			end_progress_bar();
			std::cout << "SENDBLOCK failed: timeout while sending data" << std::endl;
			return false;
		}
		if (repl != REPL_ACK)
		{
			end_progress_bar();
			std::cout << "SENDBLOCK failed: " << decode_error_reply(repl) << std::endl;
			return false;
		}
		//Send packet
		ser->Write(data, toSend);
		data += toSend;
		bytesLeft -= toSend;

		float percent = (float)(size - bytesLeft) / size;
		update_progress_bar(percent);
	}
	end_progress_bar();

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::enable_flash_write()
{
	//Send command
	CHECKED(initiate_cmd(CMD_FLASH_WR_EN));

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::enable_eeprom_write()
{
	//Send command
	CHECKED(initiate_cmd(CMD_EEPROM_WR_EN));

	//Check command result
	CHECKED(check_cmd_result());

	return true;
}

bool FastUPDI::disable_nvm_write()
{
	//Send command
	CHECKED(initiate_cmd(CMD_NVM_WR_DIS));

	//Check command result
	CHECKED(check_cmd_result());
	return true;
}

bool FastUPDI::chip_erase()
{
	//Send command
	CHECKED(initiate_cmd(CMD_CHIPERASE));

	//Check command result
	CHECKED(check_cmd_result());

	std::cout << "Chip erased" << std::endl;
	return true;
}

bool FastUPDI::flash_page_erase(uint32_t startAddress, uint16_t nPages)
{
	//std::cout << "Page erase not implemented" << std::endl;
	//return false;
	///////////////////////////////////////////////////////

	//Send command
	CHECKED(initiate_cmd(CMD_FLASH_PAGE_ERASE));

	std::cout << "Erasing " << std::dec << nPages << " flash pages starting at: " << COUTHEX(startAddress, 8) << std::endl;

	//Send address
	I24TOBUF(addrb, startAddress);
	ser->Write(addrb, 3);
	CHECKED(wait_cmd_ACK());

	//Send n pages (size)
	I16TOBUF(nPagesb, nPages);
	ser->Write(nPagesb, 2);
	CHECKED(wait_cmd_ACK());

	//Send page size (step)
	uint16_t pageSize = device->flashPageSize;
	I16TOBUF(pageSizeb, pageSize);
	ser->Write(pageSizeb, 2);

	for (; nPages; nPages--)
	{
		//Listen for ACK signalling after each erased page
		CHECKED(wait_cmd_ACK());
	}


	//Check command result
	CHECKED(check_cmd_result());
	return true;
}


bool FastUPDI::initiate_cmd(CMD cmd)
{
	//Send command
	SerialWriteByte(cmd);
	uint8_t repl;
	SerialReadByteOrReturn(&repl);
	if (repl != REPL_ACK)
	{
		std::cout << decode_cmd(cmd) << " failed to initiate: " << decode_error_reply(repl) << std::endl;
		return false;
	}
	currentCMD = cmd;
	return true;
}

bool FastUPDI::wait_cmd_ACK()
{
	uint8_t repl;
	SerialReadByteOrReturn(&repl);
	if (repl != REPL_ACK)
	{
		std::cout << decode_cmd(currentCMD) << " failed during execution: " << decode_error_reply(repl) << std::endl;
		return false;
	}
	return true;
}

bool FastUPDI::check_cmd_result()
{
	//Check command result
	uint8_t repl;
	SerialReadByteOrReturn(&repl);
	bool isOK = repl == REPL_OK;
	if (!isOK)
	{
		std::cout << decode_cmd(currentCMD) << " failed: " << decode_error_reply(repl) << std::endl;
	}
	currentCMD = CMD_NONE;
	return isOK;
}

void FastUPDI::print_progress_bar(float percentage)
{
	std::cout << " [";
	for (uint8_t i = 0; i < PROGRESS_BAR_SIZE; i++)
	{
		progressBarSegments = (percentage * PROGRESS_BAR_SIZE);
		std::cout << (i <= progressBarSegments ? '#' : ' ');
	}
	std::cout << "] " << std::dec << std::setw(3) << int(percentage * 100) << '%' << std::flush;
	progressBarActive = true;
}

void FastUPDI::update_progress_bar(float percentage)
{
	uint8_t newSegments = (percentage * PROGRESS_BAR_SIZE);
	if (progressBarSegments != newSegments) {
		std::cout << '\r';
		print_progress_bar(percentage);
	}
}

inline void FastUPDI::end_progress_bar()
{
	if (progressBarActive)
	{
		std::cout << std::endl;
		progressBarActive = false;
	}

}


#define _CMD_CASE(__cmd) case __cmd: return #__cmd
std::string FastUPDI::decode_cmd(CMD _cmd)
{
	switch (_cmd)
	{
		_CMD_CASE(CMD_SENDBYTE);
		_CMD_CASE(CMD_SENDBLOCK);
		_CMD_CASE(CMD_GETBYTE);
		_CMD_CASE(CMD_GETBLOCK);
		_CMD_CASE(CMD_SETADDR);

		_CMD_CASE(CMD_CHIPERASE);
		_CMD_CASE(CMD_FLASH_PAGE_ERASE);

		_CMD_CASE(CMD_FLASH_WR_EN);
		_CMD_CASE(CMD_UROWUNLOCK);
		_CMD_CASE(CMD_EEPROM_WR_EN);
		_CMD_CASE(CMD_NVM_WR_DIS);

		_CMD_CASE(CMD_RECOVER);
		_CMD_CASE(CMD_START);
		_CMD_CASE(CMD_SETBAUD);
		_CMD_CASE(CMD_RESETTGT);
		_CMD_CASE(CMD_STOP);

	default:
		std::stringstream ss;
		ss << "Undefined CMD: " << COUTHEX(_cmd, 2);
		return ss.str();
	}
}


std::string FastUPDI::decode_error_reply(uint8_t reply)
{
	switch (reply)
	{
	case REPL_OK: return "Unexpected OK";
	case REPL_ACK: return "Unexpected ACK";
	case REPL_READY: return "Unexpected READY";
	case REPL_INVALID: return "Invalid operation";
	case REPL_UNDEFINED: return "Undefined operation";
	case REPL_ERROR: return "Unspecified error";
	case REPL_TIMEOUT: return "Target timed out";
	case REPL_UPDINOACK: return "Target did not acknowledge";
	case REPL_UPDIRXERR: return "UPDI Receive error";
	case REPL_ERASEFAIL: return "Erase operation failed";

	default:
		std::stringstream ss;
		ss << "Undefined reply: " << COUTHEX(reply, 2);
		return ss.str();
	}
}
