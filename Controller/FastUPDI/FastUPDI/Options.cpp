#include "Options.h"

const std::string UsageStr = "Usage: [global options] [operation_1 [operands] [options]] ... [operation_n [operands] [options]]\r\nAll arguments are case-sensitive";


const Option_t* Options::Help = new Option_t				{ "-?", 0, 0, " [-Operation]			Display this help message or operation-specific help message" };
const Option_t* Options::Config = new Option_t				{ "-c", 1, 0, " <filename>			Specify config file" };
const Option_t* Options::SerialBaudrate = new Option_t		{ "-sb", 1, 0, " <baudrate>			Specify serial port baudrate" };
const Option_t* Options::Port = new Option_t				{ "-p", 1, 0, " <port>			Specify port (Required)" };
const Option_t* Options::Device = new Option_t				{ "-d", 1, 0, " <device>			Select device (Required)" };
const Option_t* Options::UPDIBaudrate = new Option_t		{ "-b", 1, 0, " <baudrate>			Specify UPDI baudrate" };
const Option_t* Options::MultiMode = new Option_t			{ "-m", 0, 0, "				Enable Multi-mode (repeat the same operation until cancelled)" };

const Option_t* Options::AutoVerify = new Option_t			{ "-av", 0, 0,    "				Auto-verify after writing (if disabled by default in config)" };
const Option_t* Options::NoAutoVerify = new Option_t		{ "-av-no", 0, 0, "				Don't Auto-verify after writing (if enabled by default in config)" };
const Option_t* Options::BaudrateOverride = new Option_t	{ "-bo", 1, 0,    " <baudrate>			Override UPDI baudrate for this operation (flash & eeprom write only)" };
const Option_t* Options::EraseMode = new Option_t			{ "-em", 1, 0, " <mode>			(flash only) Specify pre-write erase mode (Default set by config):\r\n					none: Don't erase\r\n					chip: Chip erase\r\n					page: Page erase" };
const Option_t* Options::OutputMode = new Option_t			{ "-om", 1, 0, " <mode>			Specify output mode (Default set by config):\r\n					full: Output entire range\r\n					trim-end: trim away blank bytes from end of output\r\n					trim-start: trim away blank bytes from start of output\r\n					trim-both: trim away blank bytes from start and end of output\r\n					skip: Don't output any blank bytes" };
const Option_t* Options::Format = new Option_t				{ "-f", 1, 0, " <format>			Specify file format:\r\n					hex: Intel HEX (Default)" };
const Option_t* Options::MemoryType = new Option_t			{ "-t", 1, 0, " <type>			Specify memory type: [ ram (Default) | eeprom | flash ]" };

#define DEFINE_S_TO_OPTION(option) {Options::option->tag, Options::option}
std::map<std::string, const Option_t*> StoOption =
{
	DEFINE_S_TO_OPTION(Options::Help),
	DEFINE_S_TO_OPTION(Options::Config),
	DEFINE_S_TO_OPTION(Options::SerialBaudrate),
	DEFINE_S_TO_OPTION(Options::Port),
	DEFINE_S_TO_OPTION(Options::Device),
	DEFINE_S_TO_OPTION(Options::UPDIBaudrate),
	DEFINE_S_TO_OPTION(Options::MultiMode),


	DEFINE_S_TO_OPTION(Options::AutoVerify),
	DEFINE_S_TO_OPTION(Options::NoAutoVerify),
	DEFINE_S_TO_OPTION(Options::BaudrateOverride),
	DEFINE_S_TO_OPTION(Options::EraseMode),
	DEFINE_S_TO_OPTION(Options::OutputMode),
	DEFINE_S_TO_OPTION(Options::Format),
	DEFINE_S_TO_OPTION(Options::MemoryType),
};
