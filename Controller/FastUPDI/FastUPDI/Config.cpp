#include <iostream>
#include "Options.h"
#include "Config.h"

DEF_PROPERTIES(Device_t)
{
	PROPERTY_INT("signature", Device_t::signature),
	PROPERTY_INT("signatureAddress", Device_t::signatureAddress),

	PROPERTY_INT("flashPageSize", Device_t::flashPageSize),
	PROPERTY_INT("flashWriteMaxBaudrate", Device_t::flashWriteMaxBaudrate),
	PROPERTY_INT("eepromWriteMaxBaudrate", Device_t::eepromWriteMaxBaudrate),

	PROPERTY_INT("defaultBaudrate", Device_t::defaultBaudrate),

	PROPERTY_STRMAP("memories", Device_t::memories, DeserializeObject<memory_t>)
};

DEF_PROPERTIES(Config_t)
{
	PROPERTY_INT("programmerTimeout", Config_t::programmerTimeout),
	PROPERTY_INT("startAttempts", Config_t::startAttempts),
	PROPERTY_INT("autoVerifyAfterWrite", Config_t::autoVerifyAfterWrite),
	PROPERTY_ENUM("defaultOutputMode", Config_t::defaultOutputMode, StoOutputMode),
	PROPERTY_INT("defaultSerialBaudrate", Config_t::defaultSerialBaudrate),
	PROPERTY_STRMAP("devices", Config_t::devices, DeserializeObject<Device_t>)
};

Config_t g_config;

bool LoadConfig(std::string& path)
{
	FILE* config_handle;
	fopen_s(&config_handle, path.c_str(), "r");
	if (!config_handle)
	{
		std::cout << "Cannot open config file: " << path << std::endl;
		return false;
	}
	if (!DeserializeObject<Config_t>(&g_config, config_handle))
	{
		fclose(config_handle);
		std::cout << "Failed to load config file: " << path << std::endl;
		return false;
	}
	fclose(config_handle);
	return true;
}
