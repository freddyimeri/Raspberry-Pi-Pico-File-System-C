 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/mutex.h"
#include <ctype.h>
 
#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
 
#include "../filesystem/filesystem_helper.h"  

void process_file_creation(const char* path) {
    printf("Attempting to process file creation for: %s\n", path);
    FileEntry* fileEntry = FILE_find_file_entry(path);
    
    if (fileEntry) {
        printf("File '%s' already exists. No need to create.\n", path);
        free(fileEntry); // Assuming fileEntry was dynamically allocated
    } else {
        printf("File '%s' does not exist. Creating file...\n", path);
        FS_FILE* fileHandle = fs_open(path, "w");
        if (fileHandle) {
            printf("File '%s' created and opened successfully.\n", path);
            // fs_close(fileHandle); // Uncomment if you are handling file closing elsewhere
        } else {
            printf("Failed to create and open file '%s'.\n", path);
        }
    }
}

PathParts extract_last_two_parts(const char* fullPath) {
    PathParts parts;
    memset(&parts, 0, sizeof(parts));

    printf("Attempting to extract last two parts from: %s\n", fullPath);
    if (fullPath == NULL) {
        printf("Input path is NULL.\n");
        return parts; // Return empty parts if the input is NULL
    }

    // Use strrchr to find the last occurrence of '/'
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash) { // If there's no '/', the path is just a filename
        printf("Only a filename provided without directory path.\n");
        strcpy(parts.filename, fullPath); // Assume the entire path is the filename
        printf("Extracted file name: %s\n", parts.filename);
        return parts;
    }

    // If we found a '/', copy the filename after the '/'
    strcpy(parts.filename, lastSlash + 1);
    printf("Extracted file name: %s\n", parts.filename);

    // To find the directory just before the filename:
    // We make a copy of the path to manipulate
    char pathCopy[256];
    strncpy(pathCopy, fullPath, lastSlash - fullPath);
    pathCopy[lastSlash - fullPath] = '\0'; // Null-terminate at the last '/'

    // Now find the last '/' in the modified copy
    const char* secondLastSlash = strrchr(pathCopy, '/');
    if (secondLastSlash) {
        strcpy(parts.directory, secondLastSlash + 1); // Copy the directory name
    } else {
        strcpy(parts.directory, pathCopy); // If no second '/', the directory is the whole modified path
    }

    printf("Extracted directory: %s\n", parts.directory);
    return parts;
}