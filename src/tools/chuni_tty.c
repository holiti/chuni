#include "chuni_tty.h"

static struct termios old;

void tty_init()
{
    static char tty_f = 0;
    struct termios new;
    if (tty_f == 0)
    {
        tty_f = 1;
        tcgetattr(0, &old);
    }
    new = old;

    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VTIME] = 0;
    new.c_cc[VMIN] = 1;

    tcsetattr(0, TCSANOW, &new);
}

void tty_recover() { tcsetattr(0, TCSANOW, &old); }

void tty_make_fg(int pgid) { tcsetpgrp(0, pgid); }
