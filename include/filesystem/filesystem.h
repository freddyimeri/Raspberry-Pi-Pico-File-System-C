#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../config/flash_config.h"    

 
extern bool fs_initialized;
 

typedef enum {
    MODE_READ = 1, // Read mode
    MODE_WRITE,    // Write mode
    MODE_APPEND    // Append mode
} FileMode;

// File entry structure
typedef struct {
    char filename[256];     // Path of the file
    uint32_t parentDirId;   // ID of the parent directory
    uint32_t size;      // Size of the file
    bool in_use;        // Indicates if this file entry is in use
    uint32_t start_block; // Offset in flash memory where the file data starts
    bool is_directory;      // Flag to indicate if this entry is a
    uint8_t buffer[256];
    uint32_t unique_file_id; 
} FileEntry;

// File handle structure
typedef struct {
    FileEntry *entry;   // Pointer to the file entry in the file system
    uint32_t position;  // Current position in the file
    // FileMode mode; 
    char mode;
} FS_FILE;

extern FileEntry fileSystem[MAX_FILES];

 void fs_init(void);
FS_FILE* fs_open(const char* path, const char* mode);
void fs_close(FS_FILE* file);
int fs_read(FS_FILE* file, void* buffer, int size);
int fs_write(FS_FILE* file, const void* buffer, int size);
int fs_seek(FS_FILE* file, long offset, int whence);
int fs_mv(const char* old_path, const char* new_path);
int fs_wipe(const char* path);
int fs_format(const char* path);
int fs_cp(const char* source_path, const char* dest_path);
int fs_rm(const char* path);

#endif // FILESYSTEM_H



