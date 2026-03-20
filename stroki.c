#include "stroki.h"

int stroki_odinakovie(const char* a, const char* b)
{
    int i = 0;
    while (a[i] && b[i])
    {
        if (a[i] != b[i])
        {
            return 0;
        }
        i = i + 1;
    }
    return a[i] == b[i];
}

int nachinaetsya_s(const char* s, const char* prefix)
{
    int i = 0;
    while (prefix[i])
    {
        if (s[i] != prefix[i])
        {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}