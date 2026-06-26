#ifndef CHUNI_TTY
#define CHUNI_TTY

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void tty_init();
void tty_recover();

void tty_make_fg(int pgid);

#endif
