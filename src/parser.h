#ifndef PARSER
#define PARSER

#include "constant.h"
#include "ppchars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **parse_var(const char *const str);
char **parse_str(const char *const str);
void freepp(char **arg);

#endif
