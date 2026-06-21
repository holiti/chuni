#ifndef PARSER
#define PARSER

#include "constant.h"
#include "ppchars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **parse_str(const char *str);
void free_ppchar(char **arg);

#endif
