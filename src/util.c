#include <assert.h>
#include <ctype.h>
#include "util.h"

void OB_Util_strtolower(char *str) {
    assert(str != NULL);

    while(*str != '\0') {
        *str = (char)tolower((int)(*str));
        str++;
    }
}