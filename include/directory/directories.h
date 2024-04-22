#ifndef DIRECTORIES_H
#define DIRECTORIES_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../config/flash_config.h"    
#include "../filesystem/filesystem.h" 


typedef struct {
    char name[256];       // Name of the file or directory
    uint32_t parentDirId;    // ID of the parent directory
    uint32_t currentDirId; 
    bool is_directory;    // Flag to indicate if this is a directory
    uint32_t size;        // Size (for files) or number of entries (for directories)
    uint32_t start_block; // Start block in flash memory
    bool in_use;  
    char buffer[1256];  
} DirectoryEntry;


bool Dir_remove(const char* directoryName);
bool initialize_directory_block(uint32_t block_number);
bool fs_create_directory(const char* directory, const char* parentPath);         
bool fs_remove_directory(const char* path);
bool actual_create_directory(const char* path, uint32_t pathToParent);
bool reset_root_directory(void);
bool is_directory_valid(const DirectoryEntry* directoryEntry);

void split_path_into_parent_and_dir(const char* fullPath, char* parentPath, char* dirName) ;
 
// int split_path(const char* fullPath, char* directoryPath, char* fileName);
FS_FILE* split_path(const char* fullPath, char* directoryPath, char* fileName);

DirectoryEntry* DIR_find_directory_entry(const char* directoryName);
DirectoryEntry* find_directory_entry(const char* path);
DirectoryEntry* fs_list_directory(const char* path);
DirectoryEntry* find_free_directory_entry(void);
DirectoryEntry* DIR_all_directory_entries(void);

 
 
 void fs_list_all();
#endif // DIRECTORIES_H

 