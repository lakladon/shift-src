#include "vfs.h"
#include "stroki.h"
#include "iozh.h"

typedef struct
{
    char path[32];
    char data[128];
} VfsFile;

static VfsFile g_files[8];
static int g_count = 0;
static unsigned char g_store[4096];

#define ATA_IO_BASE 0x1F0
#define VFS_DISK_LBA 0
#define VFS_DISK_SECTORS 8

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

static int str_fits31(const char* s)
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

static int str_fits127(const char* s)
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

static void mem_zero(unsigned char* p, int n)
{
    int i = 0;
    while (i < n)
    {
        p[i] = 0;
        i = i + 1;
    }
}

static int ata_wait_not_bsy()
{
    int t = 0;
    while (t < 100000)
    {
        unsigned char st = zhmyak_in(ATA_IO_BASE + 7);
        if ((st & 0x80) == 0)
        {
            return 1;
        }
        t = t + 1;
    }
    return 0;
}

static int ata_wait_drq()
{
    int t = 0;
    while (t < 100000)
    {
        unsigned char st = zhmyak_in(ATA_IO_BASE + 7);
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

static int ata_read_sectors(unsigned int lba, unsigned char count, unsigned char* out)
{
    unsigned int s = 0;

    if (count == 0)
    {
        return 1;
    }

    while (s < count)
    {
        int i = 0;

        if (!ata_wait_not_bsy())
        {
            return 0;
        }

        zhmyak_out(ATA_IO_BASE + 6, (unsigned char)(0xE0 | ((lba >> 24) & 0x0F)));
        zhmyak_out(ATA_IO_BASE + 2, 1);
        zhmyak_out(ATA_IO_BASE + 3, (unsigned char)(lba & 0xFF));
        zhmyak_out(ATA_IO_BASE + 4, (unsigned char)((lba >> 8) & 0xFF));
        zhmyak_out(ATA_IO_BASE + 5, (unsigned char)((lba >> 16) & 0xFF));
        zhmyak_out(ATA_IO_BASE + 7, 0x20);

        if (!ata_wait_drq())
        {
            return 0;
        }

        while (i < 256)
        {
            unsigned short w = zhmyak_in16(ATA_IO_BASE + 0);
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

static int ata_write_sectors(unsigned int lba, unsigned char count, const unsigned char* in)
{
    unsigned int s = 0;

    if (count == 0)
    {
        return 1;
    }

    while (s < count)
    {
        int i = 0;

        if (!ata_wait_not_bsy())
        {
            return 0;
        }

        zhmyak_out(ATA_IO_BASE + 6, (unsigned char)(0xE0 | ((lba >> 24) & 0x0F)));
        zhmyak_out(ATA_IO_BASE + 2, 1);
        zhmyak_out(ATA_IO_BASE + 3, (unsigned char)(lba & 0xFF));
        zhmyak_out(ATA_IO_BASE + 4, (unsigned char)((lba >> 8) & 0xFF));
        zhmyak_out(ATA_IO_BASE + 5, (unsigned char)((lba >> 16) & 0xFF));
        zhmyak_out(ATA_IO_BASE + 7, 0x30);

        if (!ata_wait_drq())
        {
            return 0;
        }

        while (i < 256)
        {
            unsigned short w = (unsigned short)(in[0] | ((unsigned short)in[1] << 8));
            zhmyak_out16(ATA_IO_BASE + 0, w);
            in = in + 2;
            i = i + 1;
        }

        lba = lba + 1;
        s = s + 1;
    }

    if (!ata_wait_not_bsy())
    {
        return 0;
    }

    zhmyak_out(ATA_IO_BASE + 7, 0xE7);
    return ata_wait_not_bsy();
}

static void vfs_set_defaults()
{
    g_count = 3;

    str_copy31(g_files[0].path, "/hello.txt");
    str_copy127(g_files[0].data, "Hello from Shift VFS");

    str_copy31(g_files[1].path, "/about.txt");
    str_copy127(g_files[1].data, "Shift OS in-memory virtual filesystem");

    str_copy31(g_files[2].path, "/notes.txt");
    str_copy127(g_files[2].data, "Use: ls, cat <path>, write <path> <text>");
}

static int vfs_load_from_disk()
{
    int i;
    int pos;

    if (!ata_read_sectors(VFS_DISK_LBA, VFS_DISK_SECTORS, g_store))
    {
        return 0;
    }

    if (!(g_store[0] == 'S' && g_store[1] == 'V' && g_store[2] == 'F' && g_store[3] == '1'))
    {
        return 0;
    }

    if (g_store[4] > 8)
    {
        return 0;
    }

    g_count = g_store[4];
    pos = 5;

    i = 0;
    while (i < g_count)
    {
        str_copy31(g_files[i].path, (const char*)(g_store + pos));
        pos = pos + 32;
        str_copy127(g_files[i].data, (const char*)(g_store + pos));
        pos = pos + 128;

        if (!str_fits31(g_files[i].path) || !str_fits127(g_files[i].data))
        {
            return 0;
        }

        i = i + 1;
    }

    return 1;
}

static void vfs_save_to_disk()
{
    int i = 0;
    int pos = 5;

    mem_zero(g_store, 4096);
    g_store[0] = 'S';
    g_store[1] = 'V';
    g_store[2] = 'F';
    g_store[3] = '1';
    g_store[4] = (unsigned char)g_count;

    while (i < g_count)
    {
        str_copy31((char*)(g_store + pos), g_files[i].path);
        pos = pos + 32;
        str_copy127((char*)(g_store + pos), g_files[i].data);
        pos = pos + 128;
        i = i + 1;
    }

    ata_write_sectors(VFS_DISK_LBA, VFS_DISK_SECTORS, g_store);
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
    if (!vfs_load_from_disk())
    {
        vfs_set_defaults();
        vfs_save_to_disk();
    }
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

    if (!str_fits31(path) || !str_fits127(text) || str_len31(path) == 0)
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
    vfs_save_to_disk();
    return 1;
}
