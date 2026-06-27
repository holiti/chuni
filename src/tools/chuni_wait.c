#include "chuni_wait.h"

enum
{
    STAT_LEN = 3,
    JOB_BUFFER_LEN = 1024,
};

static char *stat_msg[STAT_LEN] = {"exited", "signaled", "stoped"};

static char job_buff[JOB_BUFFER_LEN];
static char *bg_msg_front = TTL ": Job ";
static char *bg_msg_has = ", has ";

/* ----------------------------------------------------------------*/
/* STRUCTURE FOR JOBS*/
/* ----------------------------------------------------------------*/
typedef struct cmd
{
    size_t pid;
    struct cmd *next;
} cmd;

typedef struct job
{
    int cmd_count;
    struct cmd *c_last;
    struct cmd *c_begin;
    struct job *next;
    struct job *prev;
} job;

static cmd *cmd_init(size_t pid)
{
    cmd *new = malloc(sizeof(cmd));
    new->pid = pid;
    new->next = NULL;
    return new;
}

static job *job_init(job *prev)
{
    job *new = malloc(sizeof(job));
    new->cmd_count = 0;
    new->c_begin = new->c_last = NULL;
    new->next = NULL;
    new->prev = prev;
    return new;
}

void job_add(job **last)
{
    if (*last == NULL)
    {
        *last = job_init(NULL);
    }
    else
    {
        (*last)->next = job_init(*last);
        *last = (*last)->next;
    }
}

static void job_add_pid(job *jb, size_t pid)
{
    if (jb->c_begin == NULL)
    {
        jb->cmd_count = 1;
        jb->c_begin = jb->c_last = cmd_init(pid);
    }
    else
    {
        jb->cmd_count += 1;
        jb->c_last->next = cmd_init(pid);
        jb->c_last = jb->c_last->next;
    }
}

static cmd *cmd_find(cmd *begin, size_t pid)
{
    while (begin != NULL && begin->pid != pid)
    {
        begin = begin->next;
    }
    return begin;
}

static void cmd_free(cmd *begin)
{
    cmd *tmp;
    while (begin != NULL)
    {
        tmp = begin;
        begin = begin->next;
        free(tmp);
    }
}

static void job_free(job *jb)
{
    cmd_free(jb->c_begin);
    free(jb);
}

static job *bg_last = NULL;
static job *fg_job = NULL;

/* ----------------------------------------------------------------*/
/* STATUS TOOLS */
/* ----------------------------------------------------------------*/

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

static void print_bg_msg(long int id, long int pid, const char *str)
{
    int i = 0;
    int len = strlen(bg_msg_front);
    char *num_buff;
    memcpy(job_buff, bg_msg_front, len);
    i += len;

    num_buff = itos(id, &len);
    memcpy(job_buff + i, num_buff, len);
    i += len;

    job_buff[i++] = ':';

    num_buff = itos(pid, &len);
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

static void pop_bg()
{
    job *tmp;
    while (bg_last != NULL && bg_last->cmd_count == 0)
    {
        tmp = bg_last;
        bg_last = bg_last->prev;
        job_free(tmp);
    }
}

static void end_bg(size_t pid, int status)
{
    /* SIGNAL SAFETY */
    job *cur = bg_last;
    int i;

    if (bg_last == NULL)
        return;

    while (cur->prev != NULL)
        cur = cur->prev;

    for (i = 0; cur != NULL && cmd_find(cur->c_begin, pid) == NULL; ++i)
    {
        cur = cur->next;
    }

    if (cur == NULL)
        return;

    cur->cmd_count -= 1;
    if (cur->cmd_count == 0)
    {
        print_bg_msg(i + 1, pid, stat_msg[get_stat_id(status)]);
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

void add_bg()
{
    pop_bg();
    job_add(&bg_last);
}

void add_bg_pid(size_t pid) { job_add_pid(bg_last, pid); }

/* ---------------------------------------------------------------*/
/* FOREGROUND TOOLS */
/* ---------------------------------------------------------------*/

struct sigaction s_def;
struct sigaction s_new;

void add_fg() { job_add(&fg_job); }

void add_fg_pid(size_t pid) { job_add_pid(fg_job, pid); }

void wait_fg()
{

    sigaction(SIGCHLD, &s_def, NULL);
    size_t pid = 0;
    int status = 0;

    while (fg_job->cmd_count > 0)
    {
        pid = wait(&status);
        if (cmd_find(fg_job->c_begin, pid) == NULL)
        {
            end_bg(pid, status);
        }
        else
        {
            fg_job->cmd_count -= 1;
        }
    }

    job_free(fg_job);
    fg_job = NULL;
    sigaction(SIGCHLD, &s_new, NULL);
}

void signal_init()
{
    s_new.sa_handler = sigchld;
    s_new.sa_flags = SA_RESTART;
    sigemptyset(&s_new.sa_mask);

    sigaction(SIGCHLD, &s_new, &s_def);
}
