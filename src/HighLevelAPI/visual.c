#include <string.h>
#include <stdlib.h>


#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"


#include "../directory/directory_helpers.h"
#include "../HighLevelAPI/visual.h"




void print_directory(uint32_t parentDirId, int level, bool *visited) {
    if (level > 30) {
        print_indent(level);
        printf("Maximum recursion level reached, stopping to prevent stack overflow.\n");
        return;
    }

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END && !visited[i]) {
            DirectoryEntry dirEntry;
            flash_read_safe2(i * FILESYSTEM_BLOCK_SIZE, (uint8_t*)&dirEntry, sizeof(DirectoryEntry));

            if (dirEntry.in_use && dirEntry.is_directory && dirEntry.parentDirId == parentDirId) {
                print_indent(level);
                printf("--%s\n", dirEntry.name);
                visited[i] = true;  // Mark this directory as visited
                print_directory(dirEntry.currentDirId, level + 1, visited);

                for (int j = 0; j < MAX_FILES; j++) {
                    if (fileSystem[j].in_use && !fileSystem[j].is_directory && fileSystem[j].parentDirId == dirEntry.currentDirId) {
                        print_indent(level + 1);
                        printf("--%s\n", fileSystem[j].filename);
                    }
                }
            }
        }
    }
}

void print_indent(int level) {
    for (int k = 0; k < level; k++) printf("\t");
}

void print_filesystem_structure() {
    printf("Filesystem Structure:\n");
    bool visited[TOTAL_BLOCKS] = {false};  // Track visited blocks to avoid repetition
    const char* parentPath = "/root";  // Assume the path to the root is known
    DirectoryEntry* rootDirEntry = DIR_find_directory_entry(parentPath); // Function to find directory based on name
    if (rootDirEntry) {
        uint32_t rootDirId = rootDirEntry->currentDirId;
        print_directory(rootDirId, 0, visited);
        free(rootDirEntry);  // Ensure to free any allocated memory
    } else {
        printf("Root directory not found.\n");
    }
}



// void print_directory(uint32_t parentDirId, int level) {
//     if (level > 30) {
//         printf("Maximum recursion level reached, stopping to prevent stack overflow.\n");
//         return;
//     }

//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//         if (FAT[i] == FAT_ENTRY_END) {
//             DirectoryEntry dirEntry;
//             flash_read_safe2(i * FILESYSTEM_BLOCK_SIZE, (uint8_t*)&dirEntry, sizeof(DirectoryEntry));

//             if (dirEntry.in_use && dirEntry.is_directory && dirEntry.parentDirId == parentDirId) {
//                 for (int k = 0; k < level; k++) printf("\t");
//                 printf("--%s\n", dirEntry.name);
//                 if (dirEntry.currentDirId != parentDirId) {
//                     print_directory(dirEntry.currentDirId, level + 1);
//                 }

//                 for (int j = 0; j < MAX_FILES; j++) {
//                     if (fileSystem[j].in_use && !fileSystem[j].is_directory && fileSystem[j].parentDirId == dirEntry.currentDirId) {
//                         for (int k = 0; k <= level; k++) printf("\t");
//                         printf("\t--%s\n", fileSystem[j].filename);
//                     }
//                 }
//             }
//         }
//     }
// }



// void print_filesystem_structure() {
//     const char* parentPath = "/root";  // Default to the root directory
//     DirectoryEntry* ROOTdirEntry = DIR_find_directory_entry(parentPath);
//     printf("Filesystem Structure:\n");
//     uint32_t parentDirId = ROOTdirEntry->currentDirId;
//     print_directory(parentDirId, 0);  // Assuming '0' is the root directory ID
// }


////////////////////////////////////////////////////
// Function to print the directory and its contents
// void print_directory(uint32_t parentDirId, int level) {
//     // printf("Entering print_directory: parentDirId=%u, level=%d\n", parentDirId, level);
    
//     // Prevent excessive recursion which might indicate a loop or an error in directory structure
//     if (level > 30) {
//         printf("Maximum recursion level reached, stopping to prevent stack overflow.\n");
//         return;
//     }

//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//         if (FAT[i] == FAT_ENTRY_END) { // Assuming this checks if the block is a directory entry
//             DirectoryEntry dirEntry;
//             flash_read_safe2(i * FILESYSTEM_BLOCK_SIZE, (uint8_t*)&dirEntry, sizeof(DirectoryEntry));

//             // Check if entry is valid, in use, is a directory, and if it's a valid child or the root itself
//             if (dirEntry.in_use && dirEntry.is_directory && 
//                 (dirEntry.parentDirId == parentDirId) &&
//                 (dirEntry.currentDirId != parentDirId || level == 0)) { // Allow root at level 0
//                 for (int k = 0; k < level; k++) printf("\t");
//                 printf("--%s\n", dirEntry.name);

//                 // Recurse into this directory only if not revisiting the same as parent at deeper levels
//                 if (level == 0 || dirEntry.currentDirId != parentDirId) {
//                     print_directory(dirEntry.currentDirId, level + 1);
//                 }

//                 // Print files within this directory
//                 for (int j = 0; j < MAX_FILES; j++) {
//                     if (fileSystem[j].in_use && !fileSystem[j].is_directory && fileSystem[j].parentDirId == dirEntry.currentDirId) {
//                         for (int k = 0; k <= level; k++) printf("\t");
//                         printf("\t--%s\n", fileSystem[j].filename);
//                     }
//                 }
//             }
//         }
//     }
// }