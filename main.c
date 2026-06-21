#include "src/constant.h"
#include "src/edit.h"
#include "src/exec.h"
#include "src/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ERR(msg) fprintf(stderr, TTL ": %s\n", msg)
extern void chuni_exit(int code);

void chuni_exit(int code)
{
    rec_term();
    free_exec();
    exit(code);
}

void chuni_start()
{
    int r = init_term();
    init_exec();
    if (r)
    {
        ERR("standart input isn't terminal");
        exit(1);
    }
}

static void pwd(char *path, int len) { getcwd(path, len - 1); }

int main(int argc, char **argv)
{
    char *str, path[PATH_MAX];
    char **arg;
    int status = 0;

    chuni_start();

    while (1)
    {
        pwd(path, PATH_MAX);
        printf("[Chuni] < %s > | <%d> ", path, status);
        fflush(stdout);

        str = read_str();

        if (str == NULL || str[0] == 0)
        {
            status = 0;
            free(str);
            continue;
        }

        arg = parse_str(str);

        if (arg == NULL)
        {
            status = 1;
            goto end_free;
        }

        execute(arg, &status);
    end_free:
        free_ppchar(arg);
        free(str);
    }

    return 0;
}
