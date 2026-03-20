#include "vfs.h"
#include "stroki.h"

typedef struct
{
    char path[32];
    char data[128];
} VfsFile;

static VfsFile g_files[8];
static int g_count = 0;

static int str_len127(const char* s)
{
    int i = 0;
    while (i < 127 && s[i])
    {
        i = i + 1;
    }
    return i;
}

static int str_len31(const char* s)
{
    int i = 0;
    while (i < 31 && s[i])
    {
        i = i + 1;
    }
    return i;
}

static void str_copy127(char* dst, const char* src)
{
    int i = 0;
    while (i < 127 && src[i])
    {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

static void str_copy31(char* dst, const char* src)
{
    int i = 0;
    while (i < 31 && src[i])
    {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

static int find_index(const char* path)
{
    int i = 0;
    while (i < g_count)
    {
        if (stroki_odinakovie(g_files[i].path, path))
        {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

void vfs_init()
{
    g_count = 3;

    str_copy31(g_files[0].path, "/hello.txt");
    str_copy127(g_files[0].data, "Hello from Shift VFS");

    str_copy31(g_files[1].path, "/about.txt");
    str_copy127(g_files[1].data, "Shift OS in-memory virtual filesystem");

    str_copy31(g_files[2].path, "/notes.txt");
    str_copy127(g_files[2].data, "Use: ls, cat <path>, write <path> <text>");
}

int vfs_count()
{
    return g_count;
}

const char* vfs_path_at(int index)
{
    if (index < 0 || index >= g_count)
    {
        return 0;
    }
    return g_files[index].path;
}

int vfs_read(const char* path, char* out, int max_len)
{
    int idx = find_index(path);
    int i = 0;

    if (idx < 0 || max_len <= 0)
    {
        return 0;
    }

    while (i < (max_len - 1) && g_files[idx].data[i])
    {
        out[i] = g_files[idx].data[i];
        i = i + 1;
    }
    out[i] = 0;
    return 1;
}

int vfs_write(const char* path, const char* text)
{
    int idx = find_index(path);

    if (str_len31(path) == 0 || str_len127(text) > 127)
    {
        return 0;
    }

    if (idx < 0)
    {
        if (g_count >= 8)
        {
            return 0;
        }

        idx = g_count;
        g_count = g_count + 1;
        str_copy31(g_files[idx].path, path);
    }

    str_copy127(g_files[idx].data, text);
    return 1;
}
