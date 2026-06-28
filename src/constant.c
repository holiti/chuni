#include "constant.h"

const char *sep[PSEP_COUNT] = {"",  "||", "&&", ">>", ">", "<",
                               "|", "(",  ")",  ";",  "&"};

int is_psep(char *ptr) { return (long int)ptr <= PSEP_COUNT; }
