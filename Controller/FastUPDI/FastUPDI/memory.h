#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include "Serialization.h"

enum class EMemoryType : uint8_t
{
	RAM = 0,
	EEPROM = 1,
	FLASH = 2
};
extern std::map<std::string, EMemoryType> StoMemoryType;

enum class EEraseMode {
	DONTERASE = 0,
	CHIP = 1,
	PAGE = 2,
};
extern std::map<std::string, EEraseMode> StoEraseMode;

struct memory_t
{
	uint32_t address;
	uint32_t size;

	EMemoryType type = EMemoryType::RAM;
	EEraseMode eraseMode = EEraseMode::DONTERASE;

	//Address of memory's end (points right next outside the memory range)
	inline uint32_t end_address() const { return address + size; }
	//Address of the last byte in this memory
	inline uint32_t last_address() const { return end_address() - 1; }

	DECL_PROPERTIES;
};

struct image_t
{

	static constexpr uint32_t SectionMaxSize = 32768;

	//List of sections (each max 32KB in size!)
	std::vector<memory_t> sections;

	std::vector<uint8_t> dataStream;
	inline uint32_t GetTotalSize() const
	{
		uint32_t size = 0;
		for (memory_t section : sections)
			size += section.size;
		return size;
	}
};