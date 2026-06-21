#ifndef CHUNI_EXECUNIT
#define CHUNI_EXECUNIT

#include <stdlib.h>

enum
{
    UT_BACKGROUND = 1,
    UT_TRUNCATE = 2,
};

struct cmd
{
    char **arg;
    int pid;
    struct cmd *next;
};

struct exec_unit
{
    struct cmd *first, *last;
    char *input;
    char *output;
    int flags;
    /*
     * 1 - background execut
     * 2 - truncate output file
     */
};

void init_unit(struct exec_unit *ut);
void clear_unit(struct exec_unit *ut);
void unit_add_cmd(struct exec_unit *ut, char **arg);

#endif
