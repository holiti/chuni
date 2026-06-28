#include "exec.h"
#include "constant.h"

extern void term_init();
extern void term_rec();

enum
{
    STAT_LEN = 3,
    JOB_BUFFER_LEN = 1024,
    NUM_BUFFER_LEN = 32,
    BG_PROCESS_LEN = 1024,

    EX_BACKGROUND = 1,
    EX_TRUNCATE = 2,

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
static int exec(char **arg, int cmd_cnt, char *input, char *output, int flags)
{
    int fd;
    int pid;
    int status = 0;

    int pip[2] = {-1, -1};
    int sv_inp = dup(0), sv_out = dup(1);

    int chuni_cmd_id;
    int executed = 0;

    int pgid = -1;

    if (arg[0] == NULL)
        return 0;

    if (flags & EX_BACKGROUND)
    {
        add_bg();
    }
    else
    {
        add_fg();
    }

    for (int i = 0, j = 0, nxt_i; j < cmd_cnt; ++j)
    {
        nxt_i = i;
        while (arg[nxt_i] != NULL)
            ++nxt_i;

        if (j == 0)
        {
            if (input != NULL)
            {
                fd = open(input, O_RDONLY);
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

        if (j == cmd_cnt - 1)
        {
            if (output != NULL)
            {
                fd = open(output,
                          O_WRONLY | O_CREAT |
                              (flags & UT_TRUNCATE ? O_TRUNC : O_APPEND),
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
        if (j != cmd_cnt - 1)
        {
            pipe(pip);
            replace_fd(pip[1], 1);
        }

        if ((size_t)arg[i] != PSEP_LBR)
            chuni_cmd_id = is_chuni_cmd(arg[i]);
        else
            chuni_cmd_id = -1;

        if (chuni_cmd_id >= 0)
        {
            status = chuni_cmd[chuni_cmd_id](arg + i);
            if (!status)
                ++executed;
        }
        else
        {
            pid = fork();
            if (pid == 0)
            {
                if (pip[0] != -1)
                    close(pip[0]);

                if (pgid == -1)
                {
                    setpgid(0, 0);
                    tty_make_fg(getpid());
                }
                else
                {
                    setpgid(0, pgid);
                    tty_make_fg(pgid);
                }

                if ((size_t)arg[i] == PSEP_LBR)
                {
                    int local_stat = 0;
                    execute(arg + i + 1, &local_stat);
                    exit(local_stat);
                }
                else
                {

                    sigset_def(SIG_DFL);
                    execvp(arg[i], arg + i);
                    perror(TTL);
                    fflush(stderr);
                    _exit(1);
                }
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
                if (flags & UT_BACKGROUND)
                {
                    add_bg_pid(pid);
                }
                else
                {
                    tty_make_fg(pgid);
                    add_fg_pid(pid);
                }
                executed += 1;
            }
        }

        i = nxt_i + 1;
    }

    replace_fd(sv_inp, 0);
    replace_fd(sv_out, 1);

    if (!executed)
    {
        status = 1;
        return status;
    }

    if (!(flags & UT_BACKGROUND))
    {
        wait_fg(&status);
        tty_make_fg(getpgid(0));
    }

    return status;
}
/* ------------------------------------------------------------*/
/* PARSE ARRAY OF WORDS */
/* ------------------------------------------------------------*/

static void parse_execute(char **arg, int *links, int *status)
{
    char *cinput = NULL, *coutput = NULL;
    int flags = 0;
    int cmd_cnt = 1;

    size_t sep;
    int lb = 0, rb = 0;
    int lst = 0;

    if (arg == NULL)
    {
        *status = 0;
        return;
    }

    for (; arg[rb] != NULL; ++rb)
    {
        if (is_psep(arg[rb]))
        {
            switch ((size_t)arg[rb])
            {
            case PSEP_OR:
            case PSEP_AND:
            case PSEP_SEMI:
            case PSEP_BG:

                if ((size_t)arg[rb] == PSEP_BG)
                {
                    flags |= EX_BACKGROUND;
                }

                sep = (size_t)arg[rb];

                arg[lst++] = NULL;
                *status = exec(arg + lb, cmd_cnt, cinput, coutput, flags);

                cinput = NULL;
                coutput = NULL;
                flags = 0;
                cmd_cnt = 1;

                if (sep == PSEP_AND && (*status) != 0)
                {
                    return;
                }
                else if (sep == PSEP_OR && (*status) == 0)
                {
                    return;
                }

                lb = rb + 1;
                lst = lb;

                break;
            case PSEP_IN:
                if (cinput != NULL)
                {
                    ERR("multiply definition of input stream.");
                    *status = 1;
                    return;
                }

                cinput = arg[rb + 1];
                ++rb;
                break;
            case PSEP_OUT:
            case PSEP_OUTT:
                if (coutput != NULL)
                {
                    ERR("multimply definition of output stream.");
                    *status = 1;
                    return;
                }

                if ((size_t)arg[rb] == PSEP_OUTT)
                {
                    flags |= EX_TRUNCATE;
                }
                coutput = arg[rb + 1];
                ++rb;
                break;
            case PSEP_PIPE:
                arg[lst++] = NULL;
                ++cmd_cnt;
                break;
            case PSEP_LBR:
                arg[lst++] = arg[rb];
                for (int j = rb + 1; j < links[rb]; ++j)
                    arg[lst++] = arg[j];
                rb = links[rb];
            }
        }
        else
        {
            arg[lst++] = arg[rb];
        }
    }

    arg[lst++] = NULL;

    if (arg[lb] != NULL)
    {
        *status = exec(arg + lb, cmd_cnt, cinput, coutput, flags);
    }
}

static int *get_links(char **arg)
{
    int len = 0;
    int *ptr;
    int *stack;

    for (; arg[len] != NULL; ++len)
    {
    }

    if (len == 0)
        return NULL;

    ptr = malloc(len * sizeof(int));
    stack = calloc(len, sizeof(int));
    for (int i = 0; i < len; ++i)
        ptr[i] = -1;

    int st_top = 0;

    for (int i = 0; i < len; ++i)
    {
        if ((size_t)arg[i] == PSEP_LBR)
        {
            stack[st_top++] = i;
        }
        else if ((size_t)arg[i] == PSEP_RBR)
        {
            if (st_top == 0)
            {
                free(stack);
                free(ptr);
                return NULL;
            }
            else
            {
                ptr[stack[st_top - 1]] = i;
                --st_top;
            }
        }
    }
    free(stack);
    return ptr;
}

static int good_cmd(char **arg)
{
    if (is_psep(arg[0]) && (size_t)arg[0] != PSEP_LBR &&
        ((size_t)arg[0] < PSEP_OUT || (size_t)arg[0] > PSEP_IN))
        return 1;

    for (int i = 1; arg[i] != NULL; ++i)
    {
        if (is_psep(arg[i]) && is_psep(arg[i - 1]))
        {
            size_t a1 = (size_t)arg[i], a2 = (size_t)arg[i - 1];
            if (!(a1 >= PSEP_OUT && a1 <= PSEP_IN &&
                  (a2 < PSEP_OUT || a2 > PSEP_IN)) &&
                !(a1 == PSEP_LBR && a2 != PSEP_RBR) && !(a2 == PSEP_RBR))
            {
                return 1;
            }
        }
    }

    return 0;
}

void execute(char **arg, int *status)
{
    int *links;

    if (arg == NULL)
        return;

    if (good_cmd(arg))
    {
        *status = 1;
        ERR("bad line.")
        return;
    }

    links = get_links(arg);
    if (links == NULL)
    {
        ERR("error parentheses.");
        free(links);
        *status = 1;
        return;
    }

    parse_execute(arg, links, status);

    free(links);
}

/* ---------------------------------------------------------------------- */
/* INIT EXEC */
/* -----------------------------------------------------------------------*/

void init_exec()
{
    sigset_def(SIG_IGN);
    signal_init();
}

void free_exec() {}
