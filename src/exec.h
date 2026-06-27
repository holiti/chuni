#ifndef CHUNI_EXEC
#define CHUNI_EXEC

#include "chunicmd.h"
#include "constant.h"
#include "dynarr.h"
#include "execunit.h"
#include "tools/chuni_tty.h"
#include "tools/chuni_wait.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void init_exec();
void execute(char **arg, int *status);
void free_exec();

#endif
