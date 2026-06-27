#include "tools.h"

enum
{
    NUM_BUFF_LEN = 16,
};
static char num_buff[NUM_BUFF_LEN];

void reverse(char *ptr, int len)
{
    int tmp;
    for (int i = 0, mid = len / 2; i < mid; ++i)
    {
        tmp = ptr[i];
        ptr[i] = ptr[len - 1 - i];
        ptr[len - 1 - i] = tmp;
    }
}

char *itos(long int n, int *len)
{
    *len = 0;
    while (n)
    {
        num_buff[(*len)++] = (n % 10) + '0';
        n /= 10;
    }
    num_buff[*len] = 0;
    reverse(num_buff, *len);
    return num_buff;
}
