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


void process_directory_operation(const char* path);

PathSegments extract_path_segments(const char* fullPath);
void free_path_segments(PathSegments* pathSegments);

#endif // FILESYSTEM_HELPER_H