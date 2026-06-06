#ifndef CONSTANT
#define CONSTANT

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

extern const char *sep[PSEP_COUNT];
#endif
