#ifndef CHUNI_CMD
#define CHUNI_CMD

#include "constant.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
    CHUNI_CMD_COUNT = 2,
};

int cd(char *const *arg);
int cexit(char *const *arg);
int is_chuni_cmd(const char *name);
extern int (*chuni_cmd[CHUNI_CMD_COUNT])(char *const *);

#endif
