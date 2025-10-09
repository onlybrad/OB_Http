#ifndef OB_HTTP_UTIL_H
#define OB_HTTP_UTIL_H

#include <stdlib.h>

#define OB_MALLOC  malloc
#define OB_FREE    free
#define OB_REALLOC realloc

#define OB_MAX(A,B) ((A) > (B) ? (A) : (B))
#define OB_MIN(A,B) ((A) < (B) ? (A) : (B))

#endif