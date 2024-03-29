#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <cglm/struct.h>

#include "config.h"
#include "operation.h"

int parseHex(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	else if ('a' <= c && 'c' <= 'f')
		return c - 'a' + 10;
	else
		return -1;
}

bool parseColor(char* str, vec4s* out)
{
	vec4s ret;
	ret.w = 1;

	if (strlen(str) != 6)
		return false;

	for (int i = 0; i < 3; i++)
	{
		int num1 = parseHex(str[2 * i]);
		int num2 = parseHex(str[2 * i + 1]);

		if (num1 == -1 || num2 == -1)
			return false;

		int num = num1 * 16 + num2;
		ret.raw[i] = (float) num / 255;
	}

	*out = ret;
	return true;
}

char* execCommand(char* cmd, struct Preferences* preferences, struct DrawHistory* dh)
{
	for (int i = 0; i < strlen(cmd); i++)
		cmd[i] = tolower(cmd[i]);

	char* token = strsep(&cmd, " ");
	if (strcmp(token, "set") == 0)
	{
		char* token = strsep(&cmd, " ");
		
		if (strcmp(token, "color") == 0)
		{
			char* token = strsep(&cmd, " ");
			if (token == NULL)
				return "Usage: 'set' 'color' [color]";
			else
			{
				if (!parseColor(token, &preferences->color))
					return "Usage: 'set' 'color' [color]";
			}
		}
		else if (strcmp(token, "size") == 0)
		{
			char* token = strsep(&cmd, " ");
			if (token == NULL)
				return "Usage: 'set' 'size' [size]";
			else
			{
				float size = atof(token);

				if (size == 0.0)
					return "Usage: 'set' 'size' [size: 0..1]";

				preferences->radius = size;
			}
		}
		else
			return "Usage: 'set' color | size";

		return NULL;
	}
	else if (strcmp(token, "w") == 0)
	{
		char* filename = strsep(&cmd, " ");

		FILE* fp = fopen(filename, "wb");
		if (fp == NULL)
			return "Failed to read file";

		dhSerialise(*dh, fp);
		fclose(fp);

		return NULL;
	}
	else if (strcmp(token, "r") == 0)
	{
		char* filename = strsep(&cmd, " ");

		FILE* fp = fopen(filename, "rb");
		if (fp == NULL)
			return "Failed to read file";

		int ret = dhLoadDeserialise(fp, dh);
		if (ret == -1)
			return "Unrecognised file format";
		else if (ret == -2)
			return "Incompatible screen size";

		fclose(fp);

		return NULL;
	}

	return "Invalid command";
}
