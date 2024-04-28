#include <string.h>
#include <stdlib.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
#include "../filesystem/filesystem_helper.h" 

#include "../directory/directory_helpers.h"



 


DirectoryEntry* createDirectoryEntry(const char* path) {
    printf("debug createDirectoryEntry for path: %s\n", path);
    uint32_t saved_irq_status = save_and_disable_interrupts(); // Save and disable interrupts
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].in_use) {
            uint32_t rootDirId = get_root_directory_id();
            strncpy(dirEntries[i].name, path, sizeof(fileSystem[i].filename) - 1);
            dirEntries[i].parentDirId = rootDirId;
            dirEntries[i].currentDirId = generateUniqueId();
            dirEntries[i].is_directory = false; 
            dirEntries[i].start_block = fat_allocate_block();
            dirEntries[i].in_use = true;
            dirEntries[i].size = 0;
            dirEntries[i].is_directory = true;

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
                restore_interrupts(saved_irq_status); // Restore interrupts
                return NULL;
            }
            restore_interrupts(saved_irq_status); // Restore interrupts
            return &dirEntries[i];
        }
    }
    printf("Error: dirEntries is full, cannot create new file.\n");
    fflush(stdout);
    restore_interrupts(saved_irq_status); // Restore interrupts
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
    printf("Root directory ID get_root_directory_id: %u\n", rootDirId);
    // Free the directory entry if your system allocates memory dynamically in DIR_find_directory_entry

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
            printf("Directory entry index: %d\n", i);
            printf("in_use: %d\n", dirEntries[i].in_use);
            printf("is_directory: %d\n", dirEntries[i].is_directory);
            fflush(stdout);
        }
    }
}

 
void DIR_all_directory_entriesEX(void) {
    printf("\n\nENTERED DIR_all_directory_entries\n");
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        
            printf("\n\nEnrty %d is a directory\n", i);
            printf("Directory entry %d: %s\n", i, dirEntries[i].name);
            printf("Parent Directory ID: %u\n", dirEntries[i].parentDirId);
            printf("Current Directory ID: %u\n", dirEntries[i].currentDirId);
            printf("Start block: %u\n", dirEntries[i].start_block);
            printf("Directory size: %u\n", dirEntries[i].size);
            printf("Directory entry index: %d\n", i);
            printf("in_use: %d\n", dirEntries[i].in_use);
            printf("is_directory: %d\n", dirEntries[i].is_directory);
            fflush(stdout);
        
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
    printf("Directory name: %s\n", directoryName);
    fflush(stdout);
    if(directoryName == NULL) {
        printf("Directory name is NULL.\n");
        return NULL;
    }

    char path[512]; // Define a sufficiently large buffer for the path
    prepend_slash(directoryName, path, sizeof(path));
    printf("Prepended path: %s\n", path);

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




//first two blocks reserved for this function
void saveDirectoriesEntriesToFileSystem() {
    uint32_t address = 270336; 
    printf("Saving file entries to flash memory...\n");
    uint8_t *serializedData = malloc(sizeof(dirEntries)); 
    memcpy(serializedData, dirEntries, sizeof(dirEntries)); 

    flash_write_safe2(address, serializedData, sizeof(dirEntries));

    free(serializedData);
    printf("File entries saved to flash memory.\n");
}


// Function to load file entries from flash memory into a local array
void loadDirectoriesEntriesFromFileSystem() {
    uint32_t address = 270336; 
    // Local array to hold the recovered file entries
    DirectoryEntry recoverDirSystem[MAX_DIRECTORY_ENTRIES];

    // Read data from flash into the local array
    flash_read_safe2(address, (uint8_t*)recoverDirSystem, sizeof(recoverDirSystem));

    // Optionally, print out the entries to verify correctness
    for (int i = 0; i < MAX_FILES; i++) {
        printf("Recovered File Entry %d: %s\n", i, recoverDirSystem[i].name);
    }

    // Here you can add logic to process the loaded data if needed
}

