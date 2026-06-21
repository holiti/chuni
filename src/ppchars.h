#ifndef CHUNI_PPCHARS
#define CHUNI_PPCHARS

#include "constant.h"
#include <stdlib.h>
#include <string.h>

typedef struct pchar
{
    char *str;
    struct pchar *next, *prev;
} pchar;

typedef struct ppchar
{
    int len;
    struct pchar *end;
} ppchar;

ppchar *ppchar_init();
void ppchar_add(ppchar *ptr, char *str);
void ppchar_pop(ppchar *ptr);
void ppchar_free(ppchar *ptr);
char **ppchar_ptr(ppchar *ptr);

#endif
