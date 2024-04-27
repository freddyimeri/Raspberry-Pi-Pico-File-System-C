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
DirectoryEntry dirEntries[MAX_DIRECTORY_ENTRIES];





void init_directory_entries() {
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        dirEntries[i].in_use = false;
        dirEntries[i].is_directory = false;
        dirEntries[i].name[0] = '\0';  // Empty string indicates unused
        dirEntries[i].parentDirId = 0;
        dirEntries[i].currentDirId = 0;
        dirEntries[i].size = 0;
        dirEntries[i].start_block = 0;
    }
}


 
 bool fs_create_directory(const char* directory) {
    if (directory == NULL || *directory == '\0') {
        printf("ERROR: Path is NULL or empty.\n");
        return false;
    }

    // Use find_directory_entry instead of createDirectoryEntry to check if the directory exists
    DirectoryEntry* existingDir = DIR_find_directory_entry(directory);
    if (existingDir != NULL) {
        printf("ERROR: Directory already exists: %s\n", directory);
        return false;
    }

    // Create the directory entry
    DirectoryEntry* entry = createDirectoryEntry(directory);
    if (entry == NULL) {
        printf("Error: Failed to create directory entry for '%s'.\n", directory);
        fflush(stdout);
        return false;
    }

    // Print the directory entry details
    printf("\n\nDetail of of dire entry");
    printf("Directory start block: %u\n", entry->start_block);
    printf("Directory size: %u\n", entry->size);
    printf("Directory parent ID: %u\n", entry->parentDirId);
    printf("Directory current ID: %u\n", entry->currentDirId);
    printf("Directory is_directory: %d\n", entry->is_directory);
    printf("Directory in_use: %d\n", entry->in_use);
    printf("Directory name: %s\n", entry->name);
    fflush(stdout);

    // Compute the offset in bytes and write the directory entry to flash storage
    uint32_t offsetInBytes = entry->start_block * FILESYSTEM_BLOCK_SIZE;
    flash_write_safe2(offsetInBytes, (const uint8_t*)entry, sizeof(DirectoryEntry));
    printf("SUCCESS: Directory created: %s\n", directory);
    return true;
}



 



  

bool reset_root_directory(void) {
    printf("Resetting root directory...\n");
    // Ensure the filesystem has been initialized before attempting to reset the root directory
    if (!fs_initialized) {
        printf("Filesystem not initialized. Cannot reset root directory.\n");
        return false; // Filesystem must be initialized first
    }
    
 
    // to do , use the DIR_find_directory_entry to fix this 
    if (dirEntries[0].is_directory && strcmp(dirEntries[0].name, "/root") == 0 && dirEntries[0].in_use) {
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

 
    DirectoryEntry* freeEntry = find_free_directory_entry();
    if (freeEntry == NULL) {
        printf("Failed to find a free directory entry.\n");
        return false; // Handle free entry not found
    }


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

 
 
 
  
   




 
 