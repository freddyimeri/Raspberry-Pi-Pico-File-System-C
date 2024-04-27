 
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

 



 

void prepend_slash(const char* path, char* buffer, size_t buffer_size) {
    if (path == NULL || buffer == NULL || buffer_size == 0) {
        return; // Safety check for null pointers and non-zero size
    }

    if (path[0] == '/') {
        // The path already starts with a slash, just copy it.
        strncpy(buffer, path, buffer_size);
    } else {
        // Prepend a slash to the path.
        snprintf(buffer, buffer_size, "/%s", path);
    }
    buffer[buffer_size - 1] = '\0'; // Ensure null termination
}




int find_file_entry_by_name(const char* filename) {

    char path[1024]; // Define a sufficiently large buffer for the path
    prepend_slash(filename, path, sizeof(path));
    
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


int find_file_existance(const char* filename,  uint32_t parentID ) {

    char path[1024]; // Define a sufficiently large buffer for the path
    prepend_slash(filename, path, sizeof(path));
    
    if (path == NULL) {
        printf("Error: Filename is NULL.\n");
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, path) == 0 && fileSystem[i].parentDirId == parentID) {   
            printf("File found: %s at index %d\n", path, i);
            return 0;
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



/**
 * Creates a new file entry in the filesystem.
 * @param path The path or name of the file.
 * @return Pointer to the created FileEntry or NULL if creation fails.
 */
FileEntry* createFileEntry(const char* path,  uint32_t parentDirId) {
    printf("debug createFileEntry for path: %s\n", path);
    sleep_ms(1000);

    sleep_ms(1000);
    printf("debug createFileEntry for path: %s\n", path);
    for (int i = 0; i < MAX_FILES; i++) {
        printf("Checking file entry at index %d\n", i);
        sleep_ms(100);
        if (!fileSystem[i].in_use) {
            printf("Creating new file entry at index %d\n", i);
            // printf("Root directory ID: %u\n", rootDirId);
            strncpy(fileSystem[i].filename, path, sizeof(fileSystem[i].filename) - 1);
            printf("Filename: %s\n", fileSystem[i].filename);
            fileSystem[i].filename[sizeof(fileSystem[i].filename) - 1] = '\0';
            fileSystem[i].in_use = true;
            fileSystem[i].is_directory = false; // Default to file
            fileSystem[i].size = 0;
            
            fileSystem[i].start_block = fat_allocate_block();
            fileSystem[i].parentDirId = parentDirId;
            fileSystem[i].unique_file_id = generateUniqueId();

            printf("New file created: %s\n", fileSystem[i].filename);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("File size: %u\n", fileSystem[i].size);
            printf("Filesystem entry index: %d\n", i);
            // printf("Parent Directory ID: %u\n", fileSystem[i].parentDirId);
            
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
    printf("Attempting to reset file content.\n");
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




FileEntry* FILE_find_file_entry(const char* filename,  uint32_t parentID) {
     char newfilename[512]; // Define a sufficiently large buffer for the path
    prepend_slash(filename, newfilename, sizeof(newfilename));
    
    printf("\n\nENTERED FILE_find_file_entry\n");
    printf("Searching for file entry: %s\n", newfilename);
    fflush(stdout);
     for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && !fileSystem[i].is_directory 
        && strcmp(fileSystem[i].filename, newfilename) == 0
        && fileSystem[i].parentDirId == parentID) {
            printf("File entry found: %s\n", newfilename);
            fflush(stdout);
            return &fileSystem[i];
        }
    }
    return NULL;

}




// FileEntry* FILE_find_file_entry(const char* filename) {
//     const char* newfilename = prepend_slash(filename);
//     printf("\n\nENTERED FILE_find_file_entry\n");
//     fflush(stdout);

//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//         if (FAT[i] == FAT_ENTRY_END) {
//             printf("Checking block %u\n", i);
//             fflush(stdout);

//             uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
//             FileEntry fileEntry;  // Temporary storage
//             flash_read_safe2(address, (uint8_t *)&fileEntry, sizeof(FileEntry));

//             printf("Checking file name: %s\n", fileEntry.filename);
//             printf("Checking file size: %u\n", fileEntry.size);
//             printf("Checking file start block: %u\n", fileEntry.start_block);
//             printf("Checking file in_use: %d\n", fileEntry.in_use);
//             printf("Checking if directory flag: %d\n", fileEntry.is_directory);
//             printf("buffer: %s\n", fileEntry.buffer);
//             fflush(stdout);
            
//             if (fileEntry.in_use && !fileEntry.is_directory && strcmp(fileEntry.filename, newfilename) == 0) {
//                 printf("File entry found: %s\n", newfilename);
//                 fflush(stdout);
//                 FileEntry* result = malloc(sizeof(FileEntry));  // Dynamically allocate memory
//                 if (result) {
//                     *result = fileEntry;  // Copy the data
//                     return result;
//                 } else {
//                     printf("Memory allocation failed.\n");
//                     fflush(stdout);
//                     return NULL;
//                 }
//             }
//             sleep_ms(200);
//         }
//     }

//     printf("File entry not found: %s\n", filename);
//     fflush(stdout);
//     return NULL;
// }




//  //updated
// FileEntry* FILE_find_file_entry(const char* filename) {
//     if (filename == NULL) {
//         printf("Error: Filename is NULL.\n");
//         fflush(stdout);
//         return NULL;
//     }

//     const char* newfilename = prepend_slash(filename);  // Ensure filename starts with a '/'
//     printf("Searching for file entry: %s\n", newfilename);
//     fflush(stdout);

//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//         if (FAT[i] == FAT_ENTRY_END) {  // Check if the block is marked as the end of a file entry
//             uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
//             FileEntry fileEntry;  // Temporary storage
//             flash_read_safe2(address, (uint8_t *)&fileEntry, sizeof(FileEntry));  // Read the file entry from flash

//             if (fileEntry.in_use && !fileEntry.is_directory && strcmp(fileEntry.filename, newfilename) == 0) {
//                 printf("File entry found: %s at block %u\n", newfilename, i);
//                 fflush(stdout);
//                 return &fileSystem[i];  // Return a pointer to the file entry in the global file system array
//             }
//         }
//     }

//     printf("File entry not found: %s\n", newfilename);
//     fflush(stdout);
//     return NULL;  // Return NULL if no entry is found
// }


 


// PathParts extract_last_two_parts(const char* fullPath) {
//     PathParts parts;
//     memset(&parts, 0, sizeof(parts));

//     printf("Attempting to extract last two parts from: %s\n", fullPath);
//     if (fullPath == NULL) {
//         printf("Input path is NULL.\n");
//         return parts; // Return empty parts if the input is NULL
//     }

//     // Use strrchr to find the last occurrence of '/'
//     const char* lastSlash = strrchr(fullPath, '/');
//     if (!lastSlash) { // If there's no '/', the path is just a filename
//         printf("Only a filename provided without directory path.\n");
//         strcpy(parts.filename, fullPath); // Assume the entire path is the filename
//         printf("Extracted file name: %s\n", parts.filename);
//         return parts;
//     }

//     // If we found a '/', copy the filename after the '/'
//     strcpy(parts.filename, lastSlash + 1);
//     printf("Extracted file name: %s\n", parts.filename);

//     // To find the directory just before the filename:
//     // We make a copy of the path to manipulate
//     char pathCopy[256];
//     strncpy(pathCopy, fullPath, lastSlash - fullPath);
//     pathCopy[lastSlash - fullPath] = '\0'; // Null-terminate at the last '/'

//     // Now find the last '/' in the modified copy
//     const char* secondLastSlash = strrchr(pathCopy, '/');
//     if (secondLastSlash) {
//         strcpy(parts.directory, secondLastSlash + 1); // Copy the directory name
//     } else {
//         strcpy(parts.directory, pathCopy); // If no second '/', the directory is the whole modified path
//     }

//     printf("Extracted directory: %s\n", parts.directory);
//     return parts;
// }



/**
 * Extracts the last two components of a given file path: the directory and the filename.
 * This function assumes paths are Unix-like with '/' as the directory separator.
 *
 * @param fullPath The full path from which to extract the parts.
 * @return A PathParts structure containing the extracted directory and filename.
 */
PathParts extract_last_two_parts(const char* fullPath) {
    PathParts parts;
    memset(&parts, 0, sizeof(parts)); // Initialize the parts structure to zero.


    // Check if the input path is NULL, which is an error condition.
    if (fullPath == NULL) {
        printf("Input path is NULL.\n");
        return parts; // Return empty parts if the input is NULL.
    }

    // Use strrchr to find the last occurrence of '/'
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash) { // If there's no '/', the path is just a filename
        char path[512]; // Define a sufficiently large buffer for the path
        prepend_slash(fullPath, path, sizeof(path));
        strcpy(parts.filename, path); // Assume the entire path is the filename.
        return parts; // Return the parts with only the filename filled.
    }

     if (lastSlash[1] == '\0') {
            strcpy(parts.filename, lastSlash + 1);
        }else{
            strcpy(parts.filename, lastSlash );
    }

 

    // Copy the path up to the last slash into a temporary buffer to extract the directory.
    char pathCopy[256];
    strncpy(pathCopy, fullPath, lastSlash - fullPath); // Copy the part before the last slash.
    pathCopy[lastSlash - fullPath] = '\0'; // Null-terminate the copied part.


    // Find the last '/' in the temporary copy to extract the directory name.
    const char* secondLastSlash = strrchr(pathCopy, '/');
    if (secondLastSlash) {
        if (secondLastSlash[1] == '\0') {
            strcpy(parts.directory, secondLastSlash + 1);
        }else{
            char Dirpath[512]; // Define a sufficiently large buffer for the path
            prepend_slash(secondLastSlash + 1, Dirpath, sizeof(Dirpath));
            strcpy(parts.directory, Dirpath);
        }
        
    } else {
        // If there's no second last slash, the entire modified path is the directory.
        strcpy(parts.directory, pathCopy);
    }

    return parts; // Return the populated parts structure.
}



void fs_all_files_entries(void) {
    printf("\n\nENTERED fs_all_files_entries\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && !fileSystem[i].is_directory) {
            printf("\n\nEnrty %d is a directory\n", i);
            printf("file entry %d: %s\n", i, fileSystem[i].filename);
            printf("Parent file ID: %u\n", fileSystem[i].parentDirId);
            printf("Current file ID: %u\n", fileSystem[i].unique_file_id);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("file size: %u\n", fileSystem[i].size);
            printf("buffer: %s\n", fileSystem[i].buffer);
            
            fflush(stdout);
        }
    }
}



void fs_all_files_entrieszzzz(void) {
    printf("\n\nENTERED fs_all_files_entries\n");
    for (int i = 0; i < 5; i++) {
            printf("\n\nEnrty I is : %d \n", i);
            printf("file entry %d: %s\n", i, fileSystem[i].filename);
            printf("Parent file ID: %u\n", fileSystem[i].parentDirId);
            printf("Current file ID: %u\n", fileSystem[i].unique_file_id);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("file size: %u\n", fileSystem[i].size);
            printf("buffer: %s\n", fileSystem[i].buffer);
            
            fflush(stdout);
    }
}





// /**
//  * Modifies the destination filename to include "Copy" before the extension
//  * or at the end if there is no extension.
//  * 
//  * @param source_filename The original filename.
//  * @param dest_filename The buffer for the destination filename, modified in-place.
//  * @param dest_size The size of the destination buffer.
//  */
// void appendCopyToFilename(const char* source_filename, char* dest_filename, size_t dest_size) {
//     const char* lastDot = strrchr(source_filename, '.');
//     if (lastDot) {
//         // There's an extension. Copy up to the dot.
//         int basenameLength = lastDot - source_filename;
//         if (basenameLength + 5 + strlen(lastDot) >= dest_size) {  // +5 for "Copy" and null terminator
//             fprintf(stderr, "Error: Destination buffer too small to hold modified filename.\n");
//             return;
//         }
        
//         // Copy the base part of the filename
//         strncpy(dest_filename, source_filename, basenameLength);
//         dest_filename[basenameLength] = '\0';
        
//         // Append "Copy" and then the extension
//         strcat(dest_filename, "Copy");
//         strcat(dest_filename, lastDot);
//     } else {
//         // No extension found, just append "Copy".
//         if (strlen(source_filename) + 5 >= dest_size) {  // +5 for "Copy" and null terminator
//             fprintf(stderr, "Error: Destination buffer too small to hold modified filename.\n");
//             return;
//         }
//         snprintf(dest_filename, dest_size, "%sCopy", source_filename);
//     }
// }


void appendCopyToFilename(char *filename) {
    char temp[256]; // Temporary buffer to hold the new filename
    const char *lastDot = strrchr(filename, '.');
    if (lastDot) {
        // There's an extension. Copy up to the dot.
        int basenameLength = lastDot - filename;
        strncpy(temp, filename, basenameLength);
        temp[basenameLength] = '\0';  // Null-terminate after the basename

        // Append "Copy" and then the extension.
        snprintf(temp + basenameLength, sizeof(temp) - basenameLength, "Copy%s", lastDot);
    } else {
        // No extension found, just append "Copy".
        snprintf(temp, sizeof(temp), "%sCopy", filename);
    }

    // Safely copy back the new filename to the original buffer
    strncpy(filename, temp, 256);
    filename[255] = '\0'; // Ensure null termination in case of overflow
}


void construct_full_path(const char* directory, const char* filename, char* full_path, size_t max_size) {
    if (directory == NULL || filename == NULL || full_path == NULL) return;

    // Ensure the directory path ends with a slash if it's not empty
    size_t len = strlen(directory);
    if (len > 0 && directory[len - 1] != '/') {
        snprintf(full_path, max_size, "%s/%s", directory, filename);
    } else {
        snprintf(full_path, max_size, "%s%s", directory, filename);
    }
    full_path[max_size - 1] = '\0'; // Ensure null-termination
}



void set_default_path(char* path, const char* default_path) {
    if (path[0] == '\0') {
        strcpy(path, default_path);
    }
}
