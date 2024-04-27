//directory_helpers.h
#ifndef DIRECTORIES_HELPERS_H
#define DIRECTORIES_HELPERS_H


#include "../filesystem/filesystem.h" 
#include "../config/flash_config.h"    


typedef struct PathSegments {
    char** segments;     // Array of string pointers to hold each segment
    size_t count;        // Number of segments
} PathSegments;


 

// Define a structure to hold the parsed directory and file name


 
DirectoryEntry* createDirectoryEntry(const char* path);



uint32_t get_root_directory_id();


bool is_directory_valid(const DirectoryEntry* directoryEntry);

void DIR_all_directory_entriesEX(void);
DirectoryEntry* DIR_find_directory_entry(const char* directoryName);
DirectoryEntry* find_directory_entry(const char* path); // been used once in directories.c fs_create_directory

DirectoryEntry* find_free_directory_entry(void);
void DIR_all_directory_entries(void);


 
#endif // FILESYSTEM_HELPER_H