#include "src/constant.h"
#include "src/edit.h"
#include "src/exec.h"
#include "src/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ERR(msg) fprintf(stderr, TTL ": %s\n", msg)

int main_status = 0;

void chuni_exit(int code)
{
    edit_free();
    free_exec();
    exit(code);
}

void chuni_start()
{
    int r = edit_init();
    init_exec();
    if (r)
    {
        ERR("standart input isn't terminal");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    char *str;
    char **arg;

    chuni_start();

    while (1)
    {
        str = read_str();

        if (str == NULL || str[0] == 0)
        {
            main_status = 0;
            free(str);
            continue;
        }

        arg = parse_str(str);

        if (arg == NULL)
        {
            main_status = 1;
            goto end_free;
        }

        main_status = 0;
        execute(arg, &main_status);
    end_free:
        freepp(arg);
        free(str);
    }

    return 0;
}
