#include <string.h>
#include <stdlib.h>


#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"


#include "../directory/directory_helpers.h"

bool has_file_extension(const char* segment);



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




