#include "vfs.h"
#include "stroki.h"
#include "iozh.h"
#include "timerka.h"

typedef struct
{
    char put[32];
    char tekst[128];
    unsigned char tip;
    unsigned int razmer;
    unsigned int sozdan;
    unsigned int izmenen;
} VfsFile;

static VfsFile failiki[8];
static int skolko_failov = 0;
static unsigned char sklad[4096];
static int disk_connected_a = 0;
static int disk_connected_b = 0;
static unsigned char storage_drive = 0;

#define ATAIOBASE 0x1F0
#define VFSDISKLBA 128
#define VFSDISKSECTORS 8
#define VFSTYPEFILE 0
#define VFSTYPEDIR 1

static int findindex(const char* path);

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

static void u32write(unsigned char* p, unsigned int v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static unsigned int u32read(const unsigned char* p)
{
    return ((unsigned int)p[0]) |
           (((unsigned int)p[1]) << 8) |
           (((unsigned int)p[2]) << 16) |
           (((unsigned int)p[3]) << 24);
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

static int ataidentify(unsigned char drive)
{
    int i;
    unsigned char st;
    unsigned char cl;
    unsigned char ch;

    if (!atawaitnotbsy())
    {
        return 0;
    }

    zhmyak_out(ATAIOBASE + 6, (unsigned char)(0xE0 | ((drive & 1) << 4)));
    zhmyak_out(ATAIOBASE + 2, 0);
    zhmyak_out(ATAIOBASE + 3, 0);
    zhmyak_out(ATAIOBASE + 4, 0);
    zhmyak_out(ATAIOBASE + 5, 0);
    zhmyak_out(ATAIOBASE + 7, 0xEC);

    st = zhmyak_in(ATAIOBASE + 7);
    if (st == 0)
    {
        return 0;
    }

    i = 0;
    while (i < 100000)
    {
        st = zhmyak_in(ATAIOBASE + 7);
        if (st & 0x01)
        {
            return 0;
        }
        if (st & 0x08)
        {
            break;
        }
        i = i + 1;
    }

    if (i >= 100000)
    {
        return 0;
    }

    cl = zhmyak_in(ATAIOBASE + 4);
    ch = zhmyak_in(ATAIOBASE + 5);
    if (cl != 0 || ch != 0)
    {
        return 0;
    }

    i = 0;
    while (i < 256)
    {
        (void)zhmyak_in16(ATAIOBASE + 0);
        i = i + 1;
    }

    return 1;
}

static int atareadsectors(unsigned char drive, unsigned int lba, unsigned char count, unsigned char* out)
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

        zhmyak_out(ATAIOBASE + 6, (unsigned char)(0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F)));
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

static int atawritesectors(unsigned char drive, unsigned int lba, unsigned char count, const unsigned char* in)
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

        zhmyak_out(ATAIOBASE + 6, (unsigned char)(0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F)));
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
        failiki[i].tip = VFSTYPEFILE;
        failiki[i].sozdan = 0;
        failiki[i].izmenen = 0;
        i = i + 1;
    }
}

static int has_disk_prefix(const char* p)
{
    return p[0] && p[1] == ':' && p[2] == '/';
}

static int is_valid_path(const char* path)
{
    int i = 0;

    if (!has_disk_prefix(path))
    {
        return 0;
    }

    while (path[i])
    {
        if (path[i] == ' ')
        {
            return 0;
        }
        i = i + 1;
    }

    return 1;
}

static int is_root_path(const char* path)
{
    return has_disk_prefix(path) && path[3] == 0;
}

static int parent_dir_exists(const char* path)
{
    int i = 0;
    int last_slash = -1;
    char parent[32];
    int j;
    int idx;

    if (!has_disk_prefix(path))
    {
        return 0;
    }

    while (path[i])
    {
        if (path[i] == '/')
        {
            last_slash = i;
        }
        i = i + 1;
    }

    if (last_slash <= 2)
    {
        return 1;
    }

    j = 0;
    while (j < last_slash && j < 31)
    {
        parent[j] = path[j];
        j = j + 1;
    }
    parent[j] = 0;

    idx = findindex(parent);
    if (idx < 0)
    {
        return 0;
    }

    return failiki[idx].tip == VFSTYPEDIR;
}

static int is_disk_letter_connected(char disk)
{
    if (disk >= 'a' && disk <= 'z')
    {
        disk = (char)(disk - ('a' - 'A'));
    }

    if (disk == 'A')
    {
        return disk_connected_a;
    }
    if (disk == 'B')
    {
        return disk_connected_b;
    }

    return 0;
}

static int path_disk_connected(const char* path)
{
    if (!has_disk_prefix(path))
    {
        return 0;
    }

    return is_disk_letter_connected(path[0]);
}

static int vfsloadfromdisk()
{
    int i;
    int pos;
    int v4;

    if (!atareadsectors(storage_drive, VFSDISKLBA, VFSDISKSECTORS, sklad))
    {
        return 0;
    }

    if (!(sklad[0] == 'S' && sklad[1] == 'V' && sklad[2] == 'F' && (sklad[3] == '3' || sklad[3] == '4')))
    {
        return 0;
    }

    v4 = sklad[3] == '4';

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
        if (v4)
        {
            failiki[i].tip = sklad[pos];
            pos = pos + 1;
        }
        else
        {
            failiki[i].tip = VFSTYPEFILE;
        }
        strcopy127(failiki[i].tekst, (const char*)(sklad + pos));
        pos = pos + 128;
        failiki[i].razmer = u32read(sklad + pos);
        pos = pos + 4;
        failiki[i].sozdan = u32read(sklad + pos);
        pos = pos + 4;
        failiki[i].izmenen = u32read(sklad + pos);
        pos = pos + 4;

        if (!strfits31(failiki[i].put) || !strfits127(failiki[i].tekst))
        {
            return 0;
        }

        if (!(failiki[i].tip == VFSTYPEFILE || failiki[i].tip == VFSTYPEDIR))
        {
            return 0;
        }

        if (failiki[i].razmer > 127)
        {
            failiki[i].razmer = (unsigned int)strlen127(failiki[i].tekst);
        }

        if (failiki[i].tip == VFSTYPEDIR)
        {
            failiki[i].tekst[0] = 0;
            failiki[i].razmer = 0;
        }

        i = i + 1;
    }

    return 1;
}

static void vfssavetodisk()
{
    int i = 0;
    int pos = 5;

    if (!disk_connected_a && !disk_connected_b)
    {
        return;
    }

    memzero(sklad, 4096);
    sklad[0] = 'S';
    sklad[1] = 'V';
    sklad[2] = 'F';
    sklad[3] = '4';
    sklad[4] = (unsigned char)skolko_failov;

    while (i < skolko_failov)
    {
        strcopy31((char*)(sklad + pos), failiki[i].put);
        pos = pos + 32;
        sklad[pos] = failiki[i].tip;
        pos = pos + 1;
        strcopy127((char*)(sklad + pos), failiki[i].tekst);
        pos = pos + 128;
        u32write(sklad + pos, failiki[i].razmer);
        pos = pos + 4;
        u32write(sklad + pos, failiki[i].sozdan);
        pos = pos + 4;
        u32write(sklad + pos, failiki[i].izmenen);
        pos = pos + 4;
        i = i + 1;
    }

    atawritesectors(storage_drive, VFSDISKLBA, VFSDISKSECTORS, sklad);
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

static unsigned int vfspoxixtime()
{
    unsigned int t = rtc_unix_time();
    if (t == 0)
    {
        t = 946684800u + uptime_sec();
    }
    return t;
}

void vfs_init()
{
    disk_connected_a = ataidentify(0);
    disk_connected_b = ataidentify(1);

    if (!disk_connected_a && !disk_connected_b)
    {
        vfssetdefaults();
        return;
    }

    storage_drive = disk_connected_a ? 0 : 1;

    if (!atareadsectors(storage_drive, VFSDISKLBA, 1, sklad))
    {
        vfssetdefaults();
        return;
    }

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

    if (idx < 0 || maxlen <= 0 || failiki[idx].tip == VFSTYPEDIR || !path_disk_connected(path))
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
    unsigned int now;

    if (!disk_connected_a && !disk_connected_b)
    {
        return 0;
    }

    if (!strfits31(path) || !strfits127(text) || strlen31(path) == 0 || !is_valid_path(path) || is_root_path(path) || !path_disk_connected(path))
    {
        return 0;
    }

    if (!parent_dir_exists(path))
    {
        return 0;
    }

    now = vfspoxixtime();

    if (idx < 0)
    {
        if (skolko_failov >= 8)
        {
            return 0;
        }

        idx = skolko_failov;
        skolko_failov = skolko_failov + 1;
        strcopy31(failiki[idx].put, path);
        failiki[idx].tip = VFSTYPEFILE;
        failiki[idx].sozdan = now;
    }
    else if (failiki[idx].tip == VFSTYPEDIR)
    {
        return 0;
    }

    failiki[idx].razmer = (unsigned int)strlen127(text);
    failiki[idx].izmenen = now;
    strcopy127(failiki[idx].tekst, text);
    vfssavetodisk();
    return 1;
}

int vfs_mkdir(const char* path)
{
    int idx = findindex(path);
    unsigned int now;

    if (!disk_connected_a && !disk_connected_b)
    {
        return 0;
    }

    if (!strfits31(path) || strlen31(path) == 0 || !is_valid_path(path) || is_root_path(path) || !path_disk_connected(path))
    {
        return 0;
    }

    if (!parent_dir_exists(path))
    {
        return 0;
    }

    now = vfspoxixtime();

    if (idx >= 0)
    {
        if (failiki[idx].tip == VFSTYPEDIR)
        {
            return 1;
        }
        return 0;
    }

    if (skolko_failov >= 8)
    {
        return 0;
    }

    idx = skolko_failov;
    skolko_failov = skolko_failov + 1;
    strcopy31(failiki[idx].put, path);
    failiki[idx].tip = VFSTYPEDIR;
    failiki[idx].tekst[0] = 0;
    failiki[idx].razmer = 0;
    failiki[idx].sozdan = now;
    failiki[idx].izmenen = now;
    vfssavetodisk();
    return 1;
}

int vfs_is_dir(const char* path)
{
    int idx = findindex(path);
    if (idx < 0 || !path_disk_connected(path))
    {
        return 0;
    }
    return failiki[idx].tip == VFSTYPEDIR;
}

int vfs_type_at(int index)
{
    if (index < 0 || index >= skolko_failov)
    {
        return -1;
    }
    return failiki[index].tip;
}

int vfs_meta(const char* path, unsigned int* razmer, unsigned int* sozdan, unsigned int* izmenen)
{
    int idx = findindex(path);

    if (idx < 0 || !path_disk_connected(path))
    {
        return 0;
    }

    if (razmer)
    {
        *razmer = failiki[idx].razmer;
    }
    if (sozdan)
    {
        *sozdan = failiki[idx].sozdan;
    }
    if (izmenen)
    {
        *izmenen = failiki[idx].izmenen;
    }

    return 1;
}

int vfs_disk_connected(char disk)
{
    return is_disk_letter_connected(disk);
}
