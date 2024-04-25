 
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
#include "../directory/directory_helpers.h"


static int random_initialized = 0;  // Flag to check if random generator has been initialized







char* prepend_slash(const char* path) {
    // Static buffer for the new path, assuming a reasonable max path length
    static char newPath[1024];

    if (path == NULL) {
        return NULL;
    }

    // Check if the path already starts with a '/'
    if (path[0] == '/') {
        strncpy(newPath, path, sizeof(newPath) - 1);
        newPath[sizeof(newPath) - 1] = '\0';  // Ensure null termination
        return newPath;
    }

    // Ensure the total length doesn't exceed the buffer size minus two for slash and null terminator
    if (strlen(path) + 1 >= sizeof(newPath)) {
        return NULL; // Return NULL if the path is too long to fit in the buffer
    }

    // Prepend the slash and copy the original path
    newPath[0] = '/';
    strcpy(newPath + 1, path);

    return newPath;
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





FS_FILE* process_file_creation(const char* path) {
    printf("Attempting to process file creation for: %s\n", path);
    FileEntry* fileEntry = FILE_find_file_entry(path);
    
    if (fileEntry) {
        printf("File '%s' already exists. No need to create.\n", path);
        free(fileEntry); // Assuming fileEntry was dynamically allocated
        return NULL; // Return NULL or you could return an existing handle if appropriate
    } else {
        printf("File '%s' does not exist. Creating file...\n", path);
        FS_FILE* fileHandle = fs_open(path, "w");
        printf("File handle: %p\n", fileHandle);
        if (fileHandle) {
            printf("File '%s' created and opened successfully.\n", path);
            return fileHandle; // Return the handle to the newly opened file
        } else {
            printf("Failed to create and open file '%s'.\n", path);
            return NULL; // Return NULL to indicate failure
        }
    }
}



/**
 * Creates a new file entry in the filesystem.
 * @param path The path or name of the file.
 * @return Pointer to the created FileEntry or NULL if creation fails.
 */
FileEntry* createFileEntry(const char* path) {
    printf("debug createFileEntry for path: %s\n", path);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fileSystem[i].in_use) {
            uint32_t rootDirId = get_root_directory_id();
            strncpy(fileSystem[i].filename, path, sizeof(fileSystem[i].filename) - 1);
            fileSystem[i].filename[sizeof(fileSystem[i].filename) - 1] = '\0';
            fileSystem[i].in_use = true;
            fileSystem[i].is_directory = false; // Default to file
            fileSystem[i].size = 0;
            fileSystem[i].parentDirId = rootDirId;
            fileSystem[i].start_block = fat_allocate_block();
            fileSystem[i].unique_file_id = generateUniqueId();

            printf("New file created: %s\n", fileSystem[i].filename);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("File size: %u\n", fileSystem[i].size);
            printf("Filesystem entry index: %d\n", i);
            printf("Parent Directory ID: %u\n", fileSystem[i].parentDirId);
            
            fflush(stdout);
            if (fileSystem[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left on device to create new file.\n");
                fflush(stdout);
                memset(&fileSystem[i], 0, sizeof(FileEntry)); // Cleanup
                fileSystem[i].in_use = false; // Explicitly mark it as not in use
                return NULL;
            }
            return &fileSystem[i];
        }
    }
    printf("Error: Filesystem is full, cannot create new file.\n");
    fflush(stdout);
    return NULL;
}
 





void reset_file_content(FileEntry* entry) {
    if (entry == NULL) {
        printf("Error: NULL entry provided to reset_file_content.\n");
        return;
    }

    uint32_t currentBlock = entry->start_block;
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        uint32_t nextBlock;
        int result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error: Failed to free block %u.\n", currentBlock);
            break;
        }
        fat_free_block(currentBlock);
        if (nextBlock == FAT_ENTRY_END) break;
        currentBlock = nextBlock;
    }

    entry->start_block = fat_allocate_block();
    if (entry->start_block == FAT_NO_FREE_BLOCKS) {
        printf("Error: No free blocks available to allocate.\n");
        entry->size = 0;
        return;
    }
    entry->size = 0;
    printf("File content reset successfully. New start block: %u, Size reset to 0.\n", entry->start_block);
}



 //updated
FileEntry* FILE_find_file_entry(const char* filename) {
    if (filename == NULL) {
        printf("Error: Filename is NULL.\n");
        fflush(stdout);
        return NULL;
    }

    const char* newfilename = prepend_slash(filename);  // Ensure filename starts with a '/'
    printf("Searching for file entry: %s\n", newfilename);
    fflush(stdout);

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {  // Check if the block is marked as the end of a file entry
            uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            FileEntry fileEntry;  // Temporary storage
            flash_read_safe2(address, (uint8_t *)&fileEntry, sizeof(FileEntry));  // Read the file entry from flash

            if (fileEntry.in_use && !fileEntry.is_directory && strcmp(fileEntry.filename, newfilename) == 0) {
                printf("File entry found: %s at block %u\n", newfilename, i);
                fflush(stdout);
                return &fileSystem[i];  // Return a pointer to the file entry in the global file system array
            }
        }
    }

    printf("File entry not found: %s\n", newfilename);
    fflush(stdout);
    return NULL;  // Return NULL if no entry is found
}


char* prepend_slash1(const char* path);
char* prepend_slash1(const char* path) {
    static char new_path[256];
    if (path[0] != '\0' && path[0] != '/') {
        snprintf(new_path, sizeof(new_path), "/%s", path);
        return new_path;
    }
    return (char*)path;
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





























