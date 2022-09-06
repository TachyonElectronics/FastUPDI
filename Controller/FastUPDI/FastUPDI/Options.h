#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>
#include "FastUPDI.h"
#include "formats/formats.h"



struct Option_t
{
	const char* tag;
	uint8_t requiredOperands;
	uint8_t optionalOperands;
	std::string description;
};

struct Options
{
	static const Option_t* Help;
	static const Option_t* Config;
	static const Option_t* SerialBaudrate;
	static const Option_t* Port;
	static const Option_t* Device;
	static const Option_t* UPDIBaudrate;
	static const Option_t* MultiMode;


	static const Option_t* AutoVerify;
	static const Option_t* NoAutoVerify;
	static const Option_t* BaudrateOverride;
	static const Option_t* EraseMode;
	static const Option_t* OutputMode;
	static const Option_t* Format;
	static const Option_t* MemoryType;
};

struct Option_inst
{
	const Option_t* type;
	std::vector<std::string> operands;
	inline Option_inst(const Option_t* _type) : type(_type) {};
};

extern std::map<std::string, const Option_t*> StoOption;


extern const std::string UsageStr;