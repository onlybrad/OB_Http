#ifndef OB_HTTP_UTIL_H
#define OB_HTTP_UTIL_H

#include <stdlib.h>
#include <stdint.h>

#define OB_MALLOC  malloc
#define OB_FREE    free
#define OB_REALLOC realloc

#define OB_MAX(A,B) ((A) > (B) ? (A) : (B))
#define OB_MIN(A,B) ((A) < (B) ? (A) : (B))

struct OB_Size {
    const char *units;
    double      value;
};

void OB_Util_strtolower(char*);
int64_t OB_get_usec_timestamp(void);
struct OB_Size OB_format_bytes(const size_t bytes);

#endif