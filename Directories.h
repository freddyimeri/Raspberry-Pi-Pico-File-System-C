#ifndef DIRECTORIES_H
#define DIRECTORIES_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "flash_config.h"
#include "filesystem.h"     
 

typedef struct {
    char name[256];       // Name of the file or directory
    bool is_directory;    // Flag to indicate if this is a directory
    uint32_t size;        // Size (for files) or number of entries (for directories)
    uint32_t start_block; // Start block in flash memory
    bool in_use;  
} DirectoryEntry;



bool initialize_directory_block(uint32_t block_number);
bool fs_create_directory(const char* path);             
DirectoryEntry* fs_list_dir_entries(const char* path);
bool fs_remove_directory(const char* path);
DirectoryEntry* create_new_directory_entry(const char* path);
bool actual_create_directory(const char* path);
bool add_directory_entry_to_filesystem(const char* path, uint32_t newDirBlock);
bool directory_exists(const char* path);
DirectoryEntry* find_directory_entry(const char* path);
uint8_t* ensure_directory_has_space(DirectoryEntry* directory);
bool add_entry_to_directory_block(uint8_t* dirBlock, DirectoryEntry* newEntry);
bool reset_root_directory(void);
void split_path_into_parent_and_dir(const char* fullPath, char* parentPath, char* dirName) ;
DirectoryEntry* fs_list_directory(const char* path);
void read_directory_block(uint32_t block_number, uint8_t *buffer, size_t buffer_size);
int get_directory_entry_count(const uint8_t* blockBuffer);
bool is_directory_valid(const DirectoryEntry* directoryEntry);
DirectoryEntry* find_free_directory_entry(void);
 void populate_directory_entries_from_block(DirectoryEntry* dir, DirectoryEntry* entries);
 void list_directory_contents(const char* path, int indentLevel);
 void fs_list_all();
#endif // DIRECTORIES_H

 