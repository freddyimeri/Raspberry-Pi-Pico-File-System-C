 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/mutex.h"
#include <ctype.h>
 
#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
 #include "../filesystem/filesystem_helper.h"  

 
#include "../directory/directory_helpers.h"
#include "../filesystem/filesystem_helper.h"  

 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem_helper.h"

/**
 * Extracts the last two components of a given file path: the directory and the filename.
 * This function assumes paths are Unix-like with '/' as the directory separator.
 *
 * @param fullPath The full path from which to extract the parts.
 * @return A PathParts structure containing the extracted directory and filename.
 */

/**
 * Extracts the last two components of a given file path: the directory and the filename.
 * This function assumes paths are Unix-like with '/' as the directory separator.
 *
 * @param fullPath The full path from which to extract the parts.
 * @return A PathParts structure containing the extracted directory and filename.
 */
PathParts extract_last_two_parts(const char* fullPath) {
    PathParts parts;
    memset(&parts, 0, sizeof(parts)); // Initialize the parts structure to zero.

    // Print debug information about the operation being performed.
    printf("Attempting to extract last two parts from: %s\n", fullPath);

    // Check if the input path is NULL, which is an error condition.
    if (fullPath == NULL) {
        printf("Input path is NULL.\n");
        printf("case 1\n");
        return parts; // Return empty parts if the input is NULL.
    }

    // Use strrchr to find the last occurrence of '/'
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash) { // If there's no '/', the path is just a filename
        printf("case 2\n");
        printf("Only a filename provided without directory path.\n");
        char path[512]; // Define a sufficiently large buffer for the path
        prepend_slash(fullPath, path, sizeof(path));

        strcpy(parts.filename, path); // Assume the entire path is the filename.
        printf("Extracted file name: %s\n", parts.filename);
        return parts; // Return the parts with only the filename filled.
    }


     if (lastSlash[1] == '\0') {
            printf("case 2.1\n");
            printf("secondLastSlash + 1 is NULL\n");
            strcpy(parts.filename, lastSlash + 1);
        }else{
            printf("case 2.2\n");
            printf("secondLastSlash + 1 is not NULL\n");

            strcpy(parts.filename, lastSlash );
    }

   // strcpy(parts.filename, lastSlash);
    printf("Extracted file name: %s\n", parts.filename);

    // Copy the path up to the last slash into a temporary buffer to extract the directory.
    char pathCopy[256];
    strncpy(pathCopy, fullPath, lastSlash - fullPath); // Copy the part before the last slash.
    pathCopy[lastSlash - fullPath] = '\0'; // Null-terminate the copied part.
    printf("pathCopy: %s\n", pathCopy);

    // Find the last '/' in the temporary copy to extract the directory name.
    const char* secondLastSlash = strrchr(pathCopy, '/');
    if (secondLastSlash) {
        printf("case 3\n");
        // Copy the directory name after this second last slash.
        printf("secondLastSlash : %s\n", secondLastSlash );
        printf("secondLastSlash +1 : %s\n", secondLastSlash + 1);
        if (secondLastSlash[1] == '\0') {
            printf("case 3.1\n");
            printf("secondLastSlash + 1 is NULL\n");
            strcpy(parts.directory, secondLastSlash + 1);
        }else{
            printf("case 3.2\n");
            printf("secondLastSlash + 1 is not NULL\n");
            char Dirpath[512]; // Define a sufficiently large buffer for the path
            prepend_slash(secondLastSlash + 1, Dirpath, sizeof(Dirpath));
            strcpy(parts.directory, Dirpath);
        }
        
    } else {
        // If there's no second last slash, the entire modified path is the directory.
        printf("case 4\n");
        printf("pathCopy: %s\n", pathCopy); 
        strcpy(parts.directory, pathCopy);
    }

    // Print the extracted directory.
    printf("Extracted directory: %s\n", parts.directory);
    return parts; // Return the populated parts structure.
}
 

void prepend_slash(const char* path, char* buffer, size_t buffer_size) {
    if (path == NULL || buffer == NULL || buffer_size == 0) {
        return; // Safety check for null pointers and non-zero size
    }

    if (path[0] == '/') {
        // The path already starts with a slash, just copy it.
        strncpy(buffer, path, buffer_size);
    } else {
        // Prepend a slash to the path.
        snprintf(buffer, buffer_size, "/%s", path);
    }
    buffer[buffer_size - 1] = '\0'; // Ensure null termination
}
