#ifndef VFS_H
#define VFS_H

void vfs_init();
int vfs_count();
const char* vfs_path_at(int index);
int vfs_read(const char* path, char* out, int max_len);
int vfs_write(const char* path, const char* text);
// Retrieve metadata timestamps (seconds since boot) for a file.
// Returns 1 on success, 0 if file not found.
int vfs_get_metadata(const char* path, unsigned int* created, unsigned int* modified);

#endif