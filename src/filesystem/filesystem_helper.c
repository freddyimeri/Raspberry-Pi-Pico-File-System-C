 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/mutex.h"
#include <ctype.h>
 

 
#include "hardware/adc.h"
#include <stdint.h>



#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
 
#include "../filesystem/filesystem_helper.h" 



static int random_initialized = 0;  // Flag to check if random generator has been initialized


FS_FILE* process_file_creation(const char* path) {
    printf("Attempting to process file creation for: %s\n", path);
    FS_FILE* fileHandle = fs_open(path, "w");
    if (fileHandle) {
        printf("File '%s' created and opened successfully.\n", path);
        return fileHandle; // Return the file handle if successful
    } else {
        printf("Failed to create and open file '%s'.\n", path);
        return NULL; // Return NULL on failure
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




char* prepend_slash(const char* path) {
    if (path == NULL) {
        return NULL;
    }

    // Check if the path already starts with a '/'
    if (path[0] == '/') {
        return strdup(path); // Simply duplicate the path if it already starts with '/'
    }

    // Allocate memory for the new path (length of original path + 1 for the slash + 1 for the null terminator)
    char* newPath = (char*)malloc(strlen(path) + 2);
    if (newPath == NULL) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Construct the new path
    newPath[0] = '/';
    strcpy(newPath + 1, path);

    return newPath; // Return the newly created path with a leading '/'
}



int find_file_entry_by_name(const char* filename) {
    const char* path = prepend_slash(filename);
    if (path == NULL) {
        printf("Error: Filename is NULL.\n");
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, path) == 0) {
            printf("File found: %s at index %d\n", path, i);
            return i;
        }
    }

    printf("File not found: %s\n", path);
    return -1; // File not found
}





// Function to initialize random number generation
void init_random() {
    if (!random_initialized) {
        adc_init();
        adc_select_input(0);  // Select an unconnected ADC input for noise

        // Read an analog value for seeding the random number generator
        uint16_t noise = adc_read();
        srand(noise);

        random_initialized = 1;  // Set the initialized flag
    }
}

// Function to generate a unique number between 1000 and 4,294,967,294 (one less than uint32_t max)
uint32_t generateUniqueId() {
    if (!random_initialized) {
        init_random();  // Ensure random number generator is initialized
    }
    // Calculate a range from 1000 to 4,294,967,294
    uint32_t range = 4294967294u - 1000 + 1;
    return rand() % range + 1000;
}







int find_file_entry_by_unique_file_id(uint32_t unique_file_id) {

    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && fileSystem[i].unique_file_id == unique_file_id) {
            printf("File found at index %d\n", i);
            
            return i;
        }
    }

    printf("File not found:\n");
    return -1; // File not found
}












// void process_file_creation(const char* path) {
//     printf("Attempting to process file creation for: %s\n", path);
//     FileEntry* fileEntry = FILE_find_file_entry(path);
    
//     if (fileEntry) {
//         printf("File '%s' already exists. No need to create.\n", path);
//         free(fileEntry); // Assuming fileEntry was dynamically allocated
//     } else {
//         printf("File '%s' does not exist. Creating file...\n", path);
//         FS_FILE* fileHandle = fs_open(path, "w");
//         printf("hiiiiiiii");
//         printf("File handle: %p\n", fileHandle);
//         if (fileHandle) {
//             printf("File '%s' created and opened successfully.\n", path);
//             // fs_close(fileHandle); // Uncomment if you are handling file closing elsewhere
//         } else {
//             printf("Failed to create and open file '%s'.\n", path);
//         }
//     }
// }


// FS_FILE* process_file_creation(const char* path) {
//     printf("Attempting to process file creation for: %s\n", path);
//     FileEntry* fileEntry = FILE_find_file_entry(path);
    
//     if (fileEntry) {
//         printf("File '%s' already exists. No need to create.\n", path);
//         free(fileEntry); // Assuming fileEntry was dynamically allocated
//         return NULL; // Return NULL or you could return an existing handle if appropriate
//     } else {
//         printf("File '%s' does not exist. Creating file...\n", path);
//         FS_FILE* fileHandle = fs_open(path, "w");
//         printf("File handle: %p\n", fileHandle);
//         if (fileHandle) {
//             printf("File '%s' created and opened successfully.\n", path);
//             return fileHandle; // Return the handle to the newly opened file
//         } else {
//             printf("Failed to create and open file '%s'.\n", path);
//             return NULL; // Return NULL to indicate failure
//         }
//     }
// }
