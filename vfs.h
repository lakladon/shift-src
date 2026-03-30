#ifndef VFS_H
#define VFS_H

void vfs_init();
int vfs_count();
const char* vfs_path_at(int index);
int vfs_type_at(int index);
int vfs_read(const char* path, char* out, int max_len);
int vfs_write(const char* path, const char* text);
int vfs_mkdir(const char* path);
int vfs_is_dir(const char* path);
int vfs_meta(const char* path, unsigned int* razmer, unsigned int* sozdan, unsigned int* izmenen);
int vfs_disk_connected(char disk);

#endif