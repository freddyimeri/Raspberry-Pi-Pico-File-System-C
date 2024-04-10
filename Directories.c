#include "Directories.h"
#include <string.h>
#include <stdlib.h>
#include "flash_config.h"
#include "fat_fs.h"
#include "flash_ops.h"
#include "filesystem.h"


static DirectoryEntry staticDirEntries[MAX_DIRECTORY_ENTRIES];
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



bool fs_create_directory(const char* path) {
    printf("DEBUG: Entering fs_create_directory with path: %s\n", path);

    if (path == NULL || *path == '\0') {
        printf("ERROR: Path is NULL or empty.\n");
        return false;
    }

    // Duplicate the path to avoid modifying the original string during tokenization
    char* pathCopy = strdup(path);
    if (!pathCopy) {
        printf("ERROR: Failed to duplicate path string.\n");
        return false;
    }

    char* token;
    char currentPath[256] = {0}; // Assuming a max path length. Adjust as needed.

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
        if (!directory_exists(currentPath)) {
            // If the directory does not exist, attempt to create it
            printf("DEBUG: Creating directory: %s\n", currentPath);
            if (!actual_create_directory(currentPath)) {
                printf("ERROR: Failed to create directory: %s\n", currentPath);
                free(pathCopy); // Free the duplicated path string
                return false;
            }
        }
    }

    free(pathCopy); // Ensure the duplicated path string is freed after use
    printf("SUCCESS: Directory created: %s\n", path);
    return true;
}


DirectoryEntry* fs_list_directory(const char* path) {


    printf("Listing directory: %s\n", path);
    // If path is the root, handle specially
    if (strcmp(path, "/") == 0) {
        // Log or handle root directory listing
        printf("Special handling for root directory...\n");
    }

    
    printf("DEBUG: Entering fs_list_directory for path: %s\n", path);
    fflush(stdout);

    if (path == NULL || strlen(path) == 0) {
        printf("ERROR: Invalid or empty path provided to fs_list_directory.\n");
        fflush(stdout);
        return NULL;
    }

    memset(staticDirEntries, 0, sizeof(staticDirEntries));
    DirectoryEntry* dir = find_directory_entry(path);
    if (dir == NULL || !dir->is_directory) {
        printf("ERROR: Directory '%s' not found or is not a directory.\n", path);
        fflush(stdout);
        return NULL;
    }

    uint8_t* blockBuffer = malloc(FILESYSTEM_BLOCK_SIZE);
    if (!blockBuffer) {
        printf("ERROR: Memory allocation failed for directory reading.\n");
        return NULL;
    }

    // Read the first directory block
    read_directory_block(dir->start_block, blockBuffer, FILESYSTEM_BLOCK_SIZE);

    // Parse entries from the block buffer
    int count = 0;
    for (int offset = 0; offset < FILESYSTEM_BLOCK_SIZE && count < MAX_DIRECTORY_ENTRIES; offset += sizeof(DirectoryEntry)) {
        DirectoryEntry* entry = (DirectoryEntry*)(blockBuffer + offset);

        // Check if the directory entry is considered "end" or invalid based on its 'name' field
        if (entry->name[0] == '\0') break; // Found an unused or invalid entry, treat as end of directory

        // Optionally, here you might add additional validity checks if necessary
        staticDirEntries[count++] = *entry; // Add the entry to the static directory entries array
    }

    free(blockBuffer);
    printf("SUCCESS: Listed directory '%s'. Total entries: %d.\n", path, count);
    fflush(stdout);
    return staticDirEntries;
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

    // Here, you might add additional checks relevant to your filesystem design.
    // For example, you could verify the directory's block is correctly marked in the FAT,
    // check the directory's integrity, or ensure the directory's size makes sense.

    // If the directory passes all checks, it is considered valid.
    return true;
}

 

bool reset_root_directory(void) {
    if (!fs_initialized) {
        printf("Filesystem not initialized. Cannot reset root directory.\n");
        return false; // Indicate error due to filesystem not being initialized
    }

    // Attempt to locate the root directory entry
    DirectoryEntry* rootDir = find_directory_entry("/");
    if (rootDir != NULL) {
        // Root directory exists, verify its integrity
        if (is_directory_valid(rootDir)) {
            // Root directory is valid, no reset required
            printf("Root directory is valid. No reset needed.\n");
            return true;
        } else {
            // Root directory is not valid, needs reinitialization
            printf("Root directory integrity check failed. Reinitializing...\n");
            // Perform necessary cleanup or repair actions here
        }
    } else {
        // Root directory does not exist, need to create it
        printf("Root directory does not exist. Creating...\n");
    }

    // Allocate a new block for the root directory
    uint32_t rootBlock = fat_allocate_block();
    if (rootBlock == FAT_NO_FREE_BLOCKS) {
        printf("Failed to allocate block for root directory.\n");
        return false; // Indicate failure due to block allocation issue
    }

    // Initialize the newly allocated block as a directory
    if (!initialize_directory_block(rootBlock)) {
        printf("Failed to initialize block for root directory.\n");
        fat_free_block(rootBlock); // Free the allocated block on failure
        return false; // Indicate failure in directory block initialization
    }

    // Create a new root directory entry if it doesn't exist or update the existing one
    if (rootDir == NULL) {
        rootDir = create_new_directory_entry("/");
        if (!rootDir) {
            printf("Failed to create new root directory entry.\n");
            return false;
        }
    }

    // Update root directory properties
    rootDir->in_use = true;
    rootDir->is_directory = true;
    rootDir->size = 0; // The size is set to 0, as it may represent the number of contained entries which needs initialization
    rootDir->start_block = rootBlock;

    printf("Root directory has been successfully set up.\n");
    return true;
}

/////////////////////////////////////////

// bool fs_remove_directory(const char* path) {
//     printf("Attempting to remove directory: %s\n", path);

//     if (path == NULL || *path == '\0') {
//         printf("Error: Path is NULL or empty.\n");
//         return false;
//     }

//     // Check if the directory exists
//     DirectoryEntry* dirEntry = find_directory_entry(path);
//     if (dirEntry == NULL) {
//         printf("Error: Directory '%s' does not exist.\n", path);
//         return false;
//     }

//     // List the directory's contents
//     DirectoryEntry* entries = fs_list_directory(path);
//     if (entries == NULL) {
//         printf("Error listing directory contents for '%s'.\n", path);
//         return false;
//     }

//     // Iterate over each entry and remove it if it's a file or call fs_remove_directory recursively if it's a directory
//     for (int i = 0; entries[i].name[0] != '\0'; ++i) {
//         char fullPath[256]; // Ensure this size is appropriate
//         snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entries[i].name);

//         if (entries[i].is_directory) {
//             if (!fs_remove_directory(fullPath)) {
//                 printf("Failed to remove subdirectory: %s\n", fullPath);
//                 // Decide how to handle partial failures. Here we continue attempting to remove other entries
//             }
//         } else {
//             // Assuming fs_rm is your function to remove files
//             if (!fs_rm(fullPath)) {
//                 printf("Failed to remove file: %s\n", fullPath);
//                 // Handle failure
//             }
//         }
//     }

//     // Now, all contents should be removed. Free the directory's block.
//     uint32_t blockIndex = dirEntry->start_block; // Assuming this is how you store the block index for a directory entry
//     if (blockIndex != FAT_NO_FREE_BLOCKS) {
//         fat_free_block(blockIndex); // Assuming this is your function to free a block
//     } else {
//         printf("Error: Invalid block index for directory '%s'.\n", path);
//         // Handle error
//     }

 
//     printf("Directory '%s' removed successfully.\n", path);
//     return true;
// }

bool fs_remove_directory(const char* path) {
    printf("Attempting to remove directory: %s\n", path);

    if (path == NULL || *path == '\0') {
        printf("Error: Path is NULL or empty.\n");
        return false;
    }

    // Check if the directory exists
    DirectoryEntry* dirEntry = find_directory_entry(path);
    if (dirEntry == NULL) {
        printf("Error: Directory '%s' does not exist.\n", path);
        return false;
    }

    // List the directory's contents
    DirectoryEntry* entries = fs_list_directory(path);
    if (entries == NULL) {
        printf("Error listing directory contents for '%s'.\n", path);
        return false;
    }

    // Iterate over each entry and remove it if it's a file or call fs_remove_directory recursively if it's a directory
    for (int i = 0; entries[i].name[0] != '\0'; ++i) {
        char fullPath[256]; // Ensure this size is appropriate
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entries[i].name);

        if (entries[i].is_directory) {
            if (!fs_remove_directory(fullPath)) {
                printf("Failed to remove subdirectory: %s\n", fullPath);
                // Decide how to handle partial failures. Here we continue attempting to remove other entries
            }
        } else {
            // Assuming fs_rm is your function to remove files
            if (!fs_rm(fullPath)) {
                printf("Failed to remove file: %s\n", fullPath);
                // Handle failure
            }
        }
    }

    // Now, all contents should be removed. It's time to free the blocks associated with this directory.
    uint32_t currentBlock = dirEntry->start_block;
    uint32_t nextBlock;
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        if (fat_get_next_block(currentBlock, &nextBlock) != FAT_SUCCESS) {
            printf("Failed to get next block during directory removal.\n");
            // Handle failure
            break;
        }
        fat_free_block(currentBlock); // Free the current block
        currentBlock = nextBlock; // Move to the next block
    }

    // Finally, mark the directory entry as not in use
    memset(dirEntry, 0, sizeof(DirectoryEntry)); // Optional: Clear the directory entry for reuse

    printf("Directory '%s' removed successfully.\n", path);
    return true;
}



bool actual_create_directory(const char* path) {
    printf("DEBUG: Entering actual_create_directory with path: %s\n", path);

    if (path == NULL || *path == '\0') {
        printf("ERROR: Path is NULL or empty in actual_create_directory.\n");
        return false;
    }

    if (directory_exists(path)) {
        printf("DEBUG: Directory already exists: %s\n", path);
        return true; // Directory already exists, treat as a success to support idempotency
    }

    // Allocate a block for the directory. The implementation details of these
    // functions (e.g., fat_allocate_block, initialize_directory_block) are not shown here.
    uint32_t directoryBlock = fat_allocate_block();
    if (directoryBlock == FAT_NO_FREE_BLOCKS) {
        printf("ERROR: Failed to allocate block for directory: %s\n", path);
        return false;
    }

    // Initialize the directory block. This typically involves writing a specific
    // structure or markers to denote a directory.
    if (!initialize_directory_block(directoryBlock)) {
        printf("ERROR: Failed to initialize block for directory: %s\n", path);
        // Attempt to free the block if initialization fails
        fat_free_block(directoryBlock);
        return false;
    }

    // Update the filesystem structure to include the new directory. This might
    // involve updating a parent directory's entries to include this new directory.
    if (!add_directory_entry_to_filesystem(path, directoryBlock)) {
        printf("ERROR: Failed to add directory to filesystem: %s\n", path);
        // Attempt to free the block if adding the directory to the filesystem fails
        fat_free_block(directoryBlock);
        return false;
    }

    printf("SUCCESS: Directory created: %s\n", path);
    return true;
}

 bool add_directory_entry_to_filesystem(const char* path, uint32_t newDirBlock) {
    printf("DEBUG: Adding new directory entry to filesystem for path: '%s', Block: %u\n", path, newDirBlock);

    char parentPath[256] = {0};
    char dirName[FILENAME_MAX] = {0};
    split_path_into_parent_and_dir(path, parentPath, dirName); // Ensure this function is correctly splitting path

    printf("DEBUG: Parent path: '%s', Directory name: '%s'\n", parentPath, dirName);

    // Find the parent directory's DirectoryEntry, assuming a function like find_directory_entry_by_path
    DirectoryEntry* parentDir = find_directory_entry(parentPath);
    if (parentDir == NULL) {
        printf("ERROR: Parent directory '%s' not found.\n", parentPath);
        return false;
    }

    // Ensure there's space in the parent directory for a new entry
    uint8_t* dirBlock = ensure_directory_has_space(parentDir); // Ensure this handles DirectoryEntry*
    if (dirBlock == NULL) {
        printf("ERROR: No space available in parent directory '%s'.\n", parentPath);
        return false;
    }

    // Initialize the new directory entry within the filesystem's directory entry structure
    DirectoryEntry newEntry;
    memset(&newEntry, 0, sizeof(DirectoryEntry));
    strncpy(newEntry.name, dirName, sizeof(newEntry.name) - 1); // Copy directory name
    newEntry.is_directory = true; // Set as directory
    newEntry.start_block = newDirBlock; // Set the start block
    // You might also want to initialize other fields of newEntry as necessary

    // Add the new directory entry to the parent directory
    if (!add_entry_to_directory_block(dirBlock, &newEntry)) { // Ensure this function can handle DirectoryEntry*
        printf("ERROR: Failed to add new directory entry '%s' to parent directory '%s'.\n", dirName, parentPath);
        return false;
    }

    printf("SUCCESS: New directory '%s' added under parent directory '%s'.\n", dirName, parentPath);
    return true;
}



bool directory_exists(const char* path) {
    // Use 'find_directory_entry' to search for a directory by its path.
    // This function should return a pointer to a DirectoryEntry if the directory exists, otherwise NULL.
    DirectoryEntry* dirEntry = find_directory_entry(path);

    // If 'dirEntry' is not NULL, then the directory exists.
    if (dirEntry != NULL) {
        return true;
    }

    // If we reach here, it means the directory does not exist.
    return false;
}


 
DirectoryEntry* find_directory_entry(const char* path) {
    // Special handling for the root directory if necessary.
    if (strcmp(path, "/") == 0) {
        // Assuming the first entry is reserved for the root directory,
        // or you have a predefined way to identify the root directory.
        return &dirEntries[0];
    }

    // Search through the directory entries for a match.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        // Check if the entry is used, is a directory, and the name matches the path.
        if (dirEntries[i].is_directory && strcmp(dirEntries[i].name, path) == 0) {
            // Found the directory entry that matches the path.
            return &dirEntries[i];
        }
    }

    // If the function reaches here, the directory was not found.
    return NULL;
}

uint8_t* ensure_directory_has_space(DirectoryEntry* directory) {
    if (!directory) {
        printf("DEBUG: Directory is NULL.\n");
        fflush(stdout);
        return NULL; // Input must be a valid directory
    }
    printf("DEBUG: Ensuring directory '%s' has space. Directory start block: %u\n", directory->name, directory->start_block);

    // Calculate the last block of the directory by following the FAT chain
    uint32_t currentBlock = directory->start_block;
    uint32_t lastBlock = currentBlock;
    uint32_t nextBlock;
    while (fat_get_next_block(currentBlock, &nextBlock) == FAT_SUCCESS && nextBlock != FAT_ENTRY_END) {
        lastBlock = nextBlock;
        currentBlock = nextBlock;
    }

    // Allocate a buffer for the last block to check for available space
    uint8_t* blockBuffer = (uint8_t*)malloc(FILESYSTEM_BLOCK_SIZE);
    if (!blockBuffer) {
        printf("ERROR: Memory allocation for blockBuffer failed.\n");
        fflush(stdout);
        return NULL;
    }

    // Read the last block to determine the number of entries it contains
    flash_read_safe(FLASH_TARGET_OFFSET + lastBlock * FILESYSTEM_BLOCK_SIZE, blockBuffer, FILESYSTEM_BLOCK_SIZE);
    int entryCount = get_directory_entry_count(blockBuffer); // This function needs to be tailored to your directory structure
    printf("DEBUG: Directory '%s' entry count in the last block: %d\n", directory->name, entryCount);

    if (entryCount < ENTRIES_PER_BLOCK) {
        // Space is available in the current last block for a new entry
        printf("DEBUG: Space available in current block. Preparing next free entry slot.\n");
        uint8_t* nextFreeEntry = blockBuffer + entryCount * sizeof(DirectoryEntry); // Calculate the pointer to the next free entry slot
        return nextFreeEntry; // Caller is responsible for freeing blockBuffer after use
    } else {
        // Allocate a new block for additional directory entries if the current block is full
        printf("DEBUG: Current block is full. Allocating new block for directory entries.\n");
        uint32_t newBlock = fat_allocate_block(); // Function to allocate a new block
        if (newBlock == FAT_NO_FREE_BLOCKS) {
            printf("ERROR: Failed to allocate new block for directory entries.\n");
            free(blockBuffer); // Free buffer on failure
            return NULL;
        }

        // Initialize the new block for directory usage
        if (!initialize_directory_block(newBlock)) {
            printf("ERROR: Failed to initialize new block for directory entries.\n");
            free(blockBuffer);
            return NULL;
        }

        // Link the newly allocated block to the directory's block chain
        fat_link_blocks(lastBlock, newBlock); // This function links two blocks in the FAT chain

        memset(blockBuffer, 0, FILESYSTEM_BLOCK_SIZE); // Clear the buffer for the new block
        printf("DEBUG: New block %u allocated and initialized for directory '%s'.\n", newBlock, directory->name);
        return blockBuffer; // The caller should free this buffer after use
    }
}

 
bool add_entry_to_directory_block(uint8_t* dirBlock, DirectoryEntry* newEntry) {
    if (dirBlock == NULL || newEntry == NULL) {
        printf("Invalid parameters to add_entry_to_directory_block.\n");
        return false;
    }

    // Calculate how many entries are already in the block
    int entriesInBlock = 0;
    for (int i = 0; i < ENTRIES_PER_BLOCK; ++i) {
        DirectoryEntry* currentEntry = (DirectoryEntry*)(dirBlock + i * sizeof(DirectoryEntry));
        // Assuming that an unused entry is indicated by the first byte being 0 (e.g., name[0] == '\0')
        if (currentEntry->name[0] == '\0') {
            break; // Found an unused entry, so stop counting
        }
        entriesInBlock++;
    }

    // Check if there's room for another entry
    if (entriesInBlock >= ENTRIES_PER_BLOCK) {
        printf("The directory block is full. Cannot add another entry.\n");
        return false;
    }

    // There's room; add the new entry to the block
    DirectoryEntry* targetEntry = (DirectoryEntry*)(dirBlock + entriesInBlock * sizeof(DirectoryEntry));
    memcpy(targetEntry, newEntry, sizeof(DirectoryEntry));

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
        strcpy(parentPath, "/");
        strcpy(dirName, fullPath);
        printf("DEBUG: parentPath: '%s', dirName: '%s'\n", parentPath, dirName);
        return;
    }

    // Special case for root directory
    if (lastSlash == pathCopy) {
        printf("DEBUG: Root directory case encountered.\n");
        strcpy(parentPath, "/");
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



// Helper Function to Read Directory Block into Buffer
// Assumes `flash_read_safe` is a function that reads data from flash memory.
void read_directory_block(uint32_t block_number, uint8_t *buffer, size_t buffer_size) {
    // Check if block_number is within the range of valid blocks
    if (block_number >= TOTAL_BLOCKS) {
        printf("Error: Block number %u is out of range.\n", block_number);
        fflush(stdout);
        return;
    }

    // Check if the block is marked as reserved or invalid, in addition to being free
    if (FAT[block_number] == FAT_ENTRY_FREE || FAT[block_number] == FAT_ENTRY_RESERVED || FAT[block_number] == FAT_ENTRY_INVALID) {
        printf("Error: Block number %u is marked as free, reserved, or invalid.\n", block_number);
        fflush(stdout);
        return;
    }

    // Ensure the buffer_size does not exceed the FILESYSTEM_BLOCK_SIZE to prevent overreads
    if (buffer_size > FILESYSTEM_BLOCK_SIZE) {
        printf("Warning: Requested buffer_size exceeds FILESYSTEM_BLOCK_SIZE. Adjusting to FILESYSTEM_BLOCK_SIZE.\n");
        fflush(stdout);
        buffer_size = FILESYSTEM_BLOCK_SIZE;
    }

    // Calculate the physical address in flash memory for the start of the user data
    // It's essential to ensure that this calculation does not result in an address outside the usable flash space
    uint32_t address = FLASH_TARGET_OFFSET + (block_number * FILESYSTEM_BLOCK_SIZE);

    // Ensure that the address + buffer_size does not exceed the flash memory bounds
    if (address + buffer_size > FLASH_MEMORY_SIZE_BYTES) {
        printf("Error: Reading beyond the flash memory bounds.\n");
        fflush(stdout);
        return;
    }

    // Perform the safe read operation from the calculated address into the provided buffer
    flash_read_safe(address, buffer, buffer_size);
}



int get_directory_entry_count(const uint8_t* blockBuffer) {
    int count = 0;

    // Iterate over each possible directory entry in the block
    for (int i = 0; i < ENTRIES_PER_BLOCK; i++) {
        // Calculate the pointer to the current entry based on its index
        const DirectoryEntry* entry = (const DirectoryEntry*)(blockBuffer + i * sizeof(DirectoryEntry));

        // Assuming an unused entry is indicated by the first character of the name being '\0'
        if (entry->name[0] == '\0') {
            break; // Found an unused entry, stop counting
        }
        count++;
    }

    return count;
}



DirectoryEntry* create_new_directory_entry(const char* path) {
    if (path == NULL || *path == '\0') {
        printf("Invalid path provided.\n");
        return NULL;
    }

    // Find a free directory entry
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].is_directory) {
            // Initialize the directory entry
            strncpy(dirEntries[i].name, path, sizeof(dirEntries[i].name) - 1);
            dirEntries[i].name[sizeof(dirEntries[i].name) - 1] = '\0'; // Ensure null termination
            dirEntries[i].is_directory = true;
            dirEntries[i].start_block = fat_allocate_block();
            if (dirEntries[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Failed to allocate block for new directory.\n");
                memset(&dirEntries[i], 0, sizeof(DirectoryEntry)); // Clear the entry
                return NULL;
            }

            if (!initialize_directory_block(dirEntries[i].start_block)) {
                printf("Failed to initialize directory block.\n");
                fat_free_block(dirEntries[i].start_block); // Free the block
                memset(&dirEntries[i], 0, sizeof(DirectoryEntry)); // Clear the entry
                return NULL;
            }

            dirEntries[i].size = 0; // Directory size initialization, if applicable
            printf("New directory entry created for '%s'\n", path);
            return &dirEntries[i];
        }
    }

    printf("No free directory entries available.\n");
    return NULL;
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

 void populate_directory_entries_from_block(DirectoryEntry* dir, DirectoryEntry* entries) {
    if (!dir || !entries) {
        printf("Invalid arguments provided.\n");
        return;
    }

    // Assuming FILESYSTEM_BLOCK_SIZE is the size of each block on your filesystem
    uint8_t blockBuffer[FILESYSTEM_BLOCK_SIZE];
    uint32_t address = FLASH_TARGET_OFFSET + dir->start_block * FILESYSTEM_BLOCK_SIZE;
    
    // Read the block from flash memory
    flash_read_safe(address, blockBuffer, FILESYSTEM_BLOCK_SIZE);
    
    // First 4 bytes at the start of the block indicate the number of entries
    uint32_t* entryCountPtr = (uint32_t*)blockBuffer;
    uint32_t entryCount = *entryCountPtr;
    
    // Ensure entryCount is within expected bounds to avoid overruns
    if (entryCount > MAX_DIRECTORY_ENTRIES) {
        printf("Directory entry count exceeds maximum allowed entries.\n");
        return;
    }

    // Calculate the start of the first directory entry
    uint8_t* entryDataPtr = blockBuffer + sizeof(uint32_t); // Skip past the entry count
    
    for (uint32_t i = 0; i < entryCount; ++i) {
        // Assuming the serialized DirectoryEntry fits within the block
        if (i * sizeof(DirectoryEntry) + sizeof(uint32_t) < FILESYSTEM_BLOCK_SIZE) {
            // Copy the serialized entry into the provided entries array
            memcpy(&entries[i], entryDataPtr + i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
        } else {
            printf("Error: Directory entry offset exceeds block size.\n");
            return;
        }
    }

    printf("Populated %u directory entries from block starting at address %u.\n", entryCount, address);
}














void list_directory_contents(const char* path, int indentLevel) {
    DirectoryEntry* entries = fs_list_directory(path);
    if (entries == NULL) {
        printf("Could not list directory contents for '%s'\n", path);
        return;
    }

    for (int i = 0; entries[i].name[0] != '\0'; i++) {
        // Print indent based on level in the hierarchy
        for (int j = 0; j < indentLevel; j++) {
            printf("  ");
        }
        
        printf("%s (%s)\n", entries[i].name, entries[i].is_directory ? "Directory" : "File");

        // If the entry is a directory, recurse into it
        if (entries[i].is_directory) {
            char newPath[256]; // Ensure this is large enough for your paths
            snprintf(newPath, sizeof(newPath), "%s/%s", path, entries[i].name);
            list_directory_contents(newPath, indentLevel + 1);
        }
    }
}

void fs_list_all() {
    printf("Listing all files and directories in the filesystem:\n");
    list_directory_contents("/", 0); // Start from the root directory
}









// 1. fs_create_directory

//     Current Behavior: This function manages directory creation, including path parsing and directory entry initialization. However, it might directly manipulate blocks without leveraging the full capabilities of the FAT system.

//     Improvement: Use FAT functions for block allocation (fat_allocate_block) and initialization (initialize_directory_block). Ensure that directory creation directly leverages the FAT system for managing the physical storage aspects.

// 2. fs_list_directory

//     Current Behavior: Lists the contents of a directory by reading directory blocks.

//     Improvement: While listing, ensure that any manipulation or traversal of blocks uses FAT functions like fat_get_next_block to follow the chain of blocks belonging to a directory. This would help in cases where a directory spans multiple blocks.

// 3. reset_root_directory

//     Current Behavior: Resets or initializes the root directory, including allocating a new block if necessary.

//     Improvement: When allocating a new block for the root directory or any directory restructuring, ensure to use fat_allocate_block for allocation and fat_link_blocks for linking the new block to the existing directory structure if extending.

// 4. ensure_directory_has_space

//     Current Behavior: Checks for available space in the directory's current block and allocates a new block if needed.

//     Improvement: Simplify this by more effectively using fat_allocate_block and fat_link_blocks. The detailed handling of block allocation and linking should be managed within the FAT system, and this function should primarily check directory-specific conditions, deferring block operations to fat_fs.c.

// 5. fs_remove_directory and actual_create_directory

//     Current Behavior: Manages removing a directory and creating a new directory, potentially involving block freeing and allocation.

//     Improvement: For removal, use fat_free_block to release blocks used by the directory. For creation, use fat_allocate_block to get a new block and initialize_directory_block to properly set it up. Both processes should rely on FAT operations for managing the block-level details.

// General Enhancement Strategy:

//     Block Allocation and Freeing: Anytime a block is needed or to be released, use fat_allocate_block and fat_free_block. This ensures that all block management respects the filesystem's current state and policies, including handling fragmentation and retries.

//     Directory Chain Management: When a directory needs to span multiple blocks, leverage fat_link_blocks for linking and fat_get_next_block for traversal. This will simplify the logic needed in directory management functions and ensure consistency in how block chains are handled.

//     Error Handling and Integrity Checks: Leverage the FAT system's error handling and block status checks before performing operations. This ensures that directory management does not operate on blocks in invalid states.

// By adjusting these functions to integrate more deeply with the FAT system, the Directories.c module can become more robust, easier to maintain, and more aligned with the filesystem's overall architecture.