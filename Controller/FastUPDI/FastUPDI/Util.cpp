#include <stdexcept>
#include <iostream>
#include "global.h"
#include "Util.h"

int try_stoi(std::string& string, int base)
{
	try
	{
		return std::stoi(string, nullptr, base);
	}
	catch (std::invalid_argument e)
	{
		std::cout << "Invalid number: " << string << std::endl;
		end(EResult::BAD_COMMAND);
	}
	return 0;
}

bool try_convert_hex(std::string& string, int* outHex)
{
	if(string.length() < 3)	return false;
	if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
	{
		string.erase(0, 2);
		*outHex = try_stoi(string, 16);
		return true;
	}
	return false;
}
