#pragma once

#include <map>
#include "Options.h"
#include "global.h"

template<typename T>
T* Create() { return new T(); }

template<typename T>
void Destroy(T* object) { delete object; }
#define DECL_DYNAMIC_DESTRUCTOR void (*_destroy_func)(void*)
#define DEFINE_DYNAMIC_DESTRUCTOR(Typename) _destroy_func = &Destroy<Typename>

class FastUPDI;
class Operation
{
public:
	const char* tag = "-INVALID_OPERATION";
	const char* description = "(INVALID OPERATION)";
	const char* extendedDescription = nullptr;
	std::vector<const Option_t*> allowedOptions;

	std::vector<Option_inst*> options;
	std::vector<std::string> operands;

	virtual EResult Execute(FastUPDI* updi);

	virtual ~Operation()
	{
		for (Option_inst* o : options)
			delete o;
	}
};

class WriteOperation : public Operation
{
public:
	virtual EResult Execute(FastUPDI* updi) override;

	WriteOperation()
	{
		tag = "-W";
		description = " <destination> <data> [options]:	Write to memory";
		extendedDescription = "   <destination>: target memory name or '0x'-prefixed hex address\r\n   <data>: Source filename or '0x'-prefixed hex direct data (single byte)";
		allowedOptions =
		{
			Options::Format,
			Options::EraseMode,
			Options::AutoVerify,
			Options::NoAutoVerify,
			Options::BaudrateOverride,
			Options::MemoryType
		};
	}
};
class ReadOperation : public Operation
{
public:
	virtual EResult Execute(FastUPDI* updi) override;

	ReadOperation()
	{
		tag = "-R";
		description = " <source> [output] [options]:		Read memory and print to file or console";
		extendedDescription = "   <source>: source memory name or '0x'-prefixed hex address\r\n   [output]: Output filename (optional)";
		allowedOptions =
		{
			Options::Format,
			Options::OutputMode
		};
	}
};
class VerifyOperation : public Operation
{
public:
	virtual EResult Execute(FastUPDI* updi) override;

	VerifyOperation()
	{
		tag = "-V";
		description = " <source> <template> [options]:	Verify data from memory against specified data";
		extendedDescription = "   <source>: source memory name or '0x'-prefixed hex address\r\n   <template>: Template filename or '0x'-prefixed hex direct data (single byte)";
		allowedOptions = { Options::Format };
	}
};
class EraseOperation : public Operation
{
public:
	virtual EResult Execute(FastUPDI* updi) override;

	EraseOperation()
	{
		tag = "-E";
		description = ":					Perform a chip erase";
	}
};

typedef Operation* (*OperationCreationFunc)();

extern std::map<const std::string, OperationCreationFunc> StoOpCreationFunc;
