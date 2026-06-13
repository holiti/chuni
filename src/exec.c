#include "exec.h"

enum
{
    STAT_LEN = 3,
    UT_BACKGROUND = 1,
    UT_TRUNCATE = 2,
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

/* BACKGROUND EXECUTION SUPPORT */

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
            print_bg_msg(bg_cnt + 1, 0, stat[0]);
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
            bg_cnt += 1;
            bg_pids[bg_id++] = -2;
            bg_pids[bg_id++] = pid;
            status = 0;
        }
        else
        {
            int cpid;
            sigaction(SIGCHLD, &sold, NULL);
            do
            {
                cpid = wait(&status);
                if (cpid != pid)
                {
                    end_bg(cpid, status);
                }
            } while (cpid != pid);
            sigaction(SIGCHLD, &snew, NULL);
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

/* INIT AND FREE PIDS */
void init_exec()
{

    for (int i = 0; i < BG_PROCESS_LEN; ++i)
        bg_pids[i] = 0;

    snew.sa_handler = sigchld;
    snew.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &snew, &sold);
}

void free_exec() {}
