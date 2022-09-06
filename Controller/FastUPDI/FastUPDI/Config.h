#pragma once

#include <stdint.h>
#include <map>
#include <string>
#include "Serialization.h"
#include <stdio.h>
#include "memory.h"
#include "formats/formats.h"
#include "global.h"

struct Device_t
{
	uint32_t signature;
	uint32_t signatureAddress;

	uint16_t flashPageSize;
	uint32_t flashWriteMaxBaudrate;
	uint32_t eepromWriteMaxBaudrate;

	uint32_t defaultBaudrate;

	std::map<std::string, memory_t> memories;

	const inline bool HasMemory(const std::string memory) { return memories.find(memory) != memories.end(); }

	DECL_PROPERTIES;
};

//Config file wrapper
struct Config_t
{
	uint32_t programmerTimeout = 1000;
	int startAttempts;
	bool autoVerifyAfterWrite;

	EOutputMode defaultOutputMode;
	uint32_t defaultSerialBaudrate;


	Format* defaultFormat = new FIntelHex();


	std::map<std::string, Device_t> devices;

	DECL_PROPERTIES;
};

bool LoadConfig(std::string& path);

extern Config_t g_config;