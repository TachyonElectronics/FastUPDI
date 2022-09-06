#include "memory.h"


std::map<std::string, EMemoryType> StoMemoryType =
{
	{"ram", EMemoryType::RAM},
	{"flash", EMemoryType::FLASH},
	{"eeprom", EMemoryType::EEPROM},
};

std::map<std::string, EEraseMode> StoEraseMode =
{
	{ "chip", EEraseMode::CHIP},
	{ "page", EEraseMode::PAGE },
	{ "none", EEraseMode::DONTERASE },
};

DEF_PROPERTIES(memory_t)
{
	PROPERTY_INT("address", memory_t::address),
	PROPERTY_INT("size", memory_t::size),
	PROPERTY_ENUM("type", memory_t::type, StoMemoryType),
	PROPERTY_ENUM("eraseMode", memory_t::eraseMode, StoEraseMode)
};

