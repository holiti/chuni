#ifndef CONSTANT
#define CONSTANT

#define TTL "Chuni"
#define CMD_LINE                                                               \
    "\033[33m[" TTL "]\033[34m"                                                \
    " < %s > "                                                                 \
    "\033[37m"                                                                 \
    "|"                                                                        \
    "\033[31m"                                                                 \
    " <%d> "                                                                   \
    "\033[0m"

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
    PSEP_OR = 1,
    PSEP_AND = 2,
    PSEP_OUT = 3,
    PSEP_OUTT = 4,
    PSEP_IN = 5,
    PSEP_PIPE = 6,
    PSEP_LBR = 7,
    PSEP_RBR = 8,
    PSEP_SEMI = 9,
    PSEP_BG = 10,

    VAR_SEP = ':',
    MAX_MANAGE_CODE = 31,

    PATH_MAX = 2048,
};

extern const char *sep[PSEP_COUNT];
int is_psep(char *ptr);
#endif
