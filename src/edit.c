#include "edit.h"

struct termios old;

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
static void s_print(string *ptr, int fd)
{
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
    ptr->cur = sc_add(ptr->cur, ch);

    curmvleft(ptr->id);

    ++ptr->len;
    ++ptr->id;

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

int init_term()
{
    struct termios new;
    if (!isatty(0))
    {
        return 1;
    }

    tcgetattr(0, &old);
    new = old;

    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VTIME] = 0;
    new.c_cc[VMIN] = 1;

    tcsetattr(0, TCSANOW, &new);

    str = s_init();

    return 0;
}

void rec_term()
{
    tcsetattr(0, TCSANOW, &old);
    s_clear(str);
    free(str->begin);
    free(str);
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
/*
struct comut
{
    char *str;
    struct comut *next;
};

static void complete_cmd(string *str, schar *wrd) {}

static void complete_file(string *str, schar *wrd) {}

static void acomplete(string *str) {}
*/
/* ------------------------------------------------------------------ */
/* Read string */
/* ------------------------------------------------------------------ */

char *read_str()
{
    char ch;
    char *rstr;

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
            break;
        default:
            s_add(str, ch);
        }
    }
    write(1, "\n", 1);

    rstr = convert_to_pchar(str);
    s_clear(str);
    return rstr;
}
