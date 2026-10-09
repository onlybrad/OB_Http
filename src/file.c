#include <assert.h>

#include "file.h"
#include "util.h"

#ifdef _WIN32
#include <windows.h>

static bool utf8_to_utf16(wchar_t *const dst, const char *const src, const int dst_capacity) {
    assert(dst != NULL);
    assert(src != NULL);

    const int utf16_length = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if(utf16_length == 0 || utf16_length > dst_capacity) {
        return false;
    }

    return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, utf16_length) == utf16_length;
}
#endif

OB_EXTERN_C FILE *OB_fopen(const char *const path, const char *const mode) {
    assert(path != NULL);
    assert(mode != NULL);
#ifdef _WIN32
    wchar_t wpath[OB_SIZE_MAX + 1];
    if(!utf8_to_utf16(wpath, path, (int)OB_ARRAY_LENGTH(wpath))) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    wchar_t wmode[8];
    if(!utf8_to_utf16(wmode, mode, (int)OB_ARRAY_LENGTH(wmode))) {
        errno = EINVAL;
        return NULL;
    }

    return _wfopen(wpath, wmode);
#else
    return fopen(path, mode);
#endif
}