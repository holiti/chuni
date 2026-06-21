#include "exec.h"
#include "execunit.h"

enum
{
    STAT_LEN = 3,
    JOB_BUFFER_LEN = 1024,
    NUM_BUFFER_LEN = 32,
    BG_PROCESS_LEN = 1024,
};

#define ERR(msg) fprintf(stderr, TTL ": %s\n", msg);

char job_buff[JOB_BUFFER_LEN];
char num_buff[NUM_BUFFER_LEN];
char *bg_msg_front = TTL ": Job ";
char *bg_msg_has = ", has ";
int bg_id = 0;
int bg_cnt = 0;
int bg_pids[BG_PROCESS_LEN];

struct sigaction sold;
struct sigaction snew;

static void reverse(char *ptr, int len)
{
    int tmp;
    for (int i = 0, mid = len / 2; i < mid; ++i)
    {
        tmp = ptr[i];
        ptr[i] = ptr[len - 1 - i];
        ptr[len - 1 - i] = tmp;
    }
}

/* ----------------------------------------------------------------*/
/* STATUS TOOLS */
/* ----------------------------------------------------------------*/

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

/* ---------------------------------------------------------------*/
/* BACKGROUND EXECUTION SUPPORT */
/* ---------------------------------------------------------------*/

static int itos(long int n)
{
    int len = 0;
    while (n)
    {
        num_buff[len++] = (n % 10) + '0';
        n /= 10;
    }
    num_buff[len] = 0;
    reverse(num_buff, len);
    return len;
}

static void print_bg_msg(long int id, long int pid, const char *str)
{
    int i = 0;
    int len = strlen(bg_msg_front);
    memcpy(job_buff, bg_msg_front, len);
    i += len;

    len = itos(id);
    memcpy(job_buff + i, num_buff, len);
    i += len;

    job_buff[i++] = ':';

    len = itos(pid);
    memcpy(job_buff + i, num_buff, len);
    i += len;

    len = strlen(bg_msg_has);
    memcpy(job_buff + i, bg_msg_has, len);
    i += len;

    len = strlen(str);
    memcpy(job_buff + i, str, len);
    i += len;

    job_buff[i++] = '\n';

    write(0, job_buff, i);
}

static void end_bg(size_t pid, int status)
{
    int i;
    int id = 0;
    int last_bg = 0;

    for (i = 0; i < bg_id; ++i)
    {
        if (bg_pids[i] < 0)
        {
            last_bg = i;
            ++id;
        }
        else if (bg_pids[i] == pid)
        {
            break;
        }
    }

    ++bg_pids[last_bg];
    if (bg_pids[last_bg] == -1)
    {
        print_bg_msg(id, pid, stat[get_stat_id(status)]);
        --bg_cnt;
    }

    bg_pids[i] = 0;
    while (bg_id > 0 && bg_pids[bg_id - 1] <= 0)
    {
        --bg_id;
    }
}

static void sigchld(int sig)
{
    int status;
    size_t pid;

    pid = waitpid(-1, &status, WNOHANG);
    if (pid <= 0)
        return;
    end_bg(pid, status);
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
    int need_wait = 0;
    int chuni_cmd_id;

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
            {
                ++cmd_cnt;
                cur->pid = 0;
            }
        }
        else
        {

            pid = fork();
            if (pid == 0)
            {
                if (pip[0] != -1)
                    close(pip[0]);

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
                ++cmd_cnt;
                ++need_wait;
                cur->pid = pid;
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

    if ((ut->flags & UT_BACKGROUND) && need_wait)
    {
        bg_pids[bg_id++] = -(need_wait + 1);
        cur = ut->first;
        while (cur != NULL)
        {
            bg_pids[bg_id++] = cur->pid;
            cur = cur->next;
        }
    }
    else if (need_wait)
    {
        sigaction(SIGCHLD, &sold, NULL);
        do
        {
            pid = wait(&status);
            cur = ut->first;
            while (cur != NULL)
            {
                if (cur->pid == pid)
                    break;
                cur = cur->next;
            }

            if (cur == NULL)
                end_bg(pid, status);
            else
            {
                cur->pid = 0;
                --need_wait;
            }
        } while (need_wait);

        sigaction(SIGCHLD, &snew, NULL);
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

    for (int i = 0; i < BG_PROCESS_LEN; ++i)
        bg_pids[i] = 0;

    snew.sa_handler = sigchld;
    snew.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &snew, &sold);
}

void free_exec() {}
