#include "ppchars.h"

static pchar *pchar_init(char *str, pchar *prev)
{
    pchar *res = malloc(sizeof(pchar));
    res->str = str;
    res->next = NULL;
    res->prev = prev;
    return res;
}

static void pchar_free(pchar *ptr)
{
    if ((size_t)ptr->str > PSEP_COUNT)
        free(ptr->str);
    free(ptr);
}

ppchar *ppchar_init()
{
    ppchar *ptr = malloc(sizeof(ppchar));
    ptr->end = NULL;
    ptr->len = 0;
    return ptr;
}

void ppchar_add(ppchar *ptr, char *str)
{
    if (ptr->end == NULL)
    {
        ptr->end = pchar_init(str, NULL);
        ptr->len = 1;
        return;
    }

    ptr->len += 1;
    ptr->end->next = pchar_init(str, ptr->end);
    ptr->end = ptr->end->next;
}

void ppchar_pop(ppchar *ptr)
{
    pchar *new;
    if (ptr->end == NULL)
        return;

    --ptr->len;
    new = ptr->end->prev;
    pchar_free(ptr->end);
    ptr->end = new;
}

void ppchar_free(ppchar *ptr)
{
    while (ptr->len)
    {
        ppchar_pop(ptr);
    }
}

char **ppchar_ptr(ppchar *ptr)
{
    pchar *cur = ptr->end;
    char **res = calloc(ptr->len + 1, sizeof(char *));
    for (int i = ptr->len - 1; i >= 0; --i, cur = cur->prev)
    {
        if ((size_t)cur->str > PSEP_COUNT)
        {
            int len = strlen(cur->str);
            res[i] = calloc(len, sizeof(char));
            memcpy(res[i], cur->str, len);
        }
        else
        {
            res[i] = cur->str;
        }
    }
    return res;
}
