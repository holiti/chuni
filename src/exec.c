#include "exec.h"
#include <stdlib.h>

enum
{
    STAT_LEN = 3,
};

#define TTL "Chuni"
#define BG_END_MSG(id, pid, status)                                            \
    printf(TTL ": Job %ld:%ld, has %s\n", id, pid, status)

DYN_STRUCT(size_t, int)
dyn_int *pids;

char *stat[STAT_LEN] = {"exited", "signaled", "stoped"};

static int get_stat_id(const int status)
{
    int id = 0;
    if (WIFEXITED(status))
        id = 0;
    else if (WIFSIGNALED(status))
    {
        id = 1;
    }
    else if (WIFSTOPPED(status))
    {
        id = 2;
    }
    return id;
}
static void cd(char *const *arg, int *status)
{
    int r;
    int argl = 0;
    char *user_path;

    for (; arg[argl] != NULL; ++argl)
    {
    }

    if (argl > 2)
    {
        *status = 1;
        fprintf(stderr, TTL " : too more argument\n");
        return;
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
        *status = 1;
    }
    else
    {
        *status = 0;
    }
}

static int exec(char *const *arg, int *status)
{
    if (arg == NULL || arg[0] == NULL)
    {
        fprintf(stderr, TTL ": which program must be executed?\n");
        return -1;
    }

    int pid;
    if (arg == NULL)
        return -1;

    // cd command
    if (arg[0][0] == 'c' && arg[0][1] == 'd' && arg[0][2] == 0)
    {
        cd(arg, status);
        return 1;
    }

    pid = fork();
    if (pid == -1)
    {
        perror(TTL);
        *status = 1;
        return -1;
    }
    if (pid == 0)
    {
        execvp(arg[0], arg);
        perror(TTL);
        fflush(stderr);
        _exit(1);
    }
    return pid;
}

void init_exec() { pids = dyn_int_init(); }

int exec_fg(char *const *arg)
{
    int status;
    int pid = exec(arg, &status);

    if (pid > 1)
        waitpid(pid, &status, 0);

    return status;
}

int exec_bg(char *const *arg)
{
    int status = -1;
    int pid = exec(arg, &status);

    if (pid > 1)
    {
        dyn_int_push(pids, pid);
        status = 0;
    }
    else if (pid == 1)
    {
        BG_END_MSG(pids->len + 1, (long int)-1, stat[0]);
        fflush(stdout);
    }

    return status;
}

void wait_bg()
{
    int status;
    int r;
    for (int i = 0; i < pids->len; ++i)
        if (pids->arr[i] != -1)
        {
            r = waitpid(pids->arr[i], &status, WNOHANG);
            if (r > 0)
            {
                BG_END_MSG(i + 1l, pids->arr[i], stat[get_stat_id(status)]);
                pids->arr[i] = -1;
            }
        }
    fflush(stdout);

    while (pids->len > 0 && pids->arr[pids->len - 1] == -1)
    {
        dyn_int_pop(pids);
    }
}

void free_exec()
{
    free(pids->arr);
    free(pids);
}
