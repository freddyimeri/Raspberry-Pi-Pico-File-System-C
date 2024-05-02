#include <string.h>
#include <stdlib.h>
#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
#include "../directory/directory_helpers.h"
#include "../HighLevelAPI/visual.h"

/**
 * Prints the contents of a directory and recursively prints its subdirectories and files.
 * This function handles directory traversal up to a certain depth to prevent stack overflow
 * and ensures that each directory entry is only visited once.
 *
 * @param parentDirId The ID of the current directory to print.
 * @param level The current depth in the directory tree for proper indentation.
 * @param visited An array to keep track of visited directories to avoid infinite recursion.
 */
void print_directory(uint32_t parentDirId, int level, bool *visited) {
    // Prevent going too deep to avoid stack overflow.
    if (level > 30) {
        print_indent(level);
        printf("Maximum recursion level reached, stopping to prevent stack overflow.\n");
        return;
    }

    // Iterate through the FAT to find directory entries.
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END && !visited[i]) {
            DirectoryEntry dirEntry;
            flash_read_safe(i * FILESYSTEM_BLOCK_SIZE, (uint8_t*)&dirEntry, sizeof(DirectoryEntry));

            // Check if the directory entry is in use and matches the parent ID.
            if (dirEntry.in_use && dirEntry.is_directory && dirEntry.parentDirId == parentDirId) {
                print_indent(level); // Indent the output according to the current directory level.
                printf("--%s\n", dirEntry.name); // Print the directory name.
                visited[i] = true; // Mark this directory as visited.
                print_directory(dirEntry.currentDirId, level + 1, visited); // Recursively print subdirectories.

                // Print files within the directory.
                for (int j = 0; j < MAX_FILES; j++) {
                    if (fileSystem[j].in_use && !fileSystem[j].is_directory && fileSystem[j].parentDirId == dirEntry.currentDirId) {
                        print_indent(level + 1);
                        printf("--%s\n", fileSystem[j].filename); // Print each file name.
                    }
                }
            }
        }
    }
}

/**
 * Prints indentation for directory levels, enhancing the visual structure of the output.
 *
 * @param level The level of indentation required.
 */
void print_indent(int level) {
    for (int k = 0; k < level; k++) {
        printf("\t"); // Print a tab for each level of depth.
    }
}

/**
 * Initiates the process of printing the entire filesystem's structure starting from the root.
 */
void print_filesystem_structure() {
    printf("Filesystem Structure:\n");
    bool visited[TOTAL_BLOCKS] = {false}; // Initialize all entries in the visited array to false.

    // Find the root directory to start the printing process.
    const char* parentPath = "/root";
    DirectoryEntry* rootDirEntry = DIR_find_directory_entry(parentPath);
    if (rootDirEntry) {
        uint32_t rootDirId = rootDirEntry->currentDirId;
        print_directory(rootDirId, 0, visited); // Start recursive print from the root.
        free(rootDirEntry); // Free memory if your system allocates memory dynamically.
    } else {
        printf("Root directory not found.\n");
    }
}
