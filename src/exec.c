#include "exec.h"
#include "execunit.h"

extern void term_init();
extern void term_rec();

enum
{
    STAT_LEN = 3,
    JOB_BUFFER_LEN = 1024,
    NUM_BUFFER_LEN = 32,
    BG_PROCESS_LEN = 1024,
};

#define ERR(msg) fprintf(stderr, TTL ": %s\n", msg);

/* ----------------------------------------------------------------*/
/* SIGNAL TOOLS */
/* ----------------------------------------------------------------*/
struct sigaction sold;
struct sigaction snew;

static void sigset_def(void (*func)(int))
{
    struct sigaction cur = {.sa_handler = func, .sa_flags = 0};
    sigaction(SIGINT, &cur, NULL);
    sigaction(SIGQUIT, &cur, NULL);
    sigaction(SIGTTOU, &cur, NULL);
    sigaction(SIGTTIN, &cur, NULL);
}

/* ---------------------------------------------------------------*/
/* STREAM REDIRECTION */
/* ---------------------------------------------------------------*/
static inline void replace_fd(int fd, int new_fd)
{
    dup2(fd, new_fd);
    close(fd);
}

/* ---------------------------------------------------------------*/
/* EXECUTE COMAND */
/* ---------------------------------------------------------------*/
static int exec(struct exec_unit *ut)
{
    int fd;
    int pid;
    int status = 0;
    struct cmd *cur = ut->first;
    int sv_inp = dup(0), sv_out = dup(1);
    int pip[2] = {-1, -1};

    int cmd_cnt = 0;
    int chuni_cmd_id;

    int pgid = -1;

    if (ut->flags & UT_BACKGROUND)
    {
        add_bg();
    }
    else
    {
        add_fg();
    }
    while (cur != NULL)
    {
        if (cur == ut->first)
        {
            if (ut->input != NULL)
            {
                fd = open(ut->input, O_RDONLY);
                if (fd == -1)
                {
                    perror(TTL);
                    status = 1;
                    return status;
                }
                replace_fd(fd, 0);
            }
            else
            {
                dup2(sv_inp, 0);
            }
        }

        if (cur == ut->last)
        {
            if (ut->output != NULL)
            {
                fd = open(ut->output,
                          O_WRONLY | O_CREAT |
                              (ut->flags & UT_TRUNCATE ? O_TRUNC : O_APPEND),
                          0666);
                if (fd == -1)
                {
                    perror(TTL);
                    status = 1;
                    return status;
                }
                replace_fd(fd, 1);
            }
            else
            {
                dup2(sv_out, 1);
            }
        }

        if (pip[0] != -1)
        {
            replace_fd(pip[0], 0);
        }
        if (cur != ut->last)
        {
            pipe(pip);
            replace_fd(pip[1], 1);
        }

        chuni_cmd_id = is_chuni_cmd(cur->arg[0]);
        if (chuni_cmd_id >= 0)
        {

            status = chuni_cmd[chuni_cmd_id](cur->arg);
            if (!status)
                ++cmd_cnt;
        }
        else
        {
            pid = fork();
            if (pid == 0)
            {
                if (pip[0] != -1)
                    close(pip[0]);

                if (pgid == -1)
                    pgid = setpgid(0, 0);
                else
                    pgid = setpgid(0, pgid);

                sigset_def(SIG_DFL);

                execvp(cur->arg[0], cur->arg);
                perror(TTL);
                fflush(stderr);
                _exit(1);
            }
            else if (pid == -1)
            {
                perror(TTL);
            }
            else
            {
                if (pgid == -1)
                    pgid = pid;
                setpgid(pid, pgid);
                if (ut->flags & UT_BACKGROUND)
                {
                    add_bg_pid(pid);
                }
                else
                {
                    tty_make_fg(pgid);
                    add_fg_pid(pid);
                }

                ++cmd_cnt;
            }
        }

        cur = cur->next;
    }

    replace_fd(sv_inp, 0);
    replace_fd(sv_out, 1);

    if (cmd_cnt == 0)
    {
        status = 1;
        return status;
    }

    if (!(ut->flags & UT_BACKGROUND))
    {
        wait_fg();
        tty_make_fg(getpgid(0));
    }

    return status;
}
/* ------------------------------------------------------------*/
/* PARSE ARRAY OF WORDS */
/* ------------------------------------------------------------*/

static int separator_parse(char **arg, int *x, int *last_good,
                           struct exec_unit *ut)
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
    case PSEP_PIPE:
        arg[(*last_good)++] = NULL;
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
                result = separator_parse(arg, &br, &last_good, &ut);
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

        unit_add_cmd(&ut, arg + bl);
        for (; bl < last_good; ++bl)
            if (arg[bl] == NULL)
                unit_add_cmd(&ut, arg + bl + 1);

        for (; last_good < br; ++last_good)
            arg[last_good] = NULL;

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

/* ---------------------------------------------------------------------- */
/* INIT AND FREE PIDS */
/* -----------------------------------------------------------------------*/

void init_exec()
{
    sigset_def(SIG_IGN);
    signal_init();
}

void free_exec() {}
