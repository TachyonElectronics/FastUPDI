#pragma once

#include <map>
#include <string>
#include <iostream>

enum class EOutputMode {
	FULL = 0,
	TRIMEND = 1,
	TRIMSTART = 2,
	TRIMBOTH = 3,
	SKIP = 4
};
extern std::map<std::string, EOutputMode> StoOutputMode;


enum class EResult : uint8_t
{
	SUCCESS = 0,

	NO_STOP = 1,
	NO_START = 10,
	UPDI_ERROR = 11,

	FILE_IO_ERROR = 20,
	FILE_FORMAT_ERROR = 21,
	CONFIG_ERROR = 22,
	PORT_ERROR = 30,

	BAD_COMMAND = 127,

};


inline void end(EResult result)
{
	std::cout << std::endl << "Done " << (result == EResult::SUCCESS ? "(Success)" : "(Failed)") << std::endl;
	exit((uint8_t)result);
}