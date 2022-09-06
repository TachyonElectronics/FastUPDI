#pragma once
#include <string>
#include <map>


int try_stoi(std::string& string, int base = 10);

bool try_convert_hex(std::string& string, int* outHex);

template<typename Tkey, typename Tvalue>
bool SearchMap(std::map<Tkey, Tvalue>& map, const Tkey& key, Tvalue* outValue)
{
	typename std::map<Tkey, Tvalue>::iterator it = map.find(key);
	if (it == map.end()) return false;

	*outValue = it->second;
	return true;
}