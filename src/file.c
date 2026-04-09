#include "file.h"

#ifdef _WIN32
#include <windows.h>
#endif

FILE *OB_fopen(const char *const path, const bool is_readmode) {
#ifdef _WIN32
    wchar_t wpath[1024];

    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0 || wide_length >= (int)(sizeof(wpath) / sizeof(wpath[0]))) {
        return NULL;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);
    wpath[wide_length] = L'\0';

    return _wfopen(wpath, is_readmode ? L"rb" : L"wb");
#else
    return fopen(path, is_readmode ? "rb" : "wb");
#endif
}