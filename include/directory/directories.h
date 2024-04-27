#ifndef DIRECTORIES_H
#define DIRECTORIES_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../config/flash_config.h"    

// note in order to remove a dir, you need to call fs_format() to remove all the files in the directory



typedef struct {
    char name[256];       // Name of the file or directory
    uint32_t parentDirId;    // ID of the parent directory
    uint32_t currentDirId; 
    bool is_directory;    // Flag to indicate if this is a directory
    uint32_t size;        // number of entries (for directories)
    uint32_t start_block; // Start block in flash memory
    bool in_use;  
    // char buffer[1256];  
} DirectoryEntry;





extern DirectoryEntry dirEntries[MAX_DIRECTORY_ENTRIES];






void init_directory_entries();
bool fs_create_directory(const char* directory);
bool reset_root_directory(void);
 

 
#endif // DIRECTORIES_H

 