#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_MASK Mod4Mask | Mod1Mask
char rc_file[] = "/.kbswtchrc";

void defaultKeys();
int readFile();
void nullKeys();
int parseLine(keybind* key, const char *line);

extern Display *display;

int readFile()
{
	FILE *stream = NULL;
	char line[1024];
	char full_filename[1024];
	char* home;
	int ret = 0;

	home = getenv("HOME");
	strcpy(full_filename,home);
	strcat(full_filename,rc_file);

	if((stream = fopen (full_filename, "r")) == NULL) {
		fprintf (stderr, "%s not found or reading not allowed.\n", rc_file);
		return 1;
	}

	while (fgets (line, sizeof(line), stream) && !ret) {
		if (line[0] == '\n') continue;
		if(line[1] != ':') {
			fprintf (stderr, "Wrong line: %s\n", line);
			ret = 1;
		} else {
			switch (line[0]) {
			case '1': ret = parseLine(&b_keys[0], &line[2]); break;
			case '2': ret = parseLine(&b_keys[1], &line[2]); break;
			case '3': ret = parseLine(&b_keys[2], &line[2]); break;
			case '4': ret = parseLine(&b_keys[3], &line[2]); break;
			case 's': ret = parseLine(&b_keys[4], &line[2]); break;
			case 'q': ret = parseLine(&b_keys[5], &line[2]); break;
			default: ret = 1;
			}
		}
		if(ret)
			fprintf(stderr, "Wrong line: %s\n", line);
	}

	fclose(stream);
	return ret;
}

void getKeys()
{
	nullKeys();
	if(readFile()) {
		defaultKeys();
	}
}

void defaultKeys()
{
	int i;

	for(i = 0; i < 4; i++) {
		b_keys[i].modifier = DEFAULT_MASK;
		b_keys[i].keycode = i + 10;
	}

	b_keys[4].modifier = DEFAULT_MASK;
	b_keys[4].keycode = 39;
	b_keys[5].modifier = DEFAULT_MASK;
	b_keys[5].keycode = 24;
}

void nullKeys()
{
	int i;
	for (i = 0; i < 6; i++)
		b_keys[i].keycode = 0;
}

int parseLine(keybind *key, const char *line)
{
	const char *begin, *plus, *end;
	size_t len;
	char symbol[1024];
	KeySym keysym;
	int ret = 0;
	bool variable = false;

	begin = line;
	plus = line;
	while( (*plus != '\n') && (!ret)) {
		for(;*begin ==' ' || *begin == '\t'; begin++);
		plus = strchr(begin, '+');
		if(!plus)
			for(plus = begin; *plus != '\n'; plus++);
		for(end = plus - 1; *end == ' ' || *end == '\t'; end--);

		len = end - begin + 1;
		if(strncasecmp("control", begin, len)==0) {
			key->modifier |= ControlMask;
		} else if(strncasecmp("shift", begin, len)==0) {
			key->modifier |= ShiftMask;
		} else if(strncasecmp("alt", begin, len)==0) {
			key->modifier |= Mod1Mask;
		} else if(strncasecmp("mod1", begin, len)==0) {
			key->modifier |= Mod1Mask;
		} else if(strncasecmp("mod2", begin, len)==0) {
			key->modifier |= Mod2Mask;
		} else if(strncasecmp("mod3", begin, len)==0) {
			key->modifier |= Mod3Mask;
		} else if(strncasecmp("mod4", begin, len)==0) {
			key->modifier |= Mod4Mask;
		} else if(strncasecmp("mod5", begin, len)==0) {
			key->modifier |= Mod5Mask;
		} else {
			if(variable) {
				ret = 1;
			} else {
				variable = true;
				strncpy(symbol,begin,len);
				symbol[len] = '\0';
				keysym = XStringToKeysym(symbol);
				if(keysym == NoSymbol) {
					ret = 1;
				} else {
					key->keycode =
						XKeysymToKeycode(display,keysym);
				}
			}
		}
		begin = plus + 1;
	}

	return ret;
}
