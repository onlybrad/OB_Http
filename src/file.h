#ifndef OB_HTTP_FILE_H
#define OB_HTTP_FILE_H

#include <stdio.h>
#include <stdbool.h>

FILE *OB_fopen(const char *path, bool is_readmode);

#endif