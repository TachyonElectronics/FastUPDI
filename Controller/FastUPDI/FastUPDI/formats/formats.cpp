#include "formats.h"

std::map<std::string, const Format*> StoFormat =
{
	{"hex", new FIntelHex()},
};