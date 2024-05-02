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



/**
 * Initializes all directory entries in the global directory entries array.
 * This function is typically called at the start of the program or when resetting
 * the directory entries to ensure all entries are in a consistent, default state.
 * It sets each directory entry's properties to indicate that they are not in use.
 */
void init_directory_entries() {
    // Iterate through each directory entry in the global directory entries array.
    for (int i = 0; i < MAX_DIRECTORY_ENTRIES; i++) {
        // Set the 'in use' flag to false to indicate the directory entry is not currently used.
        dirEntries[i].in_use = false;

        // Set the 'is directory' flag to false as default, although this might be set true later when the directory is actually used.
        dirEntries[i].is_directory = false;

        // Initialize the name of the directory to an empty string indicating it is unused.
        dirEntries[i].name[0] = '\0';

        // Set the parent directory ID to 0, indicating no parent (useful for root or unused entries).
        dirEntries[i].parentDirId = 0;

        // Set the current directory ID to 0, also indicating no current directory association.
        dirEntries[i].currentDirId = 0;

        // Initialize the size of the directory to 0, typically used to count number of files or total size.
        dirEntries[i].size = 0;

        // Initialize the start block to 0, which would be used to identify the start of directory data in storage.
        dirEntries[i].start_block = 0;
    }
}



 
/**
 * Attempts to create a new directory with the specified name if it doesn't already exist.
 * This function checks for the existence of a directory before attempting to create it,
 * ensuring that directory names are unique within the filesystem.
 *
 * @param directory The path of the directory to create.
 * @return Returns true if the directory was successfully created, false otherwise.
 */
bool fs_create_directory(const char* directory) {
    // Check if the provided directory path is NULL or empty, which is not allowed.
    if (directory == NULL || *directory == '\0') {
        printf("ERROR: Path is NULL or empty.\n");
        return false;  // Return false indicating failure to proceed with an invalid path.
    }

    // Attempt to find an existing directory entry with the same name to prevent duplicates.
    DirectoryEntry* existingDir = DIR_find_directory_entry(directory);
    if (existingDir != NULL) {
        // If the directory already exists, log an error and prevent creation of a duplicate.
        printf("ERROR: Directory already exists: %s\n", directory);
        return false;  // Return false as the directory cannot be created again.
    }

    // If the directory does not exist, proceed to create a new directory entry.
    DirectoryEntry* entry = createDirectoryEntry(directory);
    if (entry == NULL) {
        // Log an error if creating the directory entry failed.
        printf("Error: Failed to create directory entry for '%s'.\n", directory);
        fflush(stdout);  // Ensure that the error message is output immediately.
        return false;  // Return false indicating that the directory entry creation failed.
    }

    // Calculate the offset in the flash memory where this directory entry should be written.
    uint32_t offsetInBytes = entry->start_block * FILESYSTEM_BLOCK_SIZE;

    // Write the directory entry to flash storage at the calculated offset.
    flash_write_safe(offsetInBytes, (const uint8_t*)entry, sizeof(DirectoryEntry));

    // Log a success message indicating that the directory was successfully created.
    printf("SUCCESS: Directory created: %s\n", directory);
    return true;  // Return true indicating successful directory creation.
}




 



  
/**
 * Resets or initializes the root directory in the filesystem.
 * This function checks if the root directory is valid and, if not, reinitializes it.
 * It ensures that the filesystem is initialized before proceeding and that there are
 * free blocks and directory entries available for use.
 *
 * @return True if the root directory is successfully validated or reset, false otherwise.
 */
bool reset_root_directory(void) {
    printf("Resetting root directory...\n");

    // Check if the filesystem is initialized before attempting any operations.
    if (!fs_initialized) {
        printf("Filesystem not initialized. Cannot reset root directory.\n");
        return false; // Return false if the filesystem is not ready for operations.
    }

    // Check if the first directory entry is the root directory and it is in use.
    if (dirEntries[0].is_directory && strcmp(dirEntries[0].name, "/root") == 0 && dirEntries[0].in_use) {
        // Perform an integrity check on the existing root directory.
        if (is_directory_valid(&dirEntries[0])) {
            printf("Root directory is valid. No reset needed.\n");
            return true; // Return true if the root directory is already valid.
        } else {
            printf("Root directory integrity check failed. Reinitializing...\n");
            // Proceed to reinitialize the root directory if the integrity check fails.
        }
    } else {
        printf("Initializing root directory...\n");
    }

    // Attempt to allocate a new block for the root directory.
    uint32_t rootBlock = fat_allocate_block();
    if (rootBlock == FAT_NO_FREE_BLOCKS) {
        printf("Failed to allocate block for root directory.\n");
        return false; // Return false if no free blocks are available.
    }

    // Attempt to find a free directory entry for the new root directory.
    DirectoryEntry* freeEntry = find_free_directory_entry();
    if (freeEntry == NULL) {
        printf("Failed to find a free directory entry.\n");
        return false; // Return false if no free directory entries are available.
    }

    // Prepare the directory entry with appropriate values.
    freeEntry->is_directory = true;
    strncpy(freeEntry->name, "/root", sizeof(freeEntry->name) - 1);
    freeEntry->name[sizeof(freeEntry->name) - 1] = '\0'; // Ensure null termination.
    freeEntry->parentDirId = generateUniqueId(); // Set a unique ID for the parent directory ID.
    freeEntry->currentDirId = generateUniqueId(); // Set a unique ID for the current directory ID.
    freeEntry->in_use = true;
    freeEntry->start_block = rootBlock;
    freeEntry->size = 0; // Initialize size to 0 for directories.

    printf("Root directory (re)initialized at block %u.\n", rootBlock);
    uint32_t flashAddress = rootBlock * FILESYSTEM_BLOCK_SIZE;

    // Uncomment the following line to write the directory entry to flash memory.
    // flash_write_safe(flashAddress, (const uint8_t *)freeEntry, sizeof(DirectoryEntry));
    printf("Root directory entry written to flash.\n");
    fflush(stdout);

    return true; // Return true to indicate successful reset or initialization.
}


 
 
 
  
   




 
 