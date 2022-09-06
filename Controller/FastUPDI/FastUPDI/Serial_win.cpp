#include "Serial_win.h"


bool Serial::Open(const char* name, uint64_t normalBaudrate, uint32_t timeout)
{
    port = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb;
    GetCommState(port, &dcb);
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.BaudRate = normalBaudrate;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    SetCommState(port, &dcb);

    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = timeout;
    timeouts.ReadTotalTimeoutConstant = timeout;
    timeouts.ReadTotalTimeoutMultiplier = timeout;

    SetCommTimeouts(port, &timeouts);

    return true;
}

bool Serial::Read(uint8_t* buffer, uint32_t size)
{
    DWORD bytesRead;
    bool result = ReadFile(port, buffer, size, &bytesRead, NULL);
    return result && bytesRead;
}

bool Serial::Write(uint8_t* buffer, uint32_t size)
{
    return  WriteFile(port, buffer, size, NULL, NULL);
}

void Serial::Close()
{
    CloseHandle(port);
}

void Serial::PulseDTR()
{
    EscapeCommFunction(port, SETDTR);
    EscapeCommFunction(port, CLRDTR);
}
