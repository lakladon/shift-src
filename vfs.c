#include "vfs.h"
#include "stroki.h"
#include "iozh.h"

typedef struct
{
    char put[32];
    char tekst[128];
    unsigned int created;
    unsigned int modified;
} VfsFile;

static VfsFile failiki[8];
static int skolko_failov = 0;
static unsigned char sklad[4096];

#define ATAIOBASE 0x1F0
#define VFSDISKLBA 0
#define VFSDISKSECTORS 8

static int strlen127(const char* s)
{
    int i = 0;
    while (i < 127 && s[i])
    {
        i = i + 1;
    }
    return i;
}

static int strlen31(const char* s)
{
    int i = 0;
    while (i < 31 && s[i])
    {
        i = i + 1;
    }
    return i;
}

static int strfits31(const char* s)
{
    int i = 0;
    while (i < 32)
    {
        if (!s[i])
        {
            return i > 0;
        }
        i = i + 1;
    }
    return 0;
}

static int strfits127(const char* s)
{
    int i = 0;
    while (i < 128)
    {
        if (!s[i])
        {
            return 1;
        }
        i = i + 1;
    }
    return 0;
}

static void strcopy127(char* dst, const char* src)
{
    int i = 0;
    while (i < 127 && src[i])
    {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

static void strcopy31(char* dst, const char* src)
{
    int i = 0;
    while (i < 31 && src[i])
    {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
}

static void memzero(unsigned char* p, int n)
{
    int i = 0;
    while (i < n)
    {
        p[i] = 0;
        i = i + 1;
    }
}

static int atawaitnotbsy()
{
    int t = 0;
    while (t < 100000)
    {
        unsigned char st = zhmyak_in(ATAIOBASE + 7);
        if ((st & 0x80) == 0)
        {
            return 1;
        }
        t = t + 1;
    }
    return 0;
}

static int atawaitdrq()
{
    int t = 0;
    while (t < 100000)
    {
        unsigned char st = zhmyak_in(ATAIOBASE + 7);
        if (st & 0x01)
        {
            return 0;
        }
        if ((st & 0x80) == 0 && (st & 0x08))
        {
            return 1;
        }
        t = t + 1;
    }
    return 0;
}

static int atareadsectors(unsigned int lba, unsigned char count, unsigned char* out)
{
    unsigned int s = 0;

    if (count == 0)
    {
        return 1;
    }

    while (s < count)
    {
        int i = 0;

        if (!atawaitnotbsy())
        {
            return 0;
        }

        zhmyak_out(ATAIOBASE + 6, (unsigned char)(0xE0 | ((lba >> 24) & 0x0F)));
        zhmyak_out(ATAIOBASE + 2, 1);
        zhmyak_out(ATAIOBASE + 3, (unsigned char)(lba & 0xFF));
        zhmyak_out(ATAIOBASE + 4, (unsigned char)((lba >> 8) & 0xFF));
        zhmyak_out(ATAIOBASE + 5, (unsigned char)((lba >> 16) & 0xFF));
        zhmyak_out(ATAIOBASE + 7, 0x20);

        if (!atawaitdrq())
        {
            return 0;
        }

        while (i < 256)
        {
            unsigned short w = zhmyak_in16(ATAIOBASE + 0);
            out[0] = (unsigned char)(w & 0xFF);
            out[1] = (unsigned char)((w >> 8) & 0xFF);
            out = out + 2;
            i = i + 1;
        }

        lba = lba + 1;
        s = s + 1;
    }

    return 1;
}

static int atawritesectors(unsigned int lba, unsigned char count, const unsigned char* in)
{
    unsigned int s = 0;

    if (count == 0)
    {
        return 1;
    }

    while (s < count)
    {
        int i = 0;

        if (!atawaitnotbsy())
        {
            return 0;
        }

        zhmyak_out(ATAIOBASE + 6, (unsigned char)(0xE0 | ((lba >> 24) & 0x0F)));
        zhmyak_out(ATAIOBASE + 2, 1);
        zhmyak_out(ATAIOBASE + 3, (unsigned char)(lba & 0xFF));
        zhmyak_out(ATAIOBASE + 4, (unsigned char)((lba >> 8) & 0xFF));
        zhmyak_out(ATAIOBASE + 5, (unsigned char)((lba >> 16) & 0xFF));
        zhmyak_out(ATAIOBASE + 7, 0x30);

        if (!atawaitdrq())
        {
            return 0;
        }

        while (i < 256)
        {
            unsigned short w = (unsigned short)(in[0] | ((unsigned short)in[1] << 8));
            zhmyak_out16(ATAIOBASE + 0, w);
            in = in + 2;
            i = i + 1;
        }

        lba = lba + 1;
        s = s + 1;
    }

    if (!atawaitnotbsy())
    {
        return 0;
    }

    zhmyak_out(ATAIOBASE + 7, 0xE7);
    return atawaitnotbsy();
}

static void vfssetdefaults()
{
    skolko_failov = 0;
    int i = 0;
    while (i < 8)
    {
        failiki[i].created = 0;
        failiki[i].modified = 0;
        i = i + 1;
    }
}

static int vfsloadfromdisk()
{
    int i;
    int pos;

    if (!atareadsectors(VFSDISKLBA, VFSDISKSECTORS, sklad))
    {
        return 0;
    }

    if (!(sklad[0] == 'S' && sklad[1] == 'V' && sklad[2] == 'F' && sklad[3] == '1'))
    {
        return 0;
    }

    if (sklad[4] > 8)
    {
        return 0;
    }

    skolko_failov = sklad[4];
    pos = 5;

    i = 0;
    while (i < skolko_failov)
    {
        strcopy31(failiki[i].put, (const char*)(sklad + pos));
        pos = pos + 32;
        strcopy127(failiki[i].tekst, (const char*)(sklad + pos));
        pos = pos + 128;

        if (!strfits31(failiki[i].put) || !strfits127(failiki[i].tekst))
        {
            return 0;
        }

        i = i + 1;
    }

    return 1;
}

static void vfssavetodisk()
{
    int i = 0;
    int pos = 5;

    memzero(sklad, 4096);
    sklad[0] = 'S';
    sklad[1] = 'V';
    sklad[2] = 'F';
    sklad[3] = '1';
    sklad[4] = (unsigned char)skolko_failov;

    while (i < skolko_failov)
    {
        strcopy31((char*)(sklad + pos), failiki[i].put);
        pos = pos + 32;
        strcopy127((char*)(sklad + pos), failiki[i].tekst);
        pos = pos + 128;
        i = i + 1;
    }

    atawritesectors(VFSDISKLBA, VFSDISKSECTORS, sklad);
}

static int findindex(const char* path)
{
    int i = 0;
    while (i < skolko_failov)
    {
        if (stroki_odinakovie(failiki[i].put, path))
        {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

void vfs_init()
{
    if (!vfsloadfromdisk())
    {
        vfssetdefaults();
        vfssavetodisk();
    }
}

int vfs_count()
{
    return skolko_failov;
}

const char* vfs_path_at(int index)
{
    if (index < 0 || index >= skolko_failov)
    {
        return 0;
    }
    return failiki[index].put;
}

int vfs_read(const char* path, char* out, int maxlen)
{
    int idx = findindex(path);
    int i = 0;

    if (idx < 0 || maxlen <= 0)
    {
        return 0;
    }

    while (i < (maxlen - 1) && failiki[idx].tekst[i])
    {
        out[i] = failiki[idx].tekst[i];
        i = i + 1;
    }
    out[i] = 0;
    return 1;
}

int vfs_write(const char* path, const char* text)
{
    int idx = findindex(path);

    if (!strfits31(path) || !strfits127(text) || strlen31(path) == 0)
    {
        return 0;
    }

    if (idx < 0)
    {
        if (skolko_failov >= 8)
        {
            return 0;
        }

        idx = skolko_failov;
        skolko_failov = skolko_failov + 1;
        strcopy31(failiki[idx].put, path);
    }

    strcopy127(failiki[idx].tekst, text);
    vfssavetodisk();
    return 1;
}
