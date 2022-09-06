//Windows implementation of Serial

#pragma once

#include <stdint.h>
#include <Windows.h>

class Serial
{
	HANDLE port;
public:
	bool Open(const char* name, uint64_t normalBaudrate, uint32_t timeout);

	bool Read(uint8_t* buffer, uint32_t size);

	bool Write(uint8_t* buffer, uint32_t size);

	void Close();

	void PulseDTR();
};