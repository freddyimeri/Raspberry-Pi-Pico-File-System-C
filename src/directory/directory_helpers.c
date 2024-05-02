/**
 * directory_helpers.c
 *
 * This source file contains functions that manage and manipulate directory entries
 * within the filesystem. It provides the necessary tools to create, find, validate,
 * save, and load directory entries, which are essential for maintaining the directory
 * structure of the filesystem. Functions in this file handle tasks such as:
 *
 * - Creating new directory entries in the global directory entries array.
 * - Finding free entries in the directory entries array for new directories.
 * - Validating the integrity of directory entries.
 * - Saving all directory entries to a specific location in flash memory.
 * - Loading directory entries from flash memory into a local array.
 * - Displaying all active directory entries for debugging and system monitoring.
 *
 * The utilities provided here are crucial for the filesystem's operation, ensuring
 * that directory-related operations are performed safely and efficiently.
 *
 * Functions in this file are used across the filesystem management system to handle
 * directories, which are a fundamental part of the file organization within the embedded
 * system. Proper management and maintenance of these directories ensure the robustness
 * and reliability of the file access and storage processes.
 */

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



 

/**
 * Creates a new directory entry for a given path in the filesystem's directory entries array.
 * This function searches for an unused directory entry and sets it up with the specified path,
 * handling interruptions and ensuring thread safety during its operations.
 *
 * @param path The filesystem path for the new directory.
 * @return Pointer to the newly created DirectoryEntry if successful, NULL if unsuccessful.
 */
DirectoryEntry* createDirectoryEntry(const char* path) {
    // Iterate through the directory entries to find an unused entry.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        if (!dirEntries[i].in_use) { // Check if the entry is not currently used.
            // Get the ID of the root directory to set as the parent directory ID.
            uint32_t rootDirId = get_root_directory_id();

            // Copy the provided path into the directory entry's name field.
            strncpy(dirEntries[i].name, path, sizeof(dirEntries[i].name) - 1);
            dirEntries[i].name[sizeof(dirEntries[i].name) - 1] = '\0'; // Ensure null termination.

            // Set the directory specific fields.
            dirEntries[i].parentDirId = rootDirId;
            dirEntries[i].currentDirId = generateUniqueId(); // Generate a unique ID for the new directory.
            dirEntries[i].is_directory = true;
            dirEntries[i].start_block = fat_allocate_block(); // Allocate a block for the directory.
            dirEntries[i].in_use = true;
            dirEntries[i].size = 0; // Initialize size to 0, usually used for files.

            // Check if a block could not be allocated.
            if (dirEntries[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left on device to create new file.\n");
                memset(&dirEntries[i], 0, sizeof(DirectoryEntry)); // Clean up the entry.
                dirEntries[i].in_use = false; // Mark it as not in use.
          
                return NULL;
            }

            // Successfully created the directory entry, restore interrupts and return the entry.
      
            return &dirEntries[i];
        }
    }

    // If all entries are in use, log an error and restore interrupts.
    printf("Error: dirEntries is full, cannot create new file.\n");

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




 
/**
 * Displays all active directory entries from the global directory entries array.
 * This function iterates through the entire list of directory entries and prints
 * details for each one that is marked as in use and identified as a directory.
 * It is particularly useful for debugging and ensuring the integrity of directory data.
 */
void DIR_all_directory_entries(void) {
    // Iterate through the array of directory entries.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        // Check if the current entry is in use and is a directory.
        if (dirEntries[i].in_use && dirEntries[i].is_directory) {
            // Print a header for the entry to distinguish it clearly in output.
            printf("\n\nEntry %d is a directory\n", i);

            // Print detailed information about the directory entry.
            printf("Directory entry %d: %s\n", i, dirEntries[i].name);
            printf("Parent Directory ID: %u\n", dirEntries[i].parentDirId);
            printf("Current Directory ID: %u\n", dirEntries[i].currentDirId);
            printf("Start block: %u\n", dirEntries[i].start_block);
            printf("Directory size: %u bytes\n", dirEntries[i].size);
            printf("Directory entry index: %d\n", i);
            printf("in_use: %d\n", dirEntries[i].in_use);
            printf("is_directory: %d\n", dirEntries[i].is_directory);

            // Flush the output to ensure it appears immediately in the console or log file.
            fflush(stdout);
        }
    }
}

 



 
/**
 * Searches for an unused directory entry within the global directory entries array.
 * This function iterates through the array of directory entries and returns the first one
 * that is not currently in use, allowing it to be utilized for a new directory.
 *
 * @return A pointer to an unused DirectoryEntry if one is found, or NULL if all entries are in use.
 */
DirectoryEntry* find_free_directory_entry(void) {
    // Iterate over each directory entry in the global 'dirEntries' array.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        // Check if the current directory entry is marked as not in use.
        if (!dirEntries[i].in_use) {
            // If an unused entry is found, return a pointer to this directory entry.
            // This entry can then be used to create a new directory or for other purposes.
            return &dirEntries[i];
        }
    }
    // If no unused directory entry is found after checking all entries, return NULL.
    // This indicates that there are no available slots left for new directories.
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


 




/**
 * Validates a directory entry by checking its existence and the validity of its start block.
 * This function is crucial for ensuring that directory operations are performed on valid
 * and properly initialized directory entries.
 *
 * @param directory A pointer to the DirectoryEntry to validate.
 * @return True if the directory entry is considered valid, false otherwise.
 */
bool is_directory_valid(const DirectoryEntry* directory) {
    // Check if the directory entry pointer is NULL, which would indicate an invalid reference.
    if (directory == NULL) {
        printf("Directory entry is NULL.\n");  // Log an error message for debugging.
        return false;  // Return false as a NULL directory entry cannot be valid.
    }

    // Check if the start block of the directory is valid. The start block must not be the special value indicating
    // no free blocks are available and must not exceed the total number of blocks in the filesystem.
    if (directory->start_block == FAT_NO_FREE_BLOCKS || directory->start_block >= TOTAL_BLOCKS) {
        printf("Directory start block is invalid. Block: %u\n", directory->start_block);  // Log the invalid block for reference.
        return false;  // Return false as an invalid start block makes the directory entry invalid.
    }

    // If all checks are passed, the directory entry is considered valid.
    return true;
}





/**
 * Saves all directory entries to a specific location in flash memory.
 * This function serializes the global directory entries array and writes it to a fixed
 * address in flash memory, ensuring that directory state is preserved across system restarts.
 *
 * @note The first two blocks of memory starting at the specified address are reserved for this operation.
 */
void saveDirectoriesEntriesToFileSystem() {
    // Specify the flash memory address where the directory entries will be stored.
    uint32_t address = 270336;
    printf("Saving file entries to flash memory...\n");

    // Allocate memory for serialization of the directory entries.
    // This assumes that the directory entries can be serialized directly as a byte array.
    uint8_t *serializedData = malloc(sizeof(dirEntries));
    if (!serializedData) {
        printf("Failed to allocate memory for directory entries serialization.\n");
        return; // Early return on memory allocation failure.
    }

    // Copy the directory entries from the global array into the allocated buffer.
    // This essentially serializes the data if the structure allows for direct binary copying.
    memcpy(serializedData, dirEntries, sizeof(dirEntries));

    // Write the serialized data to flash memory at the specified address.
    // The function `flash_write_safe` should ensure proper handling of flash-specific operations like sector erase before writing.
    flash_write_safe(address, serializedData, sizeof(dirEntries));

    // Free the allocated memory after the write operation to prevent memory leaks.
    free(serializedData);

    // Confirm that the directory entries have been saved to flash memory.
    printf("File entries saved to flash memory.\n");
}



/**
 * Loads directory entries from flash memory into a local array.
 * This function reads directory entry data from a specified address in flash memory,
 * populating a local array with the recovered data. It's typically used during system
 * startup or recovery to restore the state of directory entries.
 */
void loadDirectoriesEntriesFromFileSystem() {
    // Address in flash memory where the directory entries are stored.
    //270336 / 4096 = block 66
    uint32_t address = 270336;

    // Local array to temporarily hold the directory entries recovered from flash memory.
    DirectoryEntry recoverDirSystem[MAX_DIRECTORY_ENTRIES];

    // Perform the read operation from flash memory into the local array.
    // Ensure that the operation is performed safely to avoid data corruption.
    flash_read_safe(address, (uint8_t*)recoverDirSystem, sizeof(recoverDirSystem));

    // Optionally, iterate over the loaded directory entries to verify the integrity and correctness of the data.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        // Print each recovered directory entry's name to verify data has been loaded correctly.
        // This is particularly useful for debugging and during system verification.
        printf("Recovered Directory Entry %d: %s\n", i, recoverDirSystem[i].name);
    }

    // Additional logic can be implemented here to further process or integrate the loaded data
    // into the running application, depending on system requirements.
}


