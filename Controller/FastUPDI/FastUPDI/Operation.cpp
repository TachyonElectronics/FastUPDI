#include "Util.h"
#include "formats/formats.h"
#include "Config.h"
#include "Operation.h"


std::map<const std::string, OperationCreationFunc> StoOpCreationFunc =
{
	{"-W", (OperationCreationFunc)&Create<WriteOperation>},
	{"-R", (OperationCreationFunc)&Create<ReadOperation>},
	{"-V", (OperationCreationFunc)&Create<VerifyOperation>},
	{"-E", (OperationCreationFunc)&Create<EraseOperation>},
};

EResult WriteOperation::Execute(FastUPDI* updi)
{
	//Check operands
	if (operands.size() != 2)
	{
		std::cout << tag << " expects 2 operands, specified " << operands.size() << std::endl;
		return EResult::BAD_COMMAND;
	}
	memory_t mem;
	EMemoryType memtype = EMemoryType::RAM;
	EEraseMode eraseMode = EEraseMode::DONTERASE;
	uint32_t address;
	bool directAddress;
	if (try_convert_hex(operands[0], (int*)&address)) //Try to resolve direct hex address
	{
		directAddress = true;
	}
	else
	{
		//Reslove address from memory name

		if (!SearchMap(updi->device->memories, operands[0], &mem))
		{
			std::cout << "Device does not have memory '" << operands[0] << "'" << std::endl;
			return EResult::BAD_COMMAND;
		}
		address = mem.address;
		directAddress = false;
		memtype = mem.type;
		eraseMode = mem.eraseMode;
	}

	//Process options
	const Format* format = g_config.defaultFormat;
	bool autoVerify = g_config.autoVerifyAfterWrite;
	uint32_t baudrateOverride = 0;


	for (auto opt : options)
	{
		if (opt->type == Options::AutoVerify)
			autoVerify = true;
		else if (opt->type == Options::NoAutoVerify)
			autoVerify = false;
		else if (opt->type == Options::Format)
		{
			if (!SearchMap(StoFormat, opt->operands[0], &format))
			{
				std::cout << "Unrecognized format: " << opt->operands[0] << std::endl;
				return EResult::BAD_COMMAND;
			}
		}
		else if (opt->type == Options::EraseMode)
		{
			if (!SearchMap(StoEraseMode, opt->operands[0], &eraseMode))
			{
				std::cout << "Unrecognized erase mode: " << opt->operands[0] << std::endl;
				return EResult::BAD_COMMAND;
			}
		}
		else if (opt->type == Options::BaudrateOverride)
		{
			baudrateOverride = try_stoi(opt->operands[0]);
		}
		else if (opt->type == Options::MemoryType)
		{
			if (!SearchMap(StoMemoryType, opt->operands[0], &memtype))
			{
				std::cout << "Unrecognized memory type mode: " << opt->operands[0] << std::endl;
				return EResult::BAD_COMMAND;
			}
		}
	}

	image_t image;

	int directData;
	if (try_convert_hex(operands[1], &directData)) //Try to resolve direct hex data
	{
		//Create image from direct data
		if (format != g_config.defaultFormat) std::cout << "Ignoring format parameter (writing direct data)" << std::endl;
		image.sections.push_back({ 0, 1 });
		image.dataStream.push_back((uint8_t)directData);
	}
	else
	{
		//Otherwise read image from file
		FILE* file;
		fopen_s(&file, operands[1].c_str(), "r");
		if (!file)
		{
			std::cout << "Could not open file for reading: " << operands[1];
			return EResult::FILE_IO_ERROR;
		}
		if (!format->Read(file, &image)) return EResult::FILE_FORMAT_ERROR;
		fclose(file);
	}


	//Erase if memory is flash type
	if (memtype == EMemoryType::FLASH)
	{
		switch (eraseMode) {
		case EEraseMode::CHIP:
		{
			if (!updi->chip_erase()) return EResult::UPDI_ERROR;
		}
		break;

		case EEraseMode::PAGE:
		{
			uint32_t lastErasedPage = UINT32_MAX;
			for (memory_t section : image.sections)
			{
				uint32_t firstPage = section.address / updi->device->flashPageSize;

				//Skip first page if it was already previously erased
				if (firstPage == lastErasedPage)
					firstPage++;

				uint32_t lastPage = section.last_address() / updi->device->flashPageSize;
				uint32_t requiredPages = lastPage - firstPage + 1;

				if (!updi->flash_page_erase(firstPage * updi->device->flashPageSize + address, requiredPages));
				lastErasedPage = lastPage;
			}
		}
		break;
		}
	}
	else
		if (eraseMode != EEraseMode::DONTERASE) std::cout << "Ignoring erase mode parameter (not writing flash)" << std::endl;


	//Override baudrate if needed
	uint32_t maxBaudrate;
	switch (memtype)
	{
	case EMemoryType::FLASH:
		maxBaudrate = updi->device->flashWriteMaxBaudrate;
		break;
	case EMemoryType::EEPROM:
		maxBaudrate = updi->device->eepromWriteMaxBaudrate;
	default:
		maxBaudrate = UINT32_MAX;
	}

	if (!baudrateOverride && updi->normalBaudrate > maxBaudrate)
		baudrateOverride = maxBaudrate;

	if (baudrateOverride)
		if (!updi->set_baud(baudrateOverride)) return EResult::UPDI_ERROR;


	//Write
	if (memtype == EMemoryType::RAM && baudrateOverride != 0) std::cout << "Ignoring baudrate override parameter (not writing flash or eeprom)" << std::endl;

	if (!updi->write_image(&image, address, memtype)) return EResult::UPDI_ERROR;
	

	//Revert baudrate override
	if (baudrateOverride)
		if (!updi->set_baud(updi->normalBaudrate)) return EResult::UPDI_ERROR;


	if (autoVerify)
		if (!updi->verify_image(&image, address)) return EResult::UPDI_ERROR;

	return EResult::SUCCESS;
}

EResult VerifyOperation::Execute(FastUPDI* updi)
{
	//Check operands
	if (operands.size() != 2)
	{
		std::cout << tag << " expects 2 operands, specified " << operands.size() << std::endl;
		return EResult::BAD_COMMAND;
	}
	uint32_t address;
	bool directAddress;
	if (try_convert_hex(operands[0], (int*)&address)) //Try to resolve direct hex address
	{
		directAddress = true;
	}
	else
	{
		//Reslove address from memory name
		memory_t mem;
		if (!SearchMap(updi->device->memories, operands[0], &mem))
		{
			std::cout << "Device does not have memory '" << operands[0] << "'" << std::endl;
			return EResult::BAD_COMMAND;
		}
		address = mem.address;
		directAddress = false;
	}

	//Process options
	const Format* format = g_config.defaultFormat;

	for (auto opt : options)
	{
		if (opt->type == Options::Format)
		{
			if (!SearchMap(StoFormat, opt->operands[0], &format))
			{
				std::cout << "Unrecognized format: " << opt->operands[0] << std::endl;
				return EResult::BAD_COMMAND;
			}
		}
	}


	int directData;
	if (try_convert_hex(operands[1], &directData)) //Try to resolve direct hex data
	{
		//Verify single byte
		if (format != g_config.defaultFormat) std::cout << "Ignoring format parameter (writing direct hex data)" << std::endl;

		if (!updi->verify_byte(address, (uint8_t)directData)) return EResult::UPDI_ERROR;
	}
	else
	{
		//Verify image
		FILE* file;
		fopen_s(&file, operands[1].c_str(), "r");
		if (!file)
		{
			std::cout << "Could not open file for reading: " << operands[1];
			return EResult::FILE_IO_ERROR;
		}
		image_t image;
		if (!format->Read(file, &image)) return EResult::FILE_FORMAT_ERROR;
		fclose(file);

		if (directAddress)
		{
			if (!updi->verify_image(&image, address)) return EResult::UPDI_ERROR;
		}
		else
		{
			if (!updi->verify_image(&image, address)) return EResult::UPDI_ERROR;
		}

	}
	return EResult::SUCCESS;
}

EResult ReadOperation::Execute(FastUPDI* updi)
{
	//Check operands
	if (operands.size() != 1 && operands.size() != 2)
	{
		std::cout << tag << " expects 1 or 2 operands, specified " << operands.size() << std::endl;
		return EResult::BAD_COMMAND;
	}

	bool directOutput = operands.size() == 1;

	//process options and operands
	const Format* format = g_config.defaultFormat;
	EOutputMode readMode = (EOutputMode)g_config.defaultOutputMode;

	bool directAddress;
	int directAddressValue;
	directAddress = try_convert_hex(operands[0], &directAddressValue);

	for (auto opt : options)
	{
		if (opt->type == Options::Format)
			if (!SearchMap(StoFormat, opt->operands[0], &format))
			{
				std::cout << "Unrecognized format: " << opt->operands[0] << std::endl;
				return EResult::BAD_COMMAND;
			}
			else if (opt->type == Options::OutputMode)
				if (!SearchMap(StoOutputMode, opt->operands[0], &readMode))
				{
					std::cout << "Unrecognized read mode: " << opt->operands[0] << std::endl;
					return EResult::BAD_COMMAND;
				}
	}

	image_t image;
	if (directAddress)
	{
		//Read direct byte
		if (!updi->read_image(&image, 1, directAddressValue, readMode)) return EResult::UPDI_ERROR;
	}
	else
	{
		//Read memory location
		if (!updi->read_image(&image, operands[0], readMode)) return EResult::UPDI_ERROR;
	}

	if (directOutput)
	{
		//Output to console
		if (format != g_config.defaultFormat) std::cout << "Ignoring format parameter (directly outputting to console)" << std::endl;
		if (image.sections.empty())
			std::cout << "   (empty)" << std::endl;
		else
		{
			uint8_t* dataPtr = &image.dataStream[0];
			for (auto section : image.sections)
			{
				for (uint32_t i = 0; i < section.size; i++)
					std::cout << "   " << COUTHEX((section.address + i), 8) << ": " << COUTHEX(*(dataPtr++), 2) << std::endl;
			}
		}
	}
	else
	{
		//Output to file
		FILE* file;
		fopen_s(&file, operands[1].c_str(), "w");
		if (!file)
		{
			std::cout << "Could not open file for writing: " << operands[1];
			return EResult::FILE_IO_ERROR;
		}
		if (!format->Write(file, &image)) return EResult::FILE_FORMAT_ERROR;
		fclose(file);
	}

	return EResult::SUCCESS;
}

EResult EraseOperation::Execute(FastUPDI* updi)
{
	if (!updi->chip_erase()) return EResult::UPDI_ERROR;
	return EResult::SUCCESS;
}

EResult Operation::Execute(FastUPDI* updi)
{
	return EResult::BAD_COMMAND;
}