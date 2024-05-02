#ifndef FILESYSTEM_HELPER_H
#define FILESYSTEM_HELPER_H


#include "../filesystem/filesystem.h" 
#include "../config/flash_config.h"    



typedef struct {
    char directory[256];
    char filename[256];
} PathParts;


void set_default_path(char* path, const char* default_path);
void appendCopyToFilename(char *filename);
void construct_full_path(const char* directory, const char* filename, char* full_path, size_t max_size);
 
 
void prepend_slash(const char* path, char* buffer, size_t buffer_size);
PathParts extract_last_two_parts(const char* fullPath);
int find_file_entry_by_name(const char* filename);
int find_file_existance(const char* filename,  uint32_t parentID );

int find_file_entry_by_unique_file_id(uint32_t unique_file_id);
uint32_t generateUniqueId();
FileEntry* createFileEntry(const char* path,  uint32_t parentID );
void reset_file_content(FileEntry* entry);

FileEntry* FILE_find_file_entry(const char* filename,uint32_t parentID);

void saveFileEntriesToFileSystem();
void loadFileEntriesFromFileSystem();
#endif // FILESYSTEM_HELPER_H