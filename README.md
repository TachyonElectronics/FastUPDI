# FastUPDI UPDI Programmer & interface

**FastUPDI is an implementation of the UPDI protocol which supports speeds of up to 1Mb/s and mass complex operations.**

FastUPDI consists of a custom Controller command line program for Windows and a programmer firmware designed to be used on Arduino mega2560 boards.

Supports only NVMv2 UPDI devices (such as AVR-Dx series)


## MAKING A CUSTOM PROGRAMMER
1) Install the firmware into a supported host device of choice (such as Arduino MEGA) to make it into a programmer. This can be done using Atmel Studio or avrdude command `avrdude -C avrdude.conf -p m2560 -P COM5 -c wiring -U flash:w:FastUPDI.hex:i -D`
2) Connect a 4.7k resistor between the UART Tx and Rx pins (on Arduino MEGA - Tx/Rx 1)
3) Connect the Rx pin to the target's UPDI line
4) Make sure the target is powered with an appropriate voltage (such as 3.3v from Arduino) and connected to a common ground with the programmer

![image](https://user-images.githubusercontent.com/4728385/188677898-4e1c37c0-388d-48d6-8561-e2ad93bc2d23.png)


## USING THE SOFTWARE
1) Edit the config file if needed to add support to your target device.
2) Use the included Controller program to interface with the programmer and device:

Use the controller in command line as:
	
  `fastupdi [global options] [operation1 [operands] [options]] ... [operation_n [operands] [options]]`

- Use `fastupdi -?` to view all supported options and operations.
- Use `fastupdi -? [-Operation]` to view all supported options for a given operation.

**NOTE: All arguments (except file names) are CASE SENSITIVE**

### Basic operations
- Use `-R <source> [output] [options]` to **read** from target and output to file or console
- Use `-W <destination> <data> [options]` to **write** to target 
- Use `-V <source> <template> [options]` to **verify** the target's data against a template
- Use `-E` for chip **erase**
#### Operands:
- `<source>` and `<destination>` can be either memory names defined in *config* or direct hex addresses in format `0xHHHHHHHH`
- `<data>` and `<template>` can be either filenames or direct hex data bytes in format `0xHH`
- `[output]` can be an output filename or can be omitted in order to output directly to console

### [For a detailed list of all options, look here](https://github.com/TachyonElectronics/FastUPDI/wiki/Usage)

### Operation chaining
Up to 255 operations can be chained together and executed in first-to-last order as a single command. This can be used, for example, to quickly program several memories using a single command line command.

### Multi-mode
Use the `-m` global option to enable Multi-mode. The specified operation chain will be repeated after each key press until ESC key is pressed. The UPDI link is closed between each run, which allows the target chip to be replaced. This allows fast programming of large batches of chips/devices
