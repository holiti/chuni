#include "exec.h"
#include "constant.h"
#include <stdlib.h>

enum
{
    STAT_LEN = 3,
    UT_BACKGROUND = 1,
    UT_TRUNCATE = 2,
};

#define BG_END_MSG(id, pid, status)                                            \
    printf(TTL ": Job %ld:%ld, has %s\n", id, pid, status)
#define ERR(msg) fprintf(stderr, TTL ": %s\n", msg);

DYN_STRUCT(size_t, int)
static dyn_int *pids;

/* EXECUTION UNIT (COMANDA) */
struct exec_unit
{
    char **arg;
    char *input;
    char *output;
    int flags;
    /*
     * 1 - background execut
     * 2 - truncate output file
     */
};

static void init_unit(struct exec_unit *ut)
{
    ut->flags = 0;
    ut->input = ut->output = NULL;
    ut->arg = NULL;
}

static void clear_unit(struct exec_unit *ut)
{
    if (ut->input != NULL)
        free(ut->input);
    if (ut->output != NULL)
        free(ut->output);
    init_unit(ut);
}

/* STATUS TOOLS */
static char *stat[STAT_LEN] = {"exited", "signaled", "stoped"};
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

/* STREAM REDIRECTION */
static int replace_fd(int new_fd, int fd)
{
    int sv_fd = dup(new_fd);
    dup2(fd, new_fd);
    close(fd);
    return sv_fd;
}

/* EXECUTE COMAND */
static int exec(struct exec_unit *ut)
{
    int sv_input = -1, sv_output = -1;
    int fd;
    int pid;
    int status = 0;
    int chuni_cmd_i;

    if (ut->arg == NULL || ut->arg[0] == NULL)
    {
        return 1;
    }

    if (ut->input != NULL)
    {
        fd = open(ut->input, O_RDONLY);
        if (fd == -1)
        {
            perror(TTL);
            return errno;
        }
        sv_input = replace_fd(0, fd);
    }

    if (ut->output != NULL)
    {
        fd = open(ut->output,
                  O_WRONLY | O_CREAT |
                      (ut->flags && UT_TRUNCATE ? O_TRUNC : O_APPEND),
                  0666);
        if (fd == -1)
        {
            perror(TTL);
            return errno;
        }
        sv_output = replace_fd(1, fd);
    }

    chuni_cmd_i = is_chuni_cmd(ut->arg[0]);
    if (chuni_cmd_i >= 0)
    {
        status = chuni_cmd[chuni_cmd_i](ut->arg);
        if (ut->flags & UT_BACKGROUND)
        {
            BG_END_MSG(pids->len + 1, -1l, stat[0]);
        }
    }
    else
    {

        pid = fork();
        if (pid == -1)
        {
            perror(TTL);
            return errno;
        }
        if (pid == 0)
        {
            execvp(ut->arg[0], ut->arg);
            ERR("error during execution.");
            fflush(stderr);
            _exit(1);
        }

        if (ut->flags & UT_BACKGROUND)
        {
            dyn_int_push(pids, pid);
            status = 0;
        }
        else
        {
            waitpid(pid, &status, 0);
        }
    }

    if (sv_input != -1)
    {
        dup2(sv_input, 0);
    }

    if (sv_output != -1)
    {
        dup2(sv_output, 1);
    }

    return status;
}

/* PARSE ARRAY OF WORDS */

static int separator_parse(char **arg, int *x, struct exec_unit *ut)
{
    switch ((size_t)arg[(*x)])
    {
    case PSEP_OUTT:
    case PSEP_OUT:
        if (ut->output != NULL)
        {
            ERR("multimply definition of output stream.");
            return 1;
        }
        if (arg[(*x) + 1] == NULL)
        {
            ERR("file not specified.");
            return 1;
        }
        ut->output = arg[(*x) + 1];

        if ((size_t)arg[(*x)] == PSEP_OUTT)
        {
            ut->flags |= UT_TRUNCATE;
        }

        ++(*x);
        break;
    case PSEP_IN:
        if (ut->input != NULL)
        {
            ERR("multiply definition of input stream.");
            return 1;
        }
        if (arg[(*x) + 1] == NULL)
        {
            ERR("file not specified.");
            return 1;
        }

        ut->input = arg[(*x) + 1];
        ++(*x);
        break;
    case PSEP_BACK:
        if (arg[(*x) + 1] != NULL)
        {
            ERR("background flag shall is last separator");
            return 1;
        }
        ut->flags |= UT_BACKGROUND;
        break;
    }
    return 0;
}
void execute(char **arg, int *status)
{
    int bl = 0, br = 0;
    int last_good;
    int result;
    struct exec_unit ut;
    init_unit(&ut);

    if (arg == NULL)
    {
        *status = 0;
        return;
    }

    *status = 0;
    while (arg[bl] != NULL)
    {
        last_good = bl;
        for (br = bl; arg[br] != NULL; ++br)
        {
            if ((size_t)arg[br] < PSEP_COUNT)
            {
                result = separator_parse(arg, &br, &ut);
                if (result)
                {
                    *status = result;
                    return;
                }
            }
            else
            {
                arg[last_good++] = arg[br];
            }
        }

        for (; last_good < br; ++last_good)
            arg[last_good] = NULL;

        ut.arg = arg + bl;

        *status = exec(&ut);

        clear_unit(&ut);

        if (!status)
        {
            return;
        }

        bl = br;
    }
    return;
}

/* BACKGROUND EXECUTION SUPPORT */
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

/* INIT AND FREE PIDS */
void init_exec() { pids = dyn_int_init(); }

void free_exec()
{
    free(pids->arr);
    free(pids);
}
