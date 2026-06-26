#include "edit.h"

static char **dirs;

enum
{
    CBACKSPACE = 127,
    CARROW = 27,
    ALEFT = 68,
    ARIGHT = 67,
    CCTRLW = 23,
    CCTRLA = 1,
    CCTRLE = 5,
    CTAB = 9,
    CNEWLINE = 10,
};

/* ------------------------------------------------------------------ */
/* MOVEMENT FUNCTION */
/* ------------------------------------------------------------------ */

static void curmvleft(int chars)
{
    if (chars < 1)
        return;
    printf("\033[%dD", chars);
    fflush(stdout);
}

static void curmvright(int chars)
{
    if (chars < 1)
        return;
    printf("\033[%dC", chars);
    fflush(stdout);
}

static void backspace(int chars)
{
    if (chars < 1)
        return;
    printf("\033[%dP", chars);
    fflush(stdout);
}

static void clearline()
{
    printf("\r\033[K");
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* STRUCTURES FOR INPUT */
/* ------------------------------------------------------------------ */

typedef struct sc
{
    char ch;
    struct sc *prev, *next;
} schar;

typedef struct ss
{
    int id, len;
    schar *begin;
    schar *cur;
} string;

schar *sc_init(char ch, schar *prev, schar *next)
{
    schar *obj = malloc(sizeof(schar));
    obj->ch = ch;
    obj->prev = prev;
    obj->next = next;
    return obj;
}

schar *sc_add(schar *ptr, char ch)
{
    schar *nxt = ptr->next;
    ptr->next = sc_init(ch, ptr, nxt);
    if (nxt != NULL)
        nxt->prev = ptr->next;
    return ptr->next;
}

schar *sc_del(schar *ptr)
{
    schar *prev = ptr->prev;
    if (prev != NULL)
        prev->next = ptr->next;
    if (ptr->next != NULL)
        ptr->next->prev = prev;

    free(ptr);
    return prev;
}

string *s_init()
{
    string *obj = malloc(sizeof(string));
    obj->begin = sc_init(' ', NULL, NULL);
    obj->cur = obj->begin;
    obj->len = 0;
    obj->id = 0;

    return obj;
}
/* ------------------------------------------------------------------ */
/* ACTIONS */
/* ------------------------------------------------------------------ */

static char path[PATH_MAX];
static void pwd(char *path, int len) { getcwd(path, len - 1); }

extern int main_status;
static void s_print(string *ptr, int fd)
{

    pwd(path, PATH_MAX);
    printf(CMD_LINE, path, main_status);
    fflush(stdout);

    schar *cr = ptr->begin->next;
    while (cr != NULL)
    {
        putchar(cr->ch);
        cr = cr->next;
    }
    fflush(stdout);
}

static void mvbeginln(string *ptr)
{
    curmvleft(ptr->id);
    ptr->id = 0;
    ptr->cur = ptr->begin;
}

static void mvendln(string *ptr)
{
    curmvright(ptr->len - ptr->id);
    ptr->id = ptr->len;
    while (ptr->cur->next != NULL)
    {
        ptr->cur = ptr->cur->next;
    }
}

static void s_add(string *ptr, char ch)
{
    if (ch != 0)
    {
        ptr->cur = sc_add(ptr->cur, ch);

        ++ptr->len;
        ++ptr->id;
    }

    clearline();
    s_print(ptr, 1);

    curmvleft(ptr->len - ptr->id);
}

static void s_del_char(string *ptr)
{
    if (ptr->cur == ptr->begin)
        return;
    ptr->cur = sc_del(ptr->cur);
    --ptr->len;
    --ptr->id;
    curmvleft(1);
    backspace(1);
}

enum
{
    SLEFT = ALEFT,
    SRIGHT = ARIGHT,
};

void s_move(string *ptr, int side)
{
    if (side == SLEFT && ptr->begin != ptr->cur)
    {
        ptr->cur = ptr->cur->prev;
        --ptr->id;
        curmvleft(1);
    }
    else if (side == SRIGHT && ptr->cur->next != NULL)
    {
        ptr->cur = ptr->cur->next;
        ++ptr->id;
        curmvright(1);
    }
}

void s_del_word(string *ptr)
{
    while (ptr->cur->ch != 0 && ptr->cur->ch != ' ')
    {
        s_del_char(ptr);
    }
}

void s_clear(string *ptr)
{
    schar *cur = ptr->begin->next;
    ptr->cur = cur;
    while (ptr->cur != NULL)
    {
        cur = ptr->cur;
        ptr->cur = ptr->cur->next;
        free(cur);
    }

    ptr->begin->next = NULL;
    ptr->cur = ptr->begin;
    ptr->len = ptr->id = 0;
}

/* ------------------------------------------------------------------ */
/* INIT AND RECOVER */
/* ------------------------------------------------------------------ */

string *str;

int edit_init()
{
    if (!isatty(0))
    {
        return 1;
    }

    tty_init();
    str = s_init();

    dirs = parse_var(getenv("PATH"));

    return 0;
}

void edit_free()
{
    tty_recover();
    s_clear(str);
    free(str->begin);
    free(str);
    freepp(dirs);
}

/* ------------------------------------------------------------------ */
/* EDITING */
/* ------------------------------------------------------------------ */

static char *convert_to_pchar(string *str)
{
    char *rstr = calloc(str->len + 1, sizeof(char));

    schar *ptr = str->begin->next;
    for (int i = 0; ptr != NULL; ++i, ptr = ptr->next)
    {
        rstr[i] = ptr->ch;
    }
    rstr[str->len] = 0;
    return rstr;
}

/* ------------------------------------------------------------------ */
/* Auto-complete */
/* ------------------------------------------------------------------ */

static int good_entry(const char *word, int wlen, const char *entr, int elen)
{
    if (elen < wlen)
        return 0;
    for (int i = 0; i < wlen; ++i)
    {
        if (word[i] != entr[i])
            return 0;
    }

    return 1;
}

static void complete_file(const char *path, const char *word, ppchar *res)
{
    int wlen = strlen(word), elen;
    char *tmp;
    DIR *dir = opendir(path);
    struct dirent *ent;

    if (dir == NULL)
        return;

    while ((ent = readdir(dir)) != NULL)
    {
        elen = strlen(ent->d_name);
        if (good_entry(word, wlen, ent->d_name, elen))
        {
            tmp = calloc(elen + 1, sizeof(char));
            memcpy(tmp, ent->d_name, elen);
            ppchar_add(res, tmp);
        }
    }
}

static void complete_cmd(char *word, ppchar *entr)
{

    for (int i = 0; dirs[i] != NULL; ++i)
    {
        if ((size_t)dirs[i] <= PSEP_COUNT)
        {
            freepp(dirs);
            return;
        }

        complete_file(dirs[i], word, entr);
    }
}

static void detach_path(char *pstr, char **path, char **word)
{
    int slen = strlen(pstr);
    int wlen = 0;
    int i;
    for (i = slen - 1; i >= 0 && pstr[i] != '/'; --i, ++wlen)
    {
    }

    *word = calloc(wlen + 1, sizeof(char));
    for (int j = i + 1; j < slen; ++j)
    {
        (*word)[j - i - 1] = pstr[j];
    }

    if (wlen != slen)
    {
        *path = calloc(slen - wlen + 1, sizeof(char));
        for (int j = 0; j <= i; ++j)
            (*path)[j] = pstr[j];
    }
}

static void acomplete(string *str)
{
    int iscmd = 0;
    int len = 0;
    char *word = NULL, *path = NULL, *pstr = NULL;
    schar *cur, *tmp;
    ppchar *cr = ppchar_init();
    pchar *pcur;

    cur = str->cur;
    while (cur->ch != ' ')
    {
        cur = cur->prev;
    }
    if (cur == str->begin)
        iscmd = 1;
    cur = cur->next;

    tmp = cur;
    while (tmp != NULL && tmp->ch != ' ')
    {
        ++len;
        tmp = tmp->next;
    }

    pstr = calloc(len + 1, sizeof(char));
    tmp = cur;
    for (int i = 0; i < len; ++i, tmp = tmp->next)
        pstr[i] = tmp->ch;

    detach_path(pstr, &path, &word);

    if (iscmd && path == NULL)
    {
        complete_cmd(word, cr);
    }
    else
    {
        if (path == NULL)
            complete_file("./", word, cr);
        else
            complete_file(path, word, cr);
    }

    if (cr->len == 1)
    {
        pcur = cr->end;
        while (str->cur->next != NULL && str->cur->next->ch != ' ')
        {
            ++str->id;
            str->cur = str->cur->next;
        }

        for (int i = len - (path != NULL ? strlen(path) : 0);
             i < strlen(pcur->str); ++i)
        {
            s_add(str, pcur->str[i]);
        }
    }
    else if (cr->len > 1)
    {
        pcur = cr->end;
        while (pcur->prev != NULL)
        {
            pcur = pcur->prev;
        }
        putchar('\n');
        for (int i = 0; i < cr->len; ++i, pcur = pcur->next)
        {
            printf("%s\n", pcur->str);
        }
    }

    free(path);
    free(pstr);
    free(word);
    s_add(str, 0);
}

/* ------------------------------------------------------------------ */
/* Read string */
/* ------------------------------------------------------------------ */

char *read_str()
{
    char ch;
    char *rstr;

    tty_init();
    clearline();
    s_print(str, 1);
    while ((ch = getchar()) != '\n')
    {
        switch (ch)
        {
        case CARROW:
            ch = getchar();
            ch = getchar();

            s_move(str, ch);
            break;
        case CBACKSPACE:
            s_del_char(str);
            break;
        case CCTRLW:
            s_del_word(str);
            break;
        case CCTRLA:
            mvbeginln(str);
            break;
        case CCTRLE:
            mvendln(str);
            break;
        case CTAB:
            acomplete(str);
            break;
        default:
            if (ch <= MAX_MANAGE_CODE)
                s_add(str, '?');
            else
                s_add(str, ch);
        }
    }
    write(1, "\n", 1);

    rstr = convert_to_pchar(str);
    s_clear(str);
    tty_recover();
    return rstr;
}
