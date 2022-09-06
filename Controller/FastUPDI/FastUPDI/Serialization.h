#pragma once

#include <string>
#include <map>
#include <iostream>
#include "Util.h"

template<typename Class, typename T>
constexpr size_t GetOffset(T Class::* variable)
{
	return (size_t) & (((Class*)nullptr)->*variable);
}

typedef bool (*DeserializeFunc)(void* outObj, FILE* file);

struct Property_t
{
	size_t offset;
	DeserializeFunc deserializeFunction;

	//Return pointer to the property's value (variable) inside the specified object
	template<typename T = void>
	inline T* Ptr(void* object) { return (T*)(((uint8_t*)object) + offset); }
};

struct Properties_t
{

protected:
	std::map< std::string, Property_t>  propertyMap;
public:

	//Find property metadata
	inline Property_t* Find(std::string propertyName)
	{
		std::map<std::string, Property_t>::iterator prop_it = propertyMap.find(propertyName);
		if (prop_it == propertyMap.end())
			return nullptr; //Return null if not found

		return &(*prop_it).second;
	}

	inline Properties_t(std::initializer_list<std::pair<const std::string, Property_t>> init)
	{
		propertyMap = init;
	}
};


inline char ser_readchar(FILE* file)
{
	char c;
	c = fgetc(file);
	if (c == '#')
	{
		//Skip until new line or EOF if comment delimiter is found
		do {
			c = fgetc(file);
		} while (c != '\n' && c != EOF);
	}
	return c;
}
//Returns false if 
inline bool ser_readchar_noeof(FILE* file, char* outChar)
{
	char c = ser_readchar(file);
	if (c == EOF) {
		std::cout << "BAD CONFIG: Unexpected end of file" << std::endl;
		return false;
	}
	*outChar = c;
	return true;
}
#define READC_CHECKED(file, outChar)if(!ser_readchar_noeof(file, outChar)) return false;

template<typename T>
bool DeserializeInt(void* outObj, FILE* file)
{
	std::string input;
	char c;
	//skip any preceeding whitespaces
	do
	{
		READC_CHECKED(file, &c);
	} while (std::isspace(c));

	//Read until end of record
	do {
		input += c;
		READC_CHECKED(file, &c);
	} while (c != ';');

	bool isHex = input[0] == '0' && (input[1] == 'x' || input[1] == 'X');
	if (isHex) input.erase(0, 2);

	T value = std::stoi(input, nullptr, isHex ? 16 : 10);
	*((T*)outObj) = value;

	return true;
}

template<typename T, std::map<std::string, T>& ConvMap>
bool DeserializeEnum(void* outObj, FILE* file)
{
	std::string input;
	char c;
	//skip any preceeding whitespaces
	do
	{
		READC_CHECKED(file, &c);
	} while (std::isspace(c));

	//Read until end of record
	do {
		input += c;
		READC_CHECKED(file, &c);
	} while (c != ';');

	if (!SearchMap(ConvMap, input, (T*)outObj))
	{
		std::cout << "BAD CONFIG: undefined enum value: " << input << std::endl;
		return false;
	}

	return true;
}

template<typename Class>
bool DeserializeObject(void* outObj, FILE* file)
{
	std::string input;
	char c;
	// move to to start of object {
	do
	{
		READC_CHECKED(file, &c);
	} while (c != '{');

	while (1) {
		//move to next property identifier or end of object }
		do
		{
			READC_CHECKED(file, &c);
			if (c == '}')
				return true;
		} while (std::isspace(c));

		//Read property identifier
		while (c != '=')
		{
			if (!std::isspace(c)) input += c;
			READC_CHECKED(file, &c);
		}

		//Deserialize value
		Property_t* prop = Class::_properties.Find(input);
		if (!prop)
		{
			std::cout << "BAD CONFIG: property '" << input << "' specified but non-existent in parent object" << std::endl;
			return false;
		}
		if (!prop->deserializeFunction(prop->Ptr(outObj), file)) return false;

		input.clear();
	}

	return true;
}

template<DeserializeFunc DsFunc, typename MapType>
bool DeserializeStrMap(void* outMap, FILE* file)
{
	std::string input;
	char c;
	// move to to start of objecct {
	do
	{
		READC_CHECKED(file, &c);
	} while (c != '{');

	while (1) {
		//move to next property identifier or end of object }
		do
		{
			READC_CHECKED(file, &c);
			if (c == '}')
				return true;
		} while (std::isspace(c));

		//Read property identifier
		while (c != '=')
		{
			if (!std::isspace(c)) input += c;
			READC_CHECKED(file, &c);
		}
		//Deserialize value
	//	auto newItem = ((std::map<std::string, uint32_t>*)outMap)->insert(std::make_pair(input, (std::map<std::string, uint32_t>::value_type)())); //Add key with uninitialized value
		void* target = &(*((MapType*)outMap))[input]; //Insert a new key and get pointer to its uninitialized value
		if (!DsFunc(target, file)) return false; //Deserialize value

		input.clear();
	}

	return true;
}


#define DECL_PROPERTIES static Properties_t _properties
#define DEF_PROPERTIES(_class) Properties_t _class::_properties = 

#define PROPERTY(name, variable, deserializeMethod) {name, {GetOffset(&variable), &deserializeMethod}} //Generic property
#define PROPERTY_INT(name, variable) {name, {GetOffset(&variable), &DeserializeInt<decltype(variable)>}} //Int property
#define PROPERTY_ENUM(name, variable, conversionMap) {name, {GetOffset(&variable), &DeserializeEnum<decltype(variable), conversionMap>}} //Enum property
#define PROPERTY_STRMAP(name, variable, valueDeserializeMethod) {name, {GetOffset(&variable), &DeserializeStrMap<&valueDeserializeMethod, decltype(variable)>}} //std::map<std::string, ...> property
