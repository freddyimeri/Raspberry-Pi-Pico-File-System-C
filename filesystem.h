#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_FILES 10  // Maximum number of files in the filesystem

// File entry structure
typedef struct {
    char filename[256];     // Path of the file
    uint32_t size;      // Size of the file
    bool in_use;        // Indicates if this file entry is in use
} FileEntry;

// File handle structure
typedef struct {
    FileEntry *entry;   // Pointer to the file entry in the file system
    uint32_t position;  // Current position in the file
} FS_FILE;

FS_FILE* fs_open(const char* path, const char* mode);
void fs_close(FS_FILE* file);
int fs_read(FS_FILE* file, void* buffer, int size);
int fs_write(FS_FILE* file, const void* buffer, int size);
int fs_seek(FS_FILE* file, long offset, int whence);

//int fs_format(const char* path);
//int fs_wipe(const char* path);
//int fs_mv(const char* old_path, const char* new_path);
//int fs_cp(const char* source_path, const char* dest_path);
//int fs_rm(const char* path);

#endif // FILESYSTEM_H