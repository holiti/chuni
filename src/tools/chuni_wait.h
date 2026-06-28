#ifndef CHUNI_WAIT
#define CHUNI_WAIT

#include "../constant.h"
#include "tools.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void signal_init();
void add_bg();
void add_bg_pid(size_t pid);

void add_fg();
void add_fg_pid(size_t pid);
void wait_fg(int *status);

#endif
