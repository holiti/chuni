#ifndef CHUNI_EDIT
#define CHUNI_EDIT

#include "parser.h"
#include "ppchars.h"
#include "tools/chuni_tty.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int edit_init();
char *read_str();
void edit_free();

#endif
