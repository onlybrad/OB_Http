#ifndef OB_HTTP_BUFFER_H
#define OB_HTTP_BUFFER_H

#include <stdbool.h>

struct OB_Buffer {
    unsigned char *data;
    size_t         capacity;
    size_t         size;
};

bool           OB_Buffer_init   (struct OB_Buffer*, size_t);
bool           OB_Buffer_reserve(struct OB_Buffer*, size_t);
unsigned char *OB_Buffer_append (struct OB_Buffer*, unsigned char*, size_t);
void           OB_Buffer_reset  (struct OB_Buffer*);
void           OB_Buffer_free   (struct OB_Buffer*);


#endif