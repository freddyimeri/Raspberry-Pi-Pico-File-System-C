#ifndef FILESYSTEM_HELPER_H
#define FILESYSTEM_H


#include "../filesystem/filesystem.h" 
#include "../config/flash_config.h"    



typedef struct {
    char directory[256];
    char filename[256];
} PathParts;




void process_file_creation(const char* path);


PathParts extract_last_two_parts(const char* fullPath);

#endif // FILESYSTEM_HELPER_H