#pragma once

#include "FormatBase.h"

//Uncomment to use linear addressing instead of segment addressing
#define USE_LINEAR_ADDRESS

enum class EHexRecordType : uint8_t
{
	DATA = 0x00,
	END_OF_FILE = 0x01,
	EXTENDED_SEGMENT_ADDRESS = 0x02,
	START_SEGMENT_ADDRESS = 0x03,
	EXTENDED_LINEAR_ADDRESS = 0x04,
	START_LINEAR_ADDRESS = 0x05
};

class FIntelHex : public Format
{
public:
	//Read file and create an image from it
	virtual bool Read(FILE* file, image_t* outImage) const override;

	//Create a file from an image
	virtual bool Write(FILE* file, image_t* image) const override;
};
