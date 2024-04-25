#include <string.h>
#include <stdlib.h>


#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
#include "../filesystem/filesystem_helper.h" 

#include "../directory/directory_helpers.h"



 


DirectoryEntry* createDirectoryEntry(const char* path) {
    printf("debug createDirectoryEntry for path: %s\n", path);
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].in_use) {
            uint32_t rootDirId = get_root_directory_id();
            strncpy(dirEntries[i].name, path, sizeof(dirEntries[i].name) - 1);
            dirEntries[i].parentDirId = rootDirId;
            dirEntries[i].currentDirId = generateUniqueId();
            dirEntries[i].is_directory = false; 
            dirEntries[i].start_block = fat_allocate_block();
            dirEntries[i].in_use = true;
            dirEntries[i].size = 0;

            printf("New directory created: %s\n", dirEntries[i].name);
            printf("Start block: %u\n", dirEntries[i].start_block);
            printf("Directory size: %u\n", dirEntries[i].size);
            printf("Directory entry index: %d\n", i);
            printf("Parent Directory ID: %u\n", dirEntries[i].parentDirId);
            printf("Current Directory ID: %u\n", dirEntries[i].currentDirId);
            fflush(stdout);
            
            fflush(stdout);
            if (dirEntries[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left on device to create new file.\n");
                fflush(stdout);
                memset(&dirEntries[i], 0, sizeof(FileEntry)); // Cleanup
                dirEntries[i].in_use = false; // Explicitly mark it as not in use
                return NULL;
            }
            return &dirEntries[i];
        }
    }
    printf("Error: dirEntries is full, cannot create new file.\n");
    fflush(stdout);
    return NULL;
}
 
 

 
 
 

 
 
/**
 * Gets the ID of the root directory.
 * @return The ID of the root directory, or 0 if there was an error.
 */
uint32_t get_root_directory_id() {
    // Find the directory entry for the root directory
    DirectoryEntry* dirEntry = DIR_find_directory_entry("/root");
    if (dirEntry == NULL) {
        printf("Failed to find the root directory.\n");
        return 0;  // Return 0 as an error indicator
    }

    // Store the ID of the root directory
    uint32_t rootDirId = dirEntry->currentDirId;

    // Free the directory entry if your system allocates memory dynamically in DIR_find_directory_entry
    free(dirEntry);  // Ensure to free this memory to prevent memory leaks

    return rootDirId;
}




 
void DIR_all_directory_entries(void) {
    printf("\n\nENTERED DIR_all_directory_entries\n");
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (dirEntries[i].in_use && dirEntries[i].is_directory) {
            printf("\n\nEnrty %d is a directory\n", i);
            printf("Directory entry %d: %s\n", i, dirEntries[i].name);
            printf("Parent Directory ID: %u\n", dirEntries[i].parentDirId);
            printf("Current Directory ID: %u\n", dirEntries[i].currentDirId);
            printf("Start block: %u\n", dirEntries[i].start_block);
            printf("Directory size: %u\n", dirEntries[i].size);
            fflush(stdout);
        }
    }
}


 
DirectoryEntry* find_free_directory_entry(void) {
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].in_use) {
            // Found an unused directory entry
            return &dirEntries[i];
        }
    }
    return NULL;
} 

DirectoryEntry* DIR_find_directory_entry(const char* directoryName) {
    printf("\n\nENTERED DIR_find_directory_entry\n");
    fflush(stdout);

    const char* path = prepend_slash(directoryName);

    for (uint32_t i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (dirEntries[i].in_use && dirEntries[i].is_directory && strcmp(dirEntries[i].name, path) == 0) {
            printf("Directory entry found: %s\n", path);
            fflush(stdout);
            return &dirEntries[i]; // Return a pointer to the existing entry
        }
    }
    fflush(stdout);
    return NULL;
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
