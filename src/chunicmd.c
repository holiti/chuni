#include "chunicmd.h"

int (*chuni_cmd[CHUNI_CMD_COUNT])(char *const *) = {cd, cexit};
static char *chuni_cmd_name[CHUNI_CMD_COUNT] = {"cd", "exit"};

/* IS_CHUNI_CMD */

int is_chuni_cmd(const char *str)
{
    for (int i = 0; i < CHUNI_CMD_COUNT; ++i)
        if (!strcmp(str, chuni_cmd_name[i]))
        {
            return i;
        }

    return -1;
}

/* CHUNI COMAND */

int cd(char *const *arg)
{
    int r;
    int argl = 0;
    char *user_path;

    for (; arg[argl] != NULL; ++argl)
    {
    }

    if (argl > 2)
    {
        fprintf(stderr, TTL " : too more argument\n");
        return 1;
    }

    if (argl == 2)
    {
        r = chdir(arg[1]);
    }
    else
    {
        user_path = getenv("HOME");
        r = chdir(user_path);
    }

    if (r == -1)
    {
        perror(TTL);
        return 1;
    }

    return 0;
}

int cexit(char *const *arg)
{
    exit(0);
    return 0;
}
