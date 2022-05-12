#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <cglm/struct.h>

#include "config.h"

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

char* execCommand(char* cmd, struct Preferences* preferences)
{
	for (int i = 0; i < strlen(cmd); i++)
		cmd[i] = tolower(cmd[i]);

	char* token = strsep(&cmd, " ");
	if (strcmp(token, "set") == 0)
	{
		char* token = strsep(&cmd, " ");
		
		if (strcmp(token, "brushcolor") == 0)
		{
			char* token = strsep(&cmd, " ");
			if (token == NULL)
				return "Usage: 'set' 'brushColor' [color]";
			else
			{
				if (!parseColor(token, &preferences->brush.color))
					return "Usage: 'set' 'brushColor' [color]";
			}
		}
		else
			return "Usage: 'set' brushColor | brushSize | bgColor";

		return NULL;
	}

	return "Invalid command";
}
