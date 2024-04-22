#include <string.h>
#include <stdlib.h>


#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"


#include "../directory/directory_helpers.h"




// void process_directory_operation(const char* path) {
//     printf("Attempting to process directory operation for: %s\n", path);
//     // Extract the path segments
//     PathSegments segments = extract_path_segments(path);
//     printf("Path has %zu segments:\n", segments.count);
//     printf("Path has %s segments:\n", segments.segments[0]);


//     if (segments.count == 0) {
//         printf("Invalid file path: %s\n", path);
//         // Handle error: Invalid path provided
//         return;
//     }
//     printf(" Path has %zu segments:\n", segments.count);
//     // Loop through each directory in the path (except for the last segment, assuming it's a file)
//     for (size_t i = 0; i < segments.count ; ++i) {
//         printf("Checking directory: %s\n", segments.segments[i]);
//         const char* directory = segments.segments[i];
//         printf("Checking directory: %s\n", directory);
//         DirectoryEntry* dirEntry = DIR_find_directory_entry(directory);
//         if (dirEntry == NULL) {
//             printf("Directory '%s' does not exist. Attempting to create it.\n", directory);
//             if (fs_create_directory(directory)) {
//                 printf("Directory '%s' created successfully.\n", directory);
//                 // Directory created successfully, proceed with next segment
//             } else {
//                 printf("Failed to create directory '%s'.\n", directory);
//                 // Handle error: Directory could not be created
//                 free_path_segments(&segments); // Cleanup before returning
//                 return;
//             }
//         } else {
//             printf("Directory '%s' exists.\n", directory);
//             free(dirEntry); // Assuming dirEntry was dynamically allocated
//         }
//     }

//     // Handle the file segment, which is assumed to be the last segment
//     const char* fileSegment = segments.segments[segments.count - 1];
//     printf("Proceeding with file '%s'.\n", fileSegment);
//     // Proceed with file operation: open/create file, etc.

//     // free the memory allocated for path segments
//     free_path_segments(&segments);
// }

// void process_directory_operation(const char* path) {
//     printf("Attempting to process directory operation for: %s\n", path);
//     // Extract the path segments using the previously discussed function
//     PathSegments segments = extract_path_segments(path);

//     printf("Path has %zu segments:\n", segments.count);
//     for (size_t i = 0; i < segments.count; i++) {
//         printf("Segment %zu: %s\n", i + 1, segments.segments[i]);
//     }

//     char* parentPath = NULL;
//     char* currentPath = strdup(""); // Start with an empty string for the path

//     // Loop through each directory segment, skipping the last one if it's assumed to be a file
//     for (size_t i = 0; i < segments.count; ++i) {
//         printf("Checking directory: %s\n", segments.segments[i]);

//         // Update the current path with the next segment
//         char* updatedPath = malloc(strlen(currentPath) + strlen(segments.segments[i]) + 2); // Allocate space for '/' and '\0'
//         sprintf(updatedPath, "%s/%s", currentPath, segments.segments[i]); // Append the current segment to the path

//         free(currentPath);
//         currentPath = updatedPath;

//         DirectoryEntry* dirEntry = DIR_find_directory_entry(segments.segments[i]);
//         if (dirEntry == NULL) {
//             printf("Directory '%s' does not exist. Attempting to create it.\n", segments.segments[i]);
//             if (!fs_create_directory(segments.segments[i], parentPath)) {
//                 printf("Failed to create directory '%s'.\n", segments.segments[i]);
//                 free(currentPath);
//                 free(parentPath);
//                 free_path_segments(&segments);
//                 return;
//             }
//         } else {
//             printf("Directory '%s' exists.\n", segments.segments[i]);
//             free(dirEntry); // Assuming dirEntry was dynamically allocated
//         }

//         free(parentPath);
//         parentPath = strdup(currentPath);
//     }

//     printf("All directories processed, last directory: '%s'\n", parentPath);

//     // Cleanup
//     free(currentPath);
//     free(parentPath);
//     free_path_segments(&segments);
// }

void process_directory_operation(const char* path) {
    printf("Attempting to process directory operation for: %s\n", path);

    PathSegments segments = extract_path_segments(path);
    if (segments.count == 0) {
        printf("Invalid directory path: %s\n", path);
        return;
    }

    char* parentPath = NULL;  // Start with no parent path
    char* currentPath = strdup(""); // Start with an empty path for appending

    for (size_t i = 0; i < segments.count; ++i) {
        // Construct the current path for checking/creation
        char* updatedPath = malloc(strlen(currentPath) + strlen(segments.segments[i]) + 2);
        sprintf(updatedPath, "%s%s%s", currentPath, (*currentPath) ? "/" : "", segments.segments[i]);

        free(currentPath);
        currentPath = updatedPath;

        printf("Checking directory: %s\n", segments.segments[i]);
        DirectoryEntry* dirEntry = DIR_find_directory_entry(segments.segments[i]);
        if (dirEntry == NULL) {
            printf("Directory '%s' does not exist. Attempting to create it.\n", segments.segments[i]);
            if (fs_create_directory(segments.segments[i], parentPath)) {
                printf("Directory '%s' created successfully under parent '%s'.\n", segments.segments[i], parentPath ? parentPath : "root");
            } else {
                printf("Failed to create directory '%s'.\n", segments.segments[i]);
                free(currentPath);
                free(parentPath);
                free_path_segments(&segments);
                return;
            }
        } else {
            printf("Directory '%s' exists.\n", segments.segments[i]);
            free(dirEntry); // Assuming dirEntry was dynamically allocated
        }

        // Update parentPath to the last valid directory
        free(parentPath);
        parentPath = strdup(segments.segments[i]);
    }

    free(currentPath);
    free(parentPath);
    free_path_segments(&segments);
}



PathSegments extract_path_segments(const char* fullPath) {
    printf("Attempting to extract path segments from: %s\n", fullPath);
    PathSegments pathSegments;
    pathSegments.count = 0;
    pathSegments.segments = NULL;

    if (fullPath == NULL) {
        return pathSegments;
    }

    // Copy the fullPath to modify it
    char* pathCopy = strdup(fullPath);
    if (!pathCopy) {
        return pathSegments; // Return empty on allocation failure
    }

    // Count segments and allocate memory for storing segment pointers
    char* temp = pathCopy;
    while (temp && *temp) {
        if (*temp == '/') {
            pathSegments.count++;  // Increment for each '/'
        }
        temp++;
    }
    
    if (*fullPath != '/') {
        pathSegments.count++;  // Account for the first segment if the path doesn't start with '/'
    }

    printf("Path has %zu segments.\n", pathSegments.count);
    pathSegments.segments = (char**) malloc(pathSegments.count * sizeof(char*));
    if (!pathSegments.segments) {
        free(pathCopy);
        pathSegments.count = 0;
        return pathSegments;
    }

    // Extract segments
    size_t idx = 0;
    char* token = strtok(pathCopy, "/");
    while (token) {
        pathSegments.segments[idx++] = strdup(token);
        printf("Segment %zu: %s\n", idx, pathSegments.segments[idx - 1]);
        token = strtok(NULL, "/");
    }

    printf("Path segments extracted successfully.\n");
    free(pathCopy);  // Clean up the copied path
    return pathSegments;
}

// void free_path_segments(PathSegments* pathSegments) {
//     if (pathSegments) {
//         printf("yesss\n");
//         for (size_t i = 0; i < pathSegments->count; i++) {
//             printf("freeing %s\n", pathSegments->segments[i]);
//             free(pathSegments->segments[i]);
//         }
//         // free(pathSegments->segments);

//     }
// }

void free_path_segments(PathSegments* pathSegments) {
    if (pathSegments) {
        for (size_t i = 0; i < pathSegments->count; i++) {
            free(pathSegments->segments[i]);
        }
        free(pathSegments->segments);
        pathSegments->segments = NULL;
        pathSegments->count = 0;
    }
}


// void process_directory_operation(const char* path) {
 

//     if ( ) { // check each segment from extract_path_segments
//         DirectoryEntry* dirEntry = DIR_find_directory_entry(directory);
//         if (dirEntry == NULL) {
//             printf("Directory '%s' does not exist. Attempting to create it.\n", directory);
//             if (fs_create_directory(directory)) {
//                 printf("Directory '%s' created successfully.\n", directory);
//                 // Directory created successfully, proceed with file operation
//             } else {
//                 printf("Failed to create directory '%s'.\n", directory);
//                 // Handle error: Directory could not be created
//                 return;
//             }
//         } else {
//             printf("Directory '%s' exists. Proceeding with file '%s'.\n", directory, fileName);
//             // Proceed with file operation: open/create file, etc.
//             free(dirEntry); // Assuming dirEntry was dynamically allocated
//         }
//     } else {
//         printf("Invalid file path: %s\n", path);
//         // Handle error: Invalid path provided
//     }
// }


bool has_file_extension(const char* segment) {
    return strrchr(segment, '.') != NULL;
}









// typedef struct {
//     uint32_t lastDirId;
// } FS_Metadata;

// FS_Metadata fs_metadata;

// // Function to initialize filesystem metadata
// void fs_init() {
//     // Read the last directory ID from flash
//     flash_read_safe(FLASH_STORAGE_ADDRESS, (uint8_t*)&fs_metadata, sizeof(FS_Metadata));
//     printf("Filesystem initialized with last directory ID: %u\n", fs_metadata.lastDirId);
// }

// // Function to get a new unique directory ID
// uint32_t get_new_directory_id() {
//     fs_metadata.lastDirId++;  // Increment the last used ID to generate a new one
//     // Optionally write to flash every time an ID is generated (or defer this to minimize writes)
//     flash_write_safe(FLASH_STORAGE_ADDRESS, (const uint8_t*)&fs_metadata, sizeof(FS_Metadata));
//     return fs_metadata.lastDirId;
// }

// // Function to save filesystem state before shutdown
// void fs_shutdown() {
//     // Ensure the latest directory ID is saved to flash
//     flash_write_safe(FLASH_STORAGE_ADDRESS, (const uint8_t*)&fs_metadata, sizeof(FS_Metadata));
//     printf("Filesystem state saved with last directory ID: %u\n", fs_metadata.lastDirId);
// }

// int main() {
//     fs_init();  // Initialize and load metadata from flash
//     uint32_t newDirId = get_new_directory_id();  // Get a new directory ID
//     printf("Generated new directory ID: %u\n", newDirId);
//     fs_shutdown();  // Save state before exiting
//     return 0;
// }




 

// typedef struct {
//     uint32_t entryId;
//     bool in_use;    
// } FS_entryId;

// static FS_entryId entryId[MAX_DIRECTORY_ENTRIES];
 


 

// int allocateEntryId() {
//     // loop that generates the entryId for the directory/file entries
//     for (int i = 4500; i < 4500 + MAX_DIRECTORY_ENTRIES; i++) {
//         // loop for location of the entryId
//         for (int p = 0; p < MAX_DIRECTORY_ENTRIES; p++) {
//             printf("Processing index: %d\n", p);
//             if (entryId[p].in_use == true) {
//                 printf("Entry ID %u is in use.\n", entryId[p].entryId);
//             } else {
//                 entryId[p].entryId = i;
//                 entryId[p].in_use = true;
//                 printf("Entry ID %u is now in use.\n", entryId[p].entryId);
//                 return i;
//             }
//         }
//     }
//     return -1; // If no ID is available
// }

 

// // save the static FS_entryId entryId[MAX_DIRECTORY_ENTRIES]; in the flash 

// void save_entryId(void) {
//     // find space in the flash to save the entryId using FAT system
//     //print the size of it the whole array
//     flash_write_safe(FLASH_STORAGE_ADDRESS, (const uint8_t*)&entryId, sizeof(entryId));
// }

// // recover the entryId from the flash
// void recover_entryId(void) {
//     // if entryId is empty, then allocate then recover the entries from the flash
//     if (entryId[0].entryId == 0) {
//         // look though the flash entries to find the entryId
//         // Look through the flash entries to find the entryId
//         for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {


//             // Read the entry from flash
//             flash_read_safe(FLASH_STORAGE_ADDRESS + i * sizeof(FS_entryId), (uint8_t*)&entryId, sizeof(FS_entryId));
//             // Check if the entry is in use
//             if (entryId[0].in_use) {
//                 // Entry is in use, continue searching
//                 continue;
//             } else {
//                 // Entry is available, break the loop
//                 break;
//             }
//         }
//         // Allocate a new entry ID
//         uint32_t newEntryId = allocateEntryId();
//         if (newEntryId == -1) {
//             printf("No available entry ID.\n");
//             // Handle error: No available entry ID
//             return;
//         }
//         printf("Allocated new entry ID: %u\n", newEntryId);

//         // Save the updated entry ID array to flash
//         save_entryId();

//         // Recover the entry ID array from flash
//         recover_entryId();

//         // Recover the directory entries using the recovered entry ID array
//         DirectoryEntry* recoveredEntries = recover_entries_Id();
//         allocateEntryId();
//         flash_read_safe(FLASH_STORAGE_ADDRESS, (uint8_t*)&entryId, sizeof(entryId));
//     }
//     // read the entryId from the flash
//     flash_read_safe(FLASH_STORAGE_ADDRESS, (uint8_t*)&entryId, sizeof(entryId));
//     //print the size of it the whole array
//     printf("Size of entryId: %lu\n", sizeof(entryId));
// }





// FS_entryId* recover_entries_Id(void) {
//     printf("\n\nENTERED recover_entries_Id\n");
//     fflush(stdout);

//     for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
//          if (FAT[i] == FAT_ENTRY_END) {

//             uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            
//             FS_entryId tempEntryId;  // Temporary storage

//             printf("Checking block %u\n", i);
//             fflush(stdout);

//             uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
//             DirectoryEntry dirEntry;  // Temporary storage
//             flash_read_safe2(address, (uint8_t *)&dirEntry, sizeof(DirectoryEntry));
 
//             fflush(stdout);
            
            
//         }
//     }
 
// }



// int allocateEntryId() {
//     // loop that generates the entryId for the directory/file entries
//     for (int i = 4500; i < 4500 + MAX_DIRECTORY_ENTRIES; i++) {
//         // loop for location of the entryId
//         for (int p = 0; p < MAX_DIRECTORY_ENTRIES; p++) {
//             printf("Processing index: %d\n", p);
//             if (entryId[p].in_use == true) {
//                 printf("Entry ID %u is in use.\n", entryId[p].entryId);
//             } else {
//                 entryId[p].entryId = i;
//                 entryId[p].in_use = true;
//                 printf("Entry ID %u is now in use.\n", entryId[p].entryId);
//                 return i;
//             }
//         }
//     }
//     return -1; // If no ID is available
// }

 
