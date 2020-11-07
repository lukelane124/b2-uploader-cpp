#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../c_defs.h"
#include "Encoding.h"

typedef char hexChar[2];

const char* dontNeedEncode = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-_~!+,*:@";
void getHex(hexChar* hxP, uint8_t c)
{
	hexChar ret;// = *hxP;
	uint8_t nibble = c & 0x0f;
	ret[0] = dontNeedEncode[nibble];
	nibble = (c >> 4) & 0x0f;
	ret[1] = dontNeedEncode[nibble];
	*hxP[0] = ret[0];
	*hxP[1] = ret[1];
}

char* urlEncode(uint8_t* input, size_t inputLen)
{
	char* ret = NULL;
	size_t index = 0;
	char* ptr;
	char tri[3];
	size_t outlen = 0;
	hexChar hexHold;
	static uint8_t chars[256] = {0};
	static int init = 0;

	if (init == 0)
	{
		init = 1;
		LOG_DEBUG("initializing the decode array\n");
		for (index = 0; index < strlen(dontNeedEncode); index++)
		{
			chars[dontNeedEncode[index]] = 1;
		}
	}
	LOG_DEBUG("getting count.\n");
	for (index = 0; index < inputLen; index++)
	{
		outlen++;
		if ( chars[input[index]] == 0 )
		{
			outlen+=2;
		}
	}

	outlen += 2;
	LOG_DEBUG("total encoded length count: %zu\n", outlen);
	ret = malloc(outlen);
	if (ret != NULL)
	{
		LOG_DEBUG("Got a valid malloc string back.\n");
		ptr = ret;
		index = 0;
		while (index < inputLen)
		{
			LOG_DEBUG("Top of while.\n");
			if ( chars[input[index]] == 0 )
			{
				LOG_DEBUG("Found an invalid char, encoding now.\nCharacter: 0x%02x\n", input[index]);
				*ptr++ = '%';
				snprintf(ptr, 3, "%02x", input[index]);
				ptr += 2;
			}
			else
			{
				*ptr++ = input[index];
			}
			index++;
		}
		*ptr++ = 0;
	}

	return ret;
}