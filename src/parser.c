#include "parser.h"

void freepp(char **arg)
{
    if (arg == NULL)
        return;
    for (int i = 0; arg[i] != NULL; ++i)
    {
        if ((size_t)arg[i] > 10)
            free(arg[i]);
    }
}

/* ------------------------------------------------------------------ */
/*PARSER PLACE */
/* ------------------------------------------------------------------ */

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

char **parse_var(const char *const str)
{
    int len = strlen(str);
    ppchar *words = ppchar_init();
    char **res;
    char *word;
    int left = 0;

    for (int i = 0; i < len; ++i)
    {
        if (str[i] == VAR_SEP)
        {
            if (left < i)
            {
                word = calloc(i - left + 1, sizeof(char));
                for (int j = left; j < i; ++j)
                    word[j - left] = str[j];
                ppchar_add(words, word);
            }
            left = i + 1;
        }
    }
    if (left < len)
    {
        word = calloc(len - left + 1, sizeof(char));
        for (int j = left; j < len; ++j)
            word[j - left] = str[j];
        ppchar_add(words, word);
    }
    ppchar_add(words, NULL);

    res = ppchar_ptr(words);
    ppchar_free(words);
    return res;
}

static void add_word(const char *str, ppchar *words, int l, int r, int badch)
{
    int wlen = r - l + 1;
    char *word = calloc(wlen - badch + 1, sizeof(char));

    ppchar_add(words, word);
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

char **parse_str(const char *const str)
{
    int strl = strlen(str);
    int inq = 0;
    int bleft = -1;
    int badch = 0;
    ppchar *words = ppchar_init();
    char **res;

    for (int i = 0; i < strl; ++i)
    {
        size_t isep = -1;
        if (str[i] == '\\')
        {
            if (i == strl - 1)
            {
                ppchar_free(words);
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
                ppchar_add(words, (char *)isep);
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
        ppchar_free(words);
        return NULL;
    }

    if (bleft != -1)
    {
        add_word(str, words, bleft, strl - 1, badch);
    }

    res = ppchar_ptr(words);
    ppchar_free(words);
    return res;
}
