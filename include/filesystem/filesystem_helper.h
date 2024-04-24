#ifndef FILESYSTEM_HELPER_H
#define FILESYSTEM_H


#include "../filesystem/filesystem.h" 
#include "../config/flash_config.h"    



typedef struct {
    char directory[256];
    char filename[256];
} PathParts;




// void process_file_creation(const char* path);
FS_FILE* process_file_creation(const char* path);
char* prepend_slash(const char* path);

PathParts extract_last_two_parts(const char* fullPath);
int find_file_entry_by_name(const char* filename);
int find_file_entry_by_unique_file_id(uint32_t unique_file_id);
uint32_t generateUniqueId();
#endif // FILESYSTEM_HELPER_H