#include <assert.h>
#include <ctype.h>
#include "util.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

void OB_Util_strtolower(char *str) {
    assert(str != NULL);

    while(*str != '\0') {
        *str = (char)tolower((int)(*str));
        str++;
    }
}

int64_t OB_get_usec_timestamp(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (int64_t)(
        (counter.QuadPart / frequency.QuadPart) * 1000000LL +
        (counter.QuadPart % frequency.QuadPart) * 1000000LL / frequency.QuadPart
    );
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (int64_t)ts.tv_sec * 1000000LL +
           (int64_t)(ts.tv_nsec / 1000);
#endif
}

struct OB_Bytes OB_format_bytes(const size_t bytes) {
    static const char *OB_units[] = {
        "B", "KB", "MB", "GB", "TB"
    };

    struct OB_Bytes byte_units;
    byte_units.value = (double)bytes;

    unsigned i;
    for(i = 0U; i < sizeof(OB_units)/sizeof(OB_units[0]) && byte_units.value > 1024.0; i++) {
        byte_units.value /= 1024.0;
    }

    byte_units.units = OB_units[i];

    return byte_units;
}