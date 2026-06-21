#ifndef CHUNI_EDIT
#define CHUNI_EDIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int init_term();
char *read_str();
void rec_term();

#endif
