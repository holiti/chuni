#ifndef EXEC
#define EXEC

#include "dynarr.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void init_exec();
int exec_fg(char *const *arg);
int exec_bg(char *const *arg);
void wait_bg();
void free_exec();

#endif
