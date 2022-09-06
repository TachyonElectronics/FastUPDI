#pragma once

#include "../memory.h"
#include <stdio.h>

class Format
{
public:
	//Read file and create an image from it
	virtual bool Read(FILE* file, image_t* outImage) const;

	//Create a file from an image
	virtual bool Write(FILE* file, image_t* image) const;
};