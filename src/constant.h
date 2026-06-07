#ifndef CONSTANT
#define CONSTANT

#define TTL "Chuni"

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
    PSEP_AND,
    PSEP_OUT,
    PSEP_OUTT,
    PSEP_IN,
    PSEP_OR1,
    PSEP_LBR,
    PSEP_RBR,
    PSEP_SEMI,
    PSEP_BACK,
};

extern const char *sep[PSEP_COUNT];
#endif
