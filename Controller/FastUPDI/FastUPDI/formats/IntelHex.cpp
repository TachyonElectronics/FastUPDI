#include "IntelHex.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

bool FIntelHex::Read(FILE* file, image_t* outImage) const
{
	uint32_t imageAddress = 0;
	uint16_t linearAddress = 0;
	uint16_t segmentAddress = 0;


	uint8_t bytecount;
	uint16_t address;
	uint8_t type;
	uint8_t data[255];
	uint8_t checksum;

	char c = fgetc(file);
	while (c != EOF)
	{
		if (c == ':')
		{
			uint8_t sum = 0; //For record checksum validation

			//Read byte count
			std::string s_bytecount;
			for (uint8_t i = 0; i < 2; i++)
			{
				c = fgetc(file); if (c == EOF) break; s_bytecount += c;
			}
			bytecount = std::stoi(s_bytecount, nullptr, 16);
			sum += bytecount;

			//Read address
			std::string s_address;
			for (uint8_t i = 0; i < 4; i++)
			{
				c = fgetc(file); if (c == EOF) break; s_address += c;
			}
			address = std::stoi(s_address, nullptr, 16);
			sum += (uint8_t)address + (uint8_t)(address >> 8);

			//Read type
			std::string s_type;
			for (uint8_t i = 0; i < 2; i++)
			{
				c = fgetc(file); if (c == EOF) break; s_type += c;
			}
			type = std::stoi(s_type, nullptr, 16);
			sum += type;

			//Read data
			for (uint8_t j = 0; j < bytecount; j++)
			{
				std::string s_data;
				for (uint8_t i = 0; i < 2; i++)
				{
					c = fgetc(file); if (c == EOF) break; s_data += c;
				}
				data[j] = std::stoi(s_data, nullptr, 16);
				sum += data[j];
			}

			//Read checksum
			std::string s_checksum;
			for (uint8_t i = 0; i < 2; i++)
			{
				c = fgetc(file); if (c == EOF) break; s_checksum += c;
			}
			checksum = std::stoi(s_checksum, nullptr, 16);
			sum += checksum;

			//Process record:

			//Verify checksum
			if (sum)
			{
				std::cout << "Bad checksum in hex file" << std::endl;
				return false;
			}

			switch ((EHexRecordType)type)
			{
			case EHexRecordType::DATA:
			{
				uint32_t recordAddress = ((linearAddress << 16) | address) + (segmentAddress * 16);
				if (recordAddress != imageAddress || outImage->sections.empty())
				{
					//create new section if next address is non-sequential or there are no sections yet
					outImage->sections.push_back({ recordAddress, 0 });
				}
				for (uint8_t i = 0; i < bytecount; i++)
				{
					if (outImage->sections.back().size >= outImage->SectionMaxSize)
					{
						//create new section if current section is full
						outImage->sections.push_back({ recordAddress + i, 0 });
					}
					outImage->dataStream.push_back(data[i]);
				}
				outImage->sections.back().size += bytecount;
				imageAddress = recordAddress + bytecount;
			}
			break;

			case EHexRecordType::EXTENDED_LINEAR_ADDRESS:
				linearAddress = (data[0] << 8) | data[1];
				break;

			case EHexRecordType::EXTENDED_SEGMENT_ADDRESS:
				segmentAddress = (data[0] << 8) | data[1];
				break;

			case EHexRecordType::END_OF_FILE:
				return true;

			case EHexRecordType::START_LINEAR_ADDRESS:
			case EHexRecordType::START_SEGMENT_ADDRESS:
				std::cout << "Warning: HEX file specifies a non-zero start address - program is probably configured for use with a bootloader. Ensure that the chip fuses are set accordingly." << std::endl;
				break;

			default:
				std::cout << "Unsupported HEX record type: " << std::hex << (int)type;
				return false;
			}

		}
		c = fgetc(file);
	}

	std::cout << "Unexpected end of file" << std::endl;
	return false;
}

constexpr uint8_t max_bytecount = 32;
bool FIntelHex::Write(FILE* file, image_t* image) const
{
	uint32_t imageAddress = 0;
	uint16_t linearAddress = 0;
	uint16_t segmentAddress = 0;
	
	uint8_t bytecount;
	uint16_t address;
	uint8_t type;
	uint8_t data[max_bytecount];

	uint8_t* dataPtr = &image->dataStream[0];
	for (memory_t section : image->sections)
	{
		uint32_t bytesLeft = section.size;
		uint32_t imageAddress = section.address;
		while (bytesLeft)
		{
#ifdef USE_LINEAR_ADDRESS
			uint16_t recordLinearAddress = imageAddress >> 16;
			if (recordLinearAddress != linearAddress)
			{
				//Write new segment address
				bytecount = 2;
				data[0] = (uint8_t)(recordLinearAddress >> 8);
				data[1] = (uint8_t)(recordLinearAddress);
				address = 0;
				type = (uint8_t)EHexRecordType::EXTENDED_LINEAR_ADDRESS;
				linearAddress = recordLinearAddress;
			}
#else
			uint16_t recordSegmentAddress = (imageAddress / 0xFFFF) << 12;
			if (recordSegmentAddress != segmentAddress)
			{
				//Write new segment address
				bytecount = 2;
				data[0] = (uint8_t)(recordSegmentAddress >> 8);
				data[1] = (uint8_t)(recordSegmentAddress);
				address = 0;
				type = (uint8_t)EHexRecordType::EXTENDED_SEGMENT_ADDRESS;
				segmentAddress = recordSegmentAddress;
			}
#endif
			else
			{
				//Write data
				bytecount = std::min<uint32_t>(max_bytecount, bytesLeft);
				for (uint8_t i = 0; i < bytecount; i++)
				{
					data[i] = *dataPtr;
					dataPtr++;
				}
				address = imageAddress & 0xFFFF;
				type = (uint8_t)EHexRecordType::DATA;
				bytesLeft -= bytecount;
				imageAddress += bytecount;
			}
			
			//Write record
			int8_t checksum = (int8_t)bytecount + (int8_t)(address >> 8) + (int8_t)(address & 0xff) + (int8_t)type;
			std::ostringstream record;
			record << ':' << std::uppercase;
			record << std::hex << std::setw(2) << std::setfill('0') << (int)bytecount;
			record << std::setw(4) << (int)address;
			record << std::setw(2) << (int)type;
			for (uint8_t i = 0; i < bytecount; i++)
			{
				record << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
				checksum += data[i];
			}
			checksum = -checksum;
			record << std::hex << std::setw(2) << std::setfill('0') << (((int)checksum) & 0xff) << std::endl;

			for (char c : record.str())
			{
				fputc(c, file);
			}
		}
	}

	//Write HEX EOF:
	fputs(":00000001FF", file);

	return true;
}
