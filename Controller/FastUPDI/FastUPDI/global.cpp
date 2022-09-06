#include "global.h"

std::map<std::string, EOutputMode> StoOutputMode =
{
	{ "full", EOutputMode::FULL},
	{ "trim-end", EOutputMode::TRIMEND },
	{ "trim-start", EOutputMode::TRIMSTART },
	{ "trim-both", EOutputMode::TRIMBOTH },
	{ "skip", EOutputMode::SKIP },
};