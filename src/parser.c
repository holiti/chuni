#include "parser.h"

/* Separators
 1. ||
 2. &&
 3. >>
 4. >
 5. <
 6. |
 7. (
 8. )
 9. ;
 10. &
 */

enum
{
    PSEP_COUNT = 11,
    PSEP_SZ2 = 4,
};

static const char *sep[PSEP_COUNT] = {"",  "||", "&&", ">>", ">", "<",
                                      "|", "(",  ")",  ";",  "&"};

DYN_STRUCT(char *, pchar);

static void free_pchar(dyn_pchar *pch)
{
    for (int i = 0; i < pch->len; ++i)
        free(pch->arr[i]);
    free(pch);
}

static size_t what_sep(const char *str)
{
    int size;
    int bad;
    for (int i = 1; i < PSEP_COUNT; ++i)
    {
        size = 1 + (i < PSEP_SZ2);
        bad = 0;
        for (int j = 0; j < size; ++j)
            if (str[j] != sep[i][j])
            {
                bad = 1;
                break;
            }
        if (!bad)
        {
            return i;
        }
    }
    return -1;
}

static void add_word(const char *str, dyn_pchar *words, int l, int r, int badch)
{
    int wlen = r - l + 1;
    char *word = calloc(wlen - badch + 1, sizeof(char));

    dyn_pchar_push(words, word);
    for (int x = 0; l <= r; ++l)
    {
        if (str[l] == '\\')
        {
            // screening of char
            ++l;
        }
        else if (str[l] == '"')
        {
            continue;
        }
        word[x++] = str[l];
    }
}

char **parse_str(const char *str)
{
    int strl = strlen(str);
    int inq = 0;
    int bleft = -1;
    int badch = 0;
    dyn_pchar *words = dyn_pchar_init();

    for (int i = 0; i < strl; ++i)
    {
        size_t isep = -1;
        if (str[i] == '\\')
        {
            if (i == strl - 1)
            {
                free_pchar(words);
                return NULL;
            }
            badch += 1;
            ++i;
        }
        else if (inq == 0 &&
                 (((str[i] == ' ' || str[i] == '\t') && bleft != -1) ||
                  (isep = what_sep(str + i)) != -1))
        {
            // add new word
            if (bleft != -1)
                add_word(str, words, bleft, i - 1, badch);
            badch = 0;
            bleft = -1;

            if (isep != -1)
            {
                dyn_pchar_push(words, (char *)isep);
                if (isep < PSEP_SZ2)
                {
                    ++i;
                }
            }
        }
        else if (str[i] == '"')
        {
            if (inq == 0 && bleft == -1)
            {
                bleft = i;
            }
            inq ^= 1;
            badch += 1;
        }
        else if (str[i] != ' ' && str[i] != '\t')
        {
            if (bleft == -1)
                bleft = i;
        }
    }

    if (inq)
    {
        free_pchar(words);
        return NULL;
    }

    if (bleft != -1)
    {
        add_word(str, words, bleft, strl - 1, badch);
    }
    dyn_pchar_push(words, NULL);

    return words->arr;
}
