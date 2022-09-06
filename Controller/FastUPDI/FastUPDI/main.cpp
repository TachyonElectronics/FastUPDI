

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <conio.h>
#include "Serial.h"
#include "FastUPDI.h"
#include "formats/formats.h"
#include "Options.h"
#include "Operation.h"
#include "global.h"
#include "Util.h"

//Checked with message
#define CHECKED_M(statement) if(!statement){std::cout << #statement << " failed: programmer timed out" << std::endl; return false;}

int argc;
char** argv;
int argi;
inline bool HasNextArg() { return argi < argc; }
inline char* GetNextArg() { return argv[argi++]; }
inline bool IsNextArgOperand() { return HasNextArg() && argv[argi][0] != '-'; }
inline bool IsNextArgOperation() { return HasNextArg() && argv[argi][0] == '-' && std::isupper(argv[argi][1]); }
inline bool IsNextArgOption() { return HasNextArg() && argv[argi][0] == '-' && !std::isupper(argv[argi][1]); }






int main(int _argc, char* _argv[])
{
	//Init defaults
	std::vector<const Option_t*> allowedGlobalOptions =
	{
		Options::Help,
		Options::Port,
		Options::Device,
		Options::Config,
		Options::SerialBaudrate,
		Options::UPDIBaudrate,
		Options::MultiMode,
	};

	std::vector<Option_inst*> globalOptions;
	std::vector<Operation*> operations;

	std::string configFilePath = "config.cfg";
	std::string portName;
	std::string deviceName;
	Device_t* device;
	uint32_t serialBaudrate = 0;
	uint32_t updiBaudrate = 0;
	bool multiMode = false;

	//Parse arguments...
#pragma region ParseArguments
	argc = _argc;
	argv = _argv;
	argi = 1;

	//Automatically add help option if no arguments were passed
	if (argc == 1)
	{
		globalOptions.push_back(new Option_inst(Options::Help));
	}

	while (HasNextArg())
	{
		//Start staging operation
		if (IsNextArgOperation())
		{
			char* arg = GetNextArg();
			auto it = StoOpCreationFunc.find(arg);
			if (it != StoOpCreationFunc.end())
			{
				Operation* newOperation = it->second();
				while (IsNextArgOperand())
					//Read operands...
					newOperation->operands.push_back(GetNextArg());
				operations.push_back(newOperation);
			}
			else
			{
				std::cout << "Undefined operation: '" << arg << "'" << std::endl;
				end(EResult::BAD_COMMAND);
			}
		}

		else if (IsNextArgOption())
		{
			char* arg = GetNextArg();
			auto it = StoOption.find(arg);
			if (it != StoOption.end())
			{
				const Option_t* newOptionType = it->second;
				Option_inst* newOption = new Option_inst(newOptionType);
				//Read operands...
				for (int i = 0; i < newOptionType->requiredOperands + newOptionType->optionalOperands; i++)
				{
					if (IsNextArgOperand())
						newOption->operands.push_back(GetNextArg());
					else if (i < newOptionType->requiredOperands)
					{
						std::cout << "Not enough operands for: '" << arg << "' - Required: " << std::dec << (int)newOptionType->requiredOperands << ", Specified: " << i << std::endl;
						end(EResult::BAD_COMMAND);
					}
				}
				if (!operations.empty())
				{
					if (std::find(operations.back()->allowedOptions.begin(), operations.back()->allowedOptions.end(), newOptionType) != operations.back()->allowedOptions.end())
						operations.back()->options.push_back(newOption);
					else
					{
						std::cout << "Invalid option for " << operations.back()->tag << " : '" << arg << "', use -? " << operations.back()->tag << " for a list of valid options" << std::endl;
						end(EResult::BAD_COMMAND);
					}
				}
				else
				{
					if (std::find(allowedGlobalOptions.begin(), allowedGlobalOptions.end(), newOptionType) != allowedGlobalOptions.end())
						globalOptions.push_back(newOption);
					else
					{
						std::cout << "Invalid global option: '" << arg << "', use -? for a list of valid global options" << std::endl;
						end(EResult::BAD_COMMAND);
					}
				}
			}
			else
			{
				std::cout << "Undefined option: '" << arg << "'" << std::endl;
				end(EResult::BAD_COMMAND);
			}
		}
		else
		{
			char* arg = GetNextArg();
			std::cout << "Unexpected operand: '" << arg << "'" << std::endl;
			end(EResult::BAD_COMMAND);
		}
	}

#pragma endregion

	//Process global options
	for (auto opt : globalOptions)
	{
#pragma region HelpMessage
		if (opt->type == Options::Help)
		{
			if (operations.empty())
			{
				//Global help message
				std::cout << std::endl << UsageStr << std::endl << std::endl << std::endl << "Global options:" << std::endl << std::endl;
				for (auto o : allowedGlobalOptions)
				{
					std::cout << o->tag << o->description << std::endl << std::endl;
				}
				std::cout << std::endl << std::endl << "Operations:" << std::endl << std::endl;
				for (auto ocfunc : StoOpCreationFunc)
				{
					Operation* o = ocfunc.second();
					std::cout << o->tag << o->description << std::endl << std::endl;
					delete o;
				}
				return 0; //End program
			}
			else
			{
				//Operation help message
				std::cout << std::endl << "Usage: " << operations.front()->tag << operations.front()->description << std::endl;
				if (operations.front()->extendedDescription) std::cout << operations.front()->extendedDescription << std::endl;
				std::cout << std::endl << std::endl;

				std::cout << "Valid options:" << std::endl << std::endl;
				for (auto o : operations.front()->allowedOptions)
				{
					std::cout << o->tag << o->description << std::endl << std::endl;
				}
				if (operations.front()->allowedOptions.empty())
					std::cout << "(none)" << std::endl;
				return 0; //End program
			}
		}

		else if (opt->type == Options::Port)
			portName = opt->operands[0];
		else if (opt->type == Options::Device)
			deviceName = opt->operands[0];
		else if (opt->type == Options::Config)
			configFilePath = opt->operands[0];
		else if (opt->type == Options::SerialBaudrate)
			serialBaudrate = try_stoi(opt->operands[0]);
		else if (opt->type == Options::UPDIBaudrate)
			updiBaudrate = try_stoi(opt->operands[0]);
		else if (opt->type == Options::MultiMode)
			multiMode = true;

#pragma endregion 
	}

	//Load config
	if (!LoadConfig(configFilePath)) end(EResult::CONFIG_ERROR);

	//Check port
	if (portName.empty())
	{
		std::cout << "No port specified! Use -? for more information" << std::endl;
		end(EResult::BAD_COMMAND);
	}

	//Load device
	if (deviceName.empty())
	{
		std::cout << "No device specified! Use -? for more information" << std::endl;
		end(EResult::BAD_COMMAND);
	}
	auto it = g_config.devices.find(deviceName);
	if (it == g_config.devices.end())
	{
		std::cout << "Unknown device '" << deviceName << "'. Make sure that device is defined in the used config file." << std::endl;
		end(EResult::BAD_COMMAND);
	}
	device = &it->second;


	//Load defaults if not specified already
	if (!serialBaudrate)
		serialBaudrate = g_config.defaultSerialBaudrate;
	if (!updiBaudrate)
		updiBaudrate = device->defaultBaudrate;

	//Open serial
	Serial s;
	if (s.Open(portName.c_str(), serialBaudrate, g_config.programmerTimeout))
	{
		std::cout << portName << " open " << std::endl;

		FastUPDI updi;
		updi.normalBaudrate = updiBaudrate;

		do {

			if (!updi.start(&s, device))
			{
				std::cout << "Failed to start: programmer timed out" << std::endl;
				end(EResult::NO_START);
			}

			if (!updi.set_baud(updi.normalBaudrate)) end(EResult::UPDI_ERROR);

			//Execute staged operations
			for (Operation* op : operations)
			{
				EResult result = op->Execute(&updi);
				if (result != EResult::SUCCESS)
					end(result);

			}

			if (!updi.stop())
			{
				std::cout << "Failed to stop: programmer timed out. UPDI link might still be running and consuming extra power until the device restarts!" << std::endl;
				end(EResult::NO_STOP);
			}

			if (multiMode)
			{
				std::cout << std::endl << "Multi-mode is active. Press any key to repeat last set of operations. Press ESC to exit." << std::endl << std::endl;
				char c = _getch();
				
				if (c == 27)
					multiMode = false;
			}

		} while (multiMode);
	}
	else
	{
		std::cout << "Could not open port '" << portName << "'" << std::endl;
		end(EResult::PORT_ERROR);
	}

	s.Close();
	end(EResult::SUCCESS);
}
