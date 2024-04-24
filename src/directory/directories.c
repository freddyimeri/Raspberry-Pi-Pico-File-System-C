#include <string.h>
#include <stdlib.h>


#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
#include "../directory/directory_helpers.h"
#include "../filesystem/filesystem_helper.h"  


// static DirectoryEntry staticDirEntries[MAX_DIRECTORY_ENTRIES];
static DirectoryEntry dirEntries[MAX_DIRECTORY_ENTRIES];

bool initialize_directory_block(uint32_t block_number) {
    printf("\n\nDEBUG: Entering initialize_directory_block for block number: %u\n", block_number);
    fflush(stdout);

    // Calculate the physical flash memory address for the block
    uint32_t address = FLASH_TARGET_OFFSET + block_number * FILESYSTEM_BLOCK_SIZE;
    printf("DEBUG: Calculated address = 0x%08X\n", address);
    fflush(stdout);

    if (address + FILESYSTEM_BLOCK_SIZE > FLASH_MEMORY_SIZE_BYTES) {
        printf("ERROR: Block address calculation (0x%08X) exceeds flash memory bounds. FLASH_TARGET_OFFSET: 0x%08X, FILESYSTEM_BLOCK_SIZE: %d, FLASH_MEMORY_SIZE_BYTES: 0x%08X\n", 
                address, FLASH_TARGET_OFFSET, FILESYSTEM_BLOCK_SIZE, FLASH_MEMORY_SIZE_BYTES);
        fflush(stdout);
        return false; // Exit if the calculated address is out of bounds
    }

    // Preparing to clear the directory block
    // uint8_t block_data[FILESYSTEM_BLOCK_SIZE] = {0};
    // printf("DEBUG: Preparing to write zeros to directory block at address 0x%08X\n", address);
    // fflush(stdout);

    // // Perform the flash write operation
    // flash_write_safe(address, block_data, FILESYSTEM_BLOCK_SIZE);

    // Log success of the directory block initialization
    printf("SUCCESS: Directory block %u initialized at address 0x%08X\n", block_number, address);
    fflush(stdout);
    return true; // Indicate success
}


// int split_path(const char* fullPath, char* directoryPath, char* fileName) {
//     char tempPath[256];
//     strcpy(tempPath, fullPath); // Make a copy of fullPath for manipulation

//     char *lastSlash = strrchr(tempPath, '/');
//     bool fileEncountered = false; // Flag to mark when a file has been encountered

//     // Initialize the outputs
//     directoryPath[0] = '\0';
//     fileName[0] = '\0';

//     if (lastSlash) {
//         *lastSlash = '\0'; // Terminate the directory path
//         strcpy(fileName, lastSlash + 1); // Copy the file name part
//         strcpy(directoryPath, tempPath); // Copy the directory part
//     } else {
//         // No slashes found, it's a single segment
//         strcpy(fileName, fullPath);
//     }
    

//     // Check each segment for a file pattern
//     strcpy(tempPath, fullPath); // Reset tempPath
//     char *segment = strtok(tempPath, "/");
//     while (segment) {
//         if (strrchr(segment, '.') != NULL) { // Check if the segment looks like a file
//             fileEncountered = true;
//         } else if (fileEncountered) { // A directory or another segment follows a file
//             printf("Invalid path: '%s'. A file segment cannot be followed by another segment.\n", fullPath);
//             return -1; // Error due to invalid path structure
//             // return NULL;
//         }
//         segment = strtok(NULL, "/");
//     }

//     // Determine if the final part is a file or directory
//     bool finalPartIsFile = strrchr(fileName, '.') != NULL;

//     // Debug output
//     printf("Debug: split_path()\n");
//     printf("Input Path: '%s'\n", fullPath);
//     printf("Extracted Directory: '%s'\n", directoryPath);
//     printf("Extracted File Name: '%s'\n", fileName);
//     printf("Is File: %s\n", finalPartIsFile ? "Yes" : "No");

//         // Only process directory operations if a directory path exists
//     if (directoryPath[0] != '\0') {
//         printf("START process_directory_operation(const char* path)\n");
//         process_directory_operation(directoryPath);
//         printf("END process_directory_operation(const char* path)\n");
//     }
//     //  Process file creation if the final part is determined to be a file
//     if (finalPartIsFile) {
//         char fullPath[512];
//         sprintf(fullPath, "%s/%s", directoryPath, fileName); // Construct the full path for the file
//         process_file_creation(fullPath); // Call to check or create the file
//     }

//     //   if (finalPartIsFile) {
//     //     char fullPath[512];
//     //     sprintf(fullPath, "%s/%s", directoryPath, fileName); // Construct the full path for the file
//     //     FS_FILE* file = process_file_creation(fullPath); // Call to check or create the file
//     //     // return file;
//     // }


//     return finalPartIsFile ? 1 : 0;
//     // return NULL;
// }

 FS_FILE* split_path(const char* fullPath, char* directoryPath, char* fileName) {
    char tempPath[256];
    strcpy(tempPath, fullPath); // Make a copy of fullPath for manipulation

    char *lastSlash = strrchr(tempPath, '/');
    bool fileEncountered = false; // Flag to mark when a file has been encountered

    // Initialize the outputs
    directoryPath[0] = '\0';
    fileName[0] = '\0';

    if (lastSlash) {
        *lastSlash = '\0'; // Terminate the directory path
        strcpy(fileName, lastSlash + 1); // Copy the file name part
        strcpy(directoryPath, tempPath); // Copy the directory part
    } else {
        // No slashes found, it's a single segment
        strcpy(fileName, fullPath);
    }
    
    // Check each segment for a file pattern
    strcpy(tempPath, fullPath); // Reset tempPath
    char *segment = strtok(tempPath, "/");
    while (segment) {
        if (strrchr(segment, '.') != NULL) { // Check if the segment looks like a file
            fileEncountered = true;
        } else if (fileEncountered) { // A directory or another segment follows a file
            printf("Invalid path: '%s'. A file segment cannot be followed by another segment.\n", fullPath);
            return NULL;
        }
        segment = strtok(NULL, "/");
    }

    // Determine if the final part is a file or directory
    bool finalPartIsFile = strrchr(fileName, '.') != NULL;

    // Debug output
    printf("Debug: split_path()\n");
    printf("Input Path: '%s'\n", fullPath);
    printf("Extracted Directory: '%s'\n", directoryPath);
    printf("Extracted File Name: '%s'\n", fileName);
    printf("Is File: %s\n", finalPartIsFile ? "Yes" : "No");

    // Only process directory operations if a directory path exists
    if (directoryPath[0] != '\0') {
        printf("START process_directory_operation(const char* path)\n");
        process_directory_operation(directoryPath);
        printf("END process_directory_operation(const char* path)\n");
    }
    // Process file creation if the final part is determined to be a file
    printf("fileName   %s\n", fileName);
    if (finalPartIsFile) {
        char fullPathConstructed[512];
        sprintf(fullPathConstructed, "%s/%s", directoryPath, fileName); // Construct the full path for the file
        printf("fullPathConstructed: %s\n", fullPathConstructed);
        FS_FILE* file = process_file_creation(fullPathConstructed); // Call to check or create the file
        return file; // Return the file pointer
    }

    return NULL; // Return NULL if no file is created or opened
}

 

bool fs_create_directory(const char* directory, const char* parentPath){
    const char* path = directory;
    uint32_t parentID = 0; 

    printf(" parentPath: %s\n", parentPath);
    printf("DEBUG: Entering fs_create_directory with path: %s\n", path);

    if (path == NULL || *path == '\0') {
        printf("ERROR: Path is NULL or empty.\n");
        return false;
    }

    ////////////////////////////////////////////
     if (parentPath[0] == '\0') {
        parentPath = "/root";  // Default to the root directory
        printf("No directory specified, defaulting to root: %s\n", parentPath);
    }


    DirectoryEntry* dirEntry = DIR_find_directory_entry(parentPath);

    
 
    if (dirEntry) {
        parentID = dirEntry->currentDirId;
        // If the directory is found, access its parentDirId and other details
        printf("Directory '%s' found with ParentDirId(currentDirId): %u\n", parentPath, dirEntry->currentDirId);
        printf("Directory '%s' found with parentID: %u\n", parentPath, parentID);
        free(dirEntry);
    } else {
        // If the directory is not found, handle this case
        printf("EROR: Directory '%s' not found.\n", parentPath);
    }
    ////////////////////////////////////////
    printf("DEBUG FCD 1\n");
    fflush(stdout);
    // Duplicate the path to avoid modifying the original string during tokenization
    char* pathCopy = strdup(path);
    if (!pathCopy) {
        printf("ERROR: Failed to duplicate path string.\n");
        return false;
    }

    char* token;
    char currentPath[256] = {0}; // Assuming a max path length. Adjust as needed.
    printf("DEBUG FCD 2\n");
    // Start tokenizing the path and build it up as we check each segment
    for (token = strtok(pathCopy, "/"); token != NULL; token = strtok(NULL, "/")) {
        if (strlen(currentPath) + strlen(token) + 2 > sizeof(currentPath)) {
            printf("ERROR: Path is too long.\n");
            free(pathCopy); // Free the duplicated path string
            return false;
        }

        strcat(currentPath, "/");
        strcat(currentPath, token);

        // Check if the directory already exists
     
        // If the directory does not exist, attempt to create it
        printf("DEBUG: Creating directory: %s\n", currentPath);
        printf("DEBUG: parentID before call: %u\n", parentID);
        if (!actual_create_directory(currentPath, parentID)) {
            printf("ERROR: Failed to create directory: %s\n", currentPath);
            free(pathCopy); // Free the duplicated path string
            return false;
        }
        
    }

    free(pathCopy); // Ensure the duplicated path string is freed after use
    printf("SUCCESS: Directory created: %s\n", path);
    return true;
}


DirectoryEntry* fs_list_directory(const char* path) {
    printf("Listing directory: %s\n", path);

    // Special handling for the root directory
    if (strcmp(path, "/root") == 0) {
        printf("Special handling for root directory...\n");
    }

    // if (path == NULL || strlen(path) == 0) {
    //     printf("ERROR: Invalid or empty path provided to fs_list_directory.\n");
    //     fflush(stdout);
    //     return NULL;
    // }
    printf("DEBUG: Entering fs_list_directory with path: %s\n", path);

    // Find the directory entry for the given path
    DirectoryEntry* dir = DIR_find_directory_entry(path);
    if (dir == NULL || !dir->is_directory) {
        printf("ERROR: Directory '%s' not found or is not a directory.\n", path);
        fflush(stdout);
        return NULL;
    }
    printf("code 1\n");

    // Dynamically allocate memory for directory listings. 
    // Assuming you have a way to know or estimate the maximum entries, adjust accordingly.
    DirectoryEntry* entries = malloc(sizeof(DirectoryEntry) * MAX_DIRECTORY_ENTRIES);
    if (entries == NULL) {
        printf("ERROR: Memory allocation failed for directory listing.\n");
        return NULL;
    }
    printf("code 2\n");
    uint8_t* blockBuffer = malloc(FILESYSTEM_BLOCK_SIZE);
    if (!blockBuffer) {
        printf("ERROR: Memory allocation failed for directory reading.\n");
        free(entries); // Clean up previously allocated memory before returning
        return NULL;
    }
    printf("code 3\n");
    // Read the first directory block
    // read_directory_block(dir->start_block, blockBuffer, FILESYSTEM_BLOCK_SIZE);
    printf("code 4\n");
    // Parse entries from the block buffer
    int count = 0;
    for (int offset = 0; offset < FILESYSTEM_BLOCK_SIZE && count < MAX_DIRECTORY_ENTRIES; offset += sizeof(DirectoryEntry)) {
        DirectoryEntry* entry = (DirectoryEntry*)(blockBuffer + offset);
        if (entry->name[0] == '\0') break; // End of directory

        // Add the entry to the dynamic entries array
        memcpy(&entries[count++], entry, sizeof(DirectoryEntry));
    }
    printf("code 5\n");
    free(blockBuffer); // No longer need the buffer
    printf("code 6\n");
    printf("SUCCESS: Listed directory '%s'. Total entries: %d.\n", path, count);
    fflush(stdout);

    // Resize the entries array to match the actual number of entries
    entries = realloc(entries, sizeof(DirectoryEntry) * count);
    printf("code 7\n");
    return entries; // Caller is responsible for freeing this memory
}



 
bool is_directory_valid(const DirectoryEntry* directory) {
    // Check if the directory entry is NULL
    if (directory == NULL) {
        printf("Directory entry is NULL.\n");
        return false;
    }

    // Check if the directory has a valid start block
    if (directory->start_block == FAT_NO_FREE_BLOCKS || directory->start_block >= TOTAL_BLOCKS) {
        printf("Directory start block is invalid. Block: %u\n", directory->start_block);
        return false;
    }

 
    return true;
}

  

bool reset_root_directory(void) {
    printf("Resetting root directory...\n");
    // Ensure the filesystem has been initialized before attempting to reset the root directory
    if (!fs_initialized) {
        printf("Filesystem not initialized. Cannot reset root directory.\n");
        return false; // Filesystem must be initialized first
    }
    
    // printf("dirEntries[0].is_directory: %d\n", dirEntries[0].is_directory);
    // printf("strcmp(dirEntries[0].name, \"/\"): %d\n", strcmp(dirEntries[0].name, "/"));
    // printf("dirEntries[0].in_use: %d\n", dirEntries[0].in_use);
    
    // Check if the root directory is already initialized and valid
    // to do , use the DIR_find_directory_entry to fix this 
    if (dirEntries[0].is_directory && strcmp(dirEntries[0].name, "/") == 0 && dirEntries[0].in_use) {
        // Validate the existing root directory's integrity
        if (is_directory_valid(&dirEntries[0])) {
            printf("Root directory is valid. No reset needed.\n");
            return true; // No reset required if valid
        } else {
            printf("Root directory integrity check failed. Reinitializing...\n");
            // If existing root directory is not valid, proceed to reinitialize
        }
    } else {
        printf("Initializing root directory...\n");
    }

    // Allocate a new block for the root directory as part of initialization or reinitialization
    uint32_t rootBlock = fat_allocate_block();
    if (rootBlock == FAT_NO_FREE_BLOCKS) {
        printf("Failed to allocate block for root directory.\n");
        return false; // Handle block allocation failure
    }

    // // Initialize the newly allocated block as a directory
    // if (!initialize_directory_block(rootBlock)) {
    //     printf("Failed to initialize block for root directory.\n");
    //     fat_free_block(rootBlock); // Free the allocated block on failure
    //     return false; // Handle directory block initialization failure
    // }
    DirectoryEntry* freeEntry = find_free_directory_entry();
    // Set up or update the root directory entry
    // dirEntries[0].is_directory = true;
    // strcpy(dirEntries[0].name, "/");
    // dirEntries[0].in_use = true;
    // dirEntries[0].start_block = rootBlock;
    // dirEntries[0].size = 0; // Optionally set the directory size to 0 or an appropriate value

    const char* NamePath = "/root";
    uint32_t parentDirIdHHolder = generateUniqueId();


    freeEntry->is_directory = true;
    strncpy(freeEntry->name, NamePath, sizeof(freeEntry->name) - 1);
    freeEntry->name[sizeof(freeEntry->name) - 1] = '\0'; // Ensure null termination
    freeEntry->parentDirId = parentDirIdHHolder;
    freeEntry->currentDirId = parentDirIdHHolder; 
    freeEntry->in_use = true;
    freeEntry->start_block = rootBlock;
    freeEntry->size = 0; // Initialize the size, if applicable

    printf("Root directory (re)initialized at block %u.\n", rootBlock);
    uint32_t flashAddress =  rootBlock * FILESYSTEM_BLOCK_SIZE;

    flash_write_safe2(flashAddress, (const uint8_t *)freeEntry, sizeof(DirectoryEntry));
    return true; // Indicate success
}

 
 
 
 

bool actual_create_directory(const char* path, uint32_t pathToParent) {
    printf(" parentPath in actual_create_directory: %d\n", pathToParent);
    printf("DEBUG: Entering actual_create_directory with path: %s\n", path);

    if (path == NULL || *path == '\0') {
        printf("ERROR: Path is NULL or empty in actual_create_directory.\n");
        return false;
    }
    printf("DEBUG: Entering actual_create_directory: 1\n");

 

    // Allocate a block for the directory.
    uint32_t directoryBlock = fat_allocate_block();
    if (directoryBlock == FAT_NO_FREE_BLOCKS) {
        printf("ERROR: Failed to allocate block for directory: %s\n", path);
        return false;
    }
    printf("DEBUG: Entering actual_create_directory: 3\n");

 

    // Find a free directory entry in the global array
    DirectoryEntry* freeEntry = find_free_directory_entry();
    if (!freeEntry) {
        printf("ERROR: No free directory entries available.\n");
        fat_free_block(directoryBlock);
        return false;
    }
    printf("DEBUG: Entering actual_create_directory: 5\n");
    // uint32_t parentDirId;    
    uint32_t parentDirIdHHolder = generateUniqueId();  
    // Initialize the new directory entry within the filesystem's directory entry structure
    freeEntry->is_directory = true;
    strncpy(freeEntry->name, path, sizeof(freeEntry->name) - 1); // Copy directory name
    freeEntry->name[sizeof(freeEntry->name) - 1] = '\0'; // Ensure null termination
    freeEntry->currentDirId = parentDirIdHHolder; 
     printf(" parentPath before     freeEntry->parentDirId = pathToParent;  %d\n", pathToParent);
    freeEntry->parentDirId = pathToParent; 
    freeEntry->in_use = true;
    freeEntry->start_block = directoryBlock;
    freeEntry->size = 0; // Initialize the size, if applicable
    printf("DEBUG: Entering actual_create_directory: 6\n");

    // Serialize and write the directory entry to the allocated block in flash
    // uint32_t flashAddress = FLASH_TARGET_OFFSET + directoryBlock * FILESYSTEM_BLOCK_SIZE;

    printf("DEBUG: freeEntry->name: %s\n", freeEntry->name);
    printf("DEBUG: freeEntry->parentDirId: %d\n", freeEntry->parentDirId);
    printf("DEBUG: freeEntry->currentDirId: %d\n", freeEntry->currentDirId);
    printf("DEBUG: freeEntry->is_directory: %d\n", freeEntry->is_directory);
    printf("DEBUG: freeEntry->start_block: %d\n", freeEntry->start_block);
    printf("DEBUG: freeEntry->size: %d\n", freeEntry->size);
    printf("DEBUG: freeEntry->in_use: %d\n", freeEntry->in_use);
    printf("DEBUG: directoryBlock: %d\n", directoryBlock);
    printf("sizeof(DirectoryEntry) %d\n", sizeof(DirectoryEntry));
    fflush(stdout);
    sleep_ms(100);
    uint32_t flashAddress =  directoryBlock * FILESYSTEM_BLOCK_SIZE;
   
    printf("DEBUG: flashAddress %d\n", flashAddress);
    // Note: Adjust the serialization process as per your DirectoryEntry structure
    flash_write_safe2(flashAddress, (const uint8_t *)freeEntry, sizeof(DirectoryEntry));
    // printf("DEBUG: Entering actual_create_directory: 7\n");
    // printf("read back from flash\n");
    
    // printf("DEBUG: flashAddress %d\n", flashAddress);
    //  uint32_t flashAddressREAD =  freeEntry->start_block * FILESYSTEM_BLOCK_SIZE;
    // printf("DEBUG: flashAddressREAD %d\n", flashAddressREAD);
    
 
    // printf("bufferREAD: %d\n", bufferREAD);
    // printf("bufferREAD: %s\n", bufferREAD);
    //  printf("bufferREAD: %c\n", bufferREAD);
    printf("SUCCESS: Directory created and written to flash: %s\n", path);
    return true;
}
 
 
   



void split_path_into_parent_and_dir(const char* fullPath, char* parentPath, char* dirName) {
    printf("DEBUG: Entering split_path_into_parent_and_dir for path: '%s'\n", fullPath ? fullPath : "NULL");

    // Verify input parameters are valid
    if (!fullPath || !parentPath || !dirName) {
        printf("ERROR: Invalid input parameters. fullPath, parentPath, or dirName is NULL.\n");
        return;
    }

    // Make a copy of fullPath because strtok modifies the string
    char* pathCopy = strdup(fullPath);
    if (!pathCopy) {
        printf("ERROR: Memory allocation failed for pathCopy.\n");
        return; // In case of allocation failure
    }

    printf("DEBUG: pathCopy created: '%s'\n", pathCopy);

    char* lastSlash = strrchr(pathCopy, '/');
    printf("DEBUG: Last slash found at: '%s'\n", lastSlash ? lastSlash : "NULL");

    if (!lastSlash) {
        // No slash found, implying fullPath is a root or invalid
        printf("DEBUG: No slash found, fullPath is considered a root or invalid.\n");
        free(pathCopy);
        strcpy(parentPath, "/root");
        strcpy(dirName, fullPath);
        printf("DEBUG: parentPath: '%s', dirName: '%s'\n", parentPath, dirName);
        return;
    }

    // Special case for root directory
    if (lastSlash == pathCopy) {
        printf("DEBUG: Root directory case encountered.\n");
        strcpy(parentPath, "/root");
        strcpy(dirName, lastSlash + 1);
        free(pathCopy);
        printf("DEBUG: parentPath: '%s', dirName: '%s'\n", parentPath, dirName);
        return;
    }

    // Terminate the parent path at the last slash
    *lastSlash = '\0';
    strcpy(parentPath, pathCopy);
    strcpy(dirName, lastSlash + 1);

    printf("DEBUG: parentPath: '%s', dirName: '%s'\n", parentPath, dirName);

    free(pathCopy);
} 

  

 
DirectoryEntry* find_free_directory_entry(void) {
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].is_directory) {
            // Found an unused directory entry
            return &dirEntries[i];
        }
    }
    // No unused directory entries available
    return NULL;
} 


 
 
// DirectoryEntry* DIR_find_directory_entry(const char* directoryName) {
//     printf("\n\nENTERED DIR_find_directory_entry\n");
//     fflush(stdout);

//      // Prepend '/' to directoryName if not present
//     char fullPath[256];  // Ensure this size fits your maximum expected path length
//     if (directoryName[0] != '/') {
//         snprintf(fullPath, sizeof(fullPath), "/%s", directoryName);
//     } else {
//         strncpy(fullPath, directoryName, sizeof(fullPath));
//         fullPath[sizeof(fullPath) - 1] = '\0'; // Ensure null termination
//     }

//     printf("NEWW directoryName: %s\n", directoryName);
//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//         if (FAT[i] == FAT_ENTRY_END) {
//             printf("Checking block %u\n", i);
//             fflush(stdout);

//             uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
//             DirectoryEntry dirEntry;  // Temporary storage
//             flash_read_safe2(address, (uint8_t *)&dirEntry, sizeof(DirectoryEntry));

//             printf("Checking entry name: %s\n", dirEntry.name);
//             printf("Checking entry parentDirId: %u\n", dirEntry.parentDirId);
//             printf("Checking entry is_directory: %d\n", dirEntry.is_directory);
//             printf("Checking entry start_block: %u\n", dirEntry.start_block);
//             printf("Checking entry size: %u\n", dirEntry.size);
//             printf("Checking entry in_use: %d\n", dirEntry.in_use);
//             fflush(stdout);
            
//             if (dirEntry.in_use && strcmp(dirEntry.name, directoryName) == 0) {
//                 printf("Directory entry found: %s\n", directoryName);
//                 fflush(stdout);
//                 DirectoryEntry* result = malloc(sizeof(DirectoryEntry));  // Dynamically allocate memory
//                 if (result) {
//                     *result = dirEntry;  // Copy the data
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

//     printf("Directory entry not found: %s\n", directoryName);
//     fflush(stdout);
//     return NULL;
// }

DirectoryEntry* DIR_find_directory_entry(const char* directoryName) {
    printf("\n\nENTERED DIR_find_directory_entry\n");
    fflush(stdout);

    // Prepend '/' to directoryName if not present
    char fullPath[256];  // Ensure this size fits your maximum expected path length
    if (directoryName[0] != '/') {
        snprintf(fullPath, sizeof(fullPath), "/%s", directoryName);
    } else {
        strncpy(fullPath, directoryName, sizeof(fullPath));
        fullPath[sizeof(fullPath) - 1] = '\0'; // Ensure null termination
    }

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {
            printf("Checking block %u\n", i);
            fflush(stdout);

            uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            DirectoryEntry dirEntry;  // Temporary storage
            flash_read_safe2(address, (uint8_t *)&dirEntry, sizeof(DirectoryEntry));

            printf("Checking entry name: %s\n", dirEntry.name);
            printf("Checking entry parentDirId: %u\n", dirEntry.parentDirId);
            printf("Checking entry currentDirId: %u\n", dirEntry.currentDirId);
            printf("Checking entry is_directory: %d\n", dirEntry.is_directory);
            printf("Checking entry start_block: %u\n", dirEntry.start_block);
            printf("Checking entry size: %u\n", dirEntry.size);
            printf("Checking entry in_use: %d\n", dirEntry.in_use);
            fflush(stdout);

            if (dirEntry.in_use && strcmp(dirEntry.name, fullPath) == 0) {
                printf("Directory entry found: %s\n", fullPath);
                fflush(stdout);
                DirectoryEntry* result = malloc(sizeof(DirectoryEntry));  // Dynamically allocate memory
                if (result) {
                    *result = dirEntry;  // Copy the data
                    return result;
                } else {
                    printf("Memory allocation failed.\n");
                    fflush(stdout);
                    return NULL;
                }
            }
            sleep_ms(200);
        }
    }

    printf("Directory entry not found: %s\n", fullPath);
    fflush(stdout);
    return NULL;
}


bool Dir_remove(const char* directoryName) {
    printf("Attempting to remove directory: %s\n", directoryName);
    fflush(stdout);

    // First, find the directory entry
    DirectoryEntry* dirEntry = DIR_find_directory_entry(directoryName);
    if (!dirEntry) {
        printf("Directory '%s' not found.\n", directoryName);
        fflush(stdout);
        return false;  // Directory not found
    }

    // Get the start block of the directory from the directory entry
    uint32_t directoryBlock = dirEntry->start_block;

    // Erase the flash memory corresponding to this directory
    uint32_t flashAddress = directoryBlock * FILESYSTEM_BLOCK_SIZE;
    flash_erase_safe2(flashAddress);

    // Update the FAT to mark this block as free
    if (directoryBlock < TOTAL_BLOCKS) {
        fat_free_block(directoryBlock);
    } else {
        printf("Error: Directory block out of range.\n");
        fflush(stdout);
    }

    free(dirEntry);  // Free the dynamically allocated directory entry

    printf("Directory '%s' removed successfully.\n", directoryName);
    fflush(stdout);
    return true;
}



 
DirectoryEntry* DIR_all_directory_entries(void) {
    printf("\n\nENTERED DIR_all_directory_entries\n");
    fflush(stdout);

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {
            printf("Checking block %u\n", i);
            fflush(stdout);

            uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            DirectoryEntry dirEntry;  // Temporary storage
            flash_read_safe2(address, (uint8_t *)&dirEntry, sizeof(DirectoryEntry));

            printf("Checking entry name: %s\n", dirEntry.name);
            printf("Checking entry parentDirId: %u\n", dirEntry.parentDirId);
            printf("Checking entry currentDirId: %u\n", dirEntry.currentDirId);
            printf("Checking entry is_directory: %d\n", dirEntry.is_directory);
            printf("Checking entry start_block: %u\n", dirEntry.start_block);
            printf("Checking entry size: %u\n", dirEntry.size);
            printf("Checking entry in_use: %d\n", dirEntry.in_use);
            fflush(stdout);
            
            
        }
    }
 
}
