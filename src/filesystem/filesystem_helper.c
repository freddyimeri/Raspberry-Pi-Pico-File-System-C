 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/mutex.h"
#include <ctype.h>
#include "hardware/adc.h"
#include <stdint.h>
#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
#include "../filesystem/filesystem_helper.h" 
#include "../directory/directory_helpers.h"


static int random_initialized = 0;  // Flag to check if random generator has been initialized

  
/**
 * Prepends a forward slash to a given path if it does not already start with one.
 * This is useful for ensuring that path strings are treated as absolute paths
 * in the filesystem. The function modifies the provided buffer to include
 * the leading slash and ensures the resulting string is null-terminated.
 *
 * @param path The input path that might need a leading slash.
 * @param buffer A buffer where the modified path should be stored.
 * @param buffer_size The size of the buffer to ensure safe string operations.
 *
 * This function performs the following operations:
 * 1. Validates the input parameters to ensure they are not NULL and that the buffer size is adequate.
 * 2. Checks if the path already starts with a slash and, if so, copies the path directly to the buffer.
 * 3. If the path does not start with a slash, it prepends one to the path and then copies the result to the buffer.
 * 4. Ensures that the buffer is null-terminated to prevent buffer overflow issues.
 */
void prepend_slash(const char* path, char* buffer, size_t buffer_size) {
    if (path == NULL || buffer == NULL || buffer_size == 0) {
        return; // Early exit if input parameters are invalid (safety check).
    }

    if (path[0] == '/') {
        // If the path already starts with a slash, directly copy the path to the buffer.
        strncpy(buffer, path, buffer_size);
    } else {
        // If the path does not start with a slash, prepend one using snprintf.
        snprintf(buffer, buffer_size, "/%s", path);
    }
    // Manually set the last character in the buffer to '\0' to ensure the string is null-terminated.
    buffer[buffer_size - 1] = '\0';
}





int find_file_entry_by_name(const char* filename) {

    char path[1024]; // Define a sufficiently large buffer for the path
    prepend_slash(filename, path, sizeof(path));
    
    if (path == NULL) {
        printf("Error: Filename is NULL.\n");
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, path) == 0) {
            printf("File found: %s at index %d\n", path, i);
            return i;
        }
    }

    printf("File not found: %s\n", path);
    return -1; // File not found
}


/**
 * Checks for the existence of a file within a filesystem based on its name and parent directory ID.
 * 
 * This function searches through the global filesystem array to find a file that matches
 * both the provided filename and parent directory identifier. It also ensures the filename
 * starts with a slash for consistency in comparisons.
 *
 * @param filename The name of the file to search for, which may not initially include a leading slash.
 * @param parentID The identifier of the parent directory in which the file is supposed to exist.
 * @return Returns 0 if the file is found, -1 if not found or if there is an error (e.g., NULL filename).
 */
int find_file_existance(const char* filename, uint32_t parentID) {
    // Define a sufficiently large buffer for the path to accommodate the filename with a leading slash.
    char path[1024];

    // Prepend a slash to the filename to ensure consistent formatting in the filesystem.
    prepend_slash(filename, path, sizeof(path));

    // Check if the filename after modification is NULL, which should never be true given the buffer handling.
    if (path == NULL) {
        printf("Error: Filename processing failed or filename is NULL.\n");
        return -1;
    }

    // Loop through all file entries in the global file system array to find a match.
    for (int i = 0; i < MAX_FILES; i++) {
        // Check if the current file entry is in use and matches both the filename and parent directory ID.
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, path) == 0 && fileSystem[i].parentDirId == parentID) {
            // If a matching file is found, print its details and return 0.
            printf("File found: %s at index %d\n", path, i);
            return 0;  // File exists
        }
    }

    // If no matching file is found after checking all entries, print a not found message and return -1.
    printf("File not found: %s\n", path);
    return -1;  // File not found
}





/**
 * Initializes the random number generator by using ADC noise as a seed.
 * This function sets up the ADC (Analog to Digital Converter) to read an unconnected input,
 * utilizing the electrical noise from this input as a seed for the random number generator,
 * ensuring more unpredictability in the generated values.
 */
void init_random() {
    // Check if the random number generator has already been initialized.
    if (!random_initialized) {
        adc_init();  // Initialize the ADC hardware to prepare for reading.
        adc_select_input(0);  // Select ADC input channel 0, assuming it is unconnected and noisy.

        // Read an analog value from the selected ADC channel to use as a seed.
        uint16_t noise = adc_read();
        srand(noise);  // Seed the standard random number generator with the ADC noise value.

        random_initialized = 1;  // Set the flag indicating the random generator is initialized.
    }
}

/**
 * Generates a unique ID number within a specific range by utilizing the standard
 * C library random function, which has been seeded by ADC noise.
 *
 * @return A unique ID number between 1000 and 4294967294 (0xFFFFFFFE), ensuring
 *         the ID is not less than 1000.
 */
uint32_t generateUniqueId() {
    // Ensure the random number generator is properly initialized before generating a number.
    if (!random_initialized) {
        init_random();  // Initialize the random number generator if not already done.
    }

    // Define the range of the unique ID to be generated, from 1000 to uint32_t's maximum value minus one.
    uint32_t range = 4294967294u - 1000 + 1;  // Compute the total number of possible values.

    // Generate a random number within the defined range and adjust by adding 1000 to shift the start.
    return rand() % range + 1000;
}





/**
 * Searches for a file entry in the global fileSystem array by a unique identifier.
 * This function iterates through the fileSystem array, checking each entry to see if it is in use
 * and if its unique identifier matches the one provided.
 *
 * @param unique_file_id The unique identifier of the file to locate.
 * @return The index of the file in the fileSystem array if found, or -1 if no matching file is found.
 */
int find_file_entry_by_unique_file_id(uint32_t unique_file_id) {
    // Iterate over each file entry in the global fileSystem array.
    for (int i = 0; i < MAX_FILES; i++) {
        // Check if the current file entry is in use and if the unique ID matches the one being searched for.
        if (fileSystem[i].in_use && fileSystem[i].unique_file_id == unique_file_id) {
            // If a match is found, print the index at which the file is located for verification.
            printf("File found at index %d\n", i);
            
            // Return the index of the file entry, indicating where it was found.
            return i;
        }
    }

    // If no file with the given unique ID is found after checking all entries, print a message to indicate this.
    printf("File not found:\n");
    return -1; // Return -1 to indicate that the file was not found in the file system.
}




 



/**
 * Creates a new file entry in the filesystem.
 * @param path The path or name of the file.
 * @return Pointer to the created FileEntry or NULL if creation fails.
 */
FileEntry* createFileEntry(const char* path,  uint32_t parentDirId) {
    if (path == NULL) {
        printf("Error: Path is NULL.\n");
        return NULL;
    }// if no parent directory is provided, default to root directory
    if (parentDirId == 0) {
        parentDirId = get_root_directory_id();
    }
    printf("debug createFileEntry for path: %s\n", path);
    sleep_ms(1000);

    sleep_ms(1000);
    printf("debug createFileEntry for path: %s\n", path);
    for (int i = 0; i < MAX_FILES; i++) {
        printf("Checking file entry at index %d\n", i);
        sleep_ms(100);
        if (!fileSystem[i].in_use) {
            printf("Creating new file entry at index %d\n", i);
            // printf("Root directory ID: %u\n", rootDirId);
            strncpy(fileSystem[i].filename, path, sizeof(fileSystem[i].filename) - 1);
            printf("Filename: %s\n", fileSystem[i].filename);
            fileSystem[i].filename[sizeof(fileSystem[i].filename) - 1] = '\0';
            fileSystem[i].in_use = true;
            fileSystem[i].is_directory = false; // Default to file
            fileSystem[i].size = 0;
            
            fileSystem[i].start_block = fat_allocate_block();
            fileSystem[i].parentDirId = parentDirId;
            fileSystem[i].unique_file_id = generateUniqueId();

            printf("New file created: %s\n", fileSystem[i].filename);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("File size: %u\n", fileSystem[i].size);
            printf("Filesystem entry index: %d\n", i);
            // printf("Parent Directory ID: %u\n", fileSystem[i].parentDirId);
            
            fflush(stdout);
            if (fileSystem[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left on device to create new file.\n");
                fflush(stdout);
                memset(&fileSystem[i], 0, sizeof(FileEntry)); // Cleanup
                fileSystem[i].in_use = false; // Explicitly mark it as not in use
                return NULL;
            }
            return &fileSystem[i];
        }
    }
    printf("Error: Filesystem is full, cannot create new file.\n");
    fflush(stdout);
    return NULL;
}
 





void reset_file_content(FileEntry* entry) {
    printf("Attempting to reset file content.\n");
    if (entry == NULL) {
        printf("Error: NULL entry provided to reset_file_content.\n");
        return;
    }

    uint32_t currentBlock = entry->start_block;
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        uint32_t nextBlock;
        int result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error: Failed to free block %u.\n", currentBlock);
            break;
        }
        fat_free_block(currentBlock);
        if (nextBlock == FAT_ENTRY_END) break;
        currentBlock = nextBlock;
    }

    entry->start_block = fat_allocate_block();
    if (entry->start_block == FAT_NO_FREE_BLOCKS) {
        printf("Error: No free blocks available to allocate.\n");
        entry->size = 0;
        return;
    }
    entry->size = 0;
    printf("File content reset successfully. New start block: %u, Size reset to 0.\n", entry->start_block);
}




/**
 * Searches for a file entry in the global filesystem based on the filename and its parent directory ID.
 * This function formats the filename to ensure it has a leading slash, then checks each file entry
 * in the global file system to find a match that is not a directory.
 *
 * @param filename The name of the file to search for. It may not initially include a leading slash.
 * @param parentID The identifier of the parent directory in which the file is supposed to exist.
 * @return Pointer to the FileEntry if found, or NULL if no matching file is found.
 */
FileEntry* FILE_find_file_entry(const char* filename, uint32_t parentID) {
    // Define a sufficiently large buffer to modify the filename with a leading slash.
    char newfilename[512];
    prepend_slash(filename, newfilename, sizeof(newfilename));  // Ensure the filename starts with a slash for consistent comparison.

    // Log entering the function and what file is being searched for to help with debugging.
    printf("Searching for file entry: %s\n", newfilename);
    fflush(stdout);  // Flush stdout to ensure the log message is displayed immediately.

    // Iterate through all file entries in the global file system array.
    for (int i = 0; i < MAX_FILES; i++) {
        // Check if the current file entry is active, not a directory, matches the filename, and belongs to the correct parent directory.
        if (fileSystem[i].in_use && !fileSystem[i].is_directory 
            && strcmp(fileSystem[i].filename, newfilename) == 0
            && fileSystem[i].parentDirId == parentID) {
            // If a matching file is found, print a confirmation message and return a pointer to the file entry.
            printf("File entry found: %s\n", newfilename);
            fflush(stdout);  // Ensure the output is displayed immediately.
            return &fileSystem[i];
        }
    }

    // If no matching file is found after checking all entries, return NULL.
    return NULL;
}

  

/**
 * Extracts the last two components of a given file path: the directory and the filename.
 * This function assumes paths are Unix-like with '/' as the directory separator.
 *
 * @param fullPath The full path from which to extract the parts.
 * @return A PathParts structure containing the extracted directory and filename.
 */
PathParts extract_last_two_parts(const char* fullPath) {
    PathParts parts;
    // Initialize the parts structure to zero to ensure all fields start clean.
    memset(&parts, 0, sizeof(parts));

    // Check for a NULL input which is an error condition for this function.
    if (fullPath == NULL) {
        printf("Input path is NULL.\n");
        return parts; // Early return with empty parts structure if the input path is NULL.
    }

    // Find the last occurrence of '/' which separates the filename from the rest of the path.
    const char* lastSlash = strrchr(fullPath, '/');
    if (!lastSlash) {
        // If no slash is found, it implies that the fullPath is just a filename.
        char path[512]; // Buffer to ensure we have enough space for manipulation.
        prepend_slash(fullPath, path, sizeof(path)); // Prepend a slash to indicate it as a full path.
        strcpy(parts.filename, path); // Copy the modified path as the filename in the structure.
        return parts; // Return the structure with only the filename populated.
    }

    // Check if the last character is '/', indicating the path ends with a directory.
    if (lastSlash[1] == '\0') {
        // When the path ends with '/', consider there's no filename component.
        strcpy(parts.filename, lastSlash + 1); // Copy an empty string to the filename.
    } else {
        // Otherwise, copy the portion after the last '/' as the filename.
        strcpy(parts.filename, lastSlash + 1);
    }

    // Create a copy of the path to manipulate and isolate the directory part.
    char pathCopy[256];
    // Copy up to the last slash to get the directory path.
    strncpy(pathCopy, fullPath, lastSlash - fullPath);
    pathCopy[lastSlash - fullPath] = '\0'; // Null-terminate the directory path.

    // Find the slash before the last slash to isolate the parent directory.
    const char* secondLastSlash = strrchr(pathCopy, '/');
    if (secondLastSlash) {
        // If found, there's a directory name before the filename.
        if (secondLastSlash[1] == '\0') {
            // If the directory part ends with '/', it's an empty directory name.
            strcpy(parts.directory, secondLastSlash + 1);
        } else {
            // Copy the directory part after the second last slash, ensuring it starts with a slash.
            char Dirpath[512]; // Buffer for the directory path.
            prepend_slash(secondLastSlash + 1, Dirpath, sizeof(Dirpath)); // Ensure it starts with '/'
            strcpy(parts.directory, Dirpath); // Copy the prepared directory path to the structure.
        }
    } else {
        // If there's no second last slash, use the entire path as the directory.
        strcpy(parts.directory, pathCopy);
    }

    return parts; // Return the structure populated with the directory and filename.
}


 


 


/**
 * Appends "Copy" to the provided filename, maintaining the file extension if present.
 * This function is useful when creating a duplicate file while preserving the original's extension,
 * avoiding filename conflicts in the file system.
 *
 * @param filename A pointer to the string containing the original filename which may or may not
 *                 include a file extension.
 */
void appendCopyToFilename(char *filename) {
    char temp[256]; // Temporary buffer to hold the modified filename with "Copy" appended

    // Search for the last dot in the filename, which usually starts the file extension
    const char *lastDot = strrchr(filename, '.');
    if (lastDot) {
        // If a dot is found, there is an extension in the filename.
        int basenameLength = lastDot - filename; // Calculate the length of the name part before the dot
        strncpy(temp, filename, basenameLength); // Copy the base part of the filename up to the dot
        temp[basenameLength] = '\0';  // Null-terminate the base part

        // Append "Copy" followed by the original extension to the base filename
        snprintf(temp + basenameLength, sizeof(temp) - basenameLength, "Copy%s", lastDot);
    } else {
        // No dot found, implying no extension; append "Copy" directly to the end of the filename
        snprintf(temp, sizeof(temp), "%sCopy", filename);
    }

    // Copy the modified filename back to the original filename buffer
    strncpy(filename, temp, 256);
    filename[255] = '\0'; // Ensure the string is null-terminated to prevent buffer overflow issues
}



/**
 * Constructs a full path from a directory and a filename by ensuring proper path formatting.
 * This function safely concatenates the directory and filename, adding a necessary slash if
 * the directory does not already end with one, to form a valid filesystem path.
 *
 * @param directory The directory path as a string. It should not be NULL.
 * @param filename The filename to append to the directory path. It should not be NULL.
 * @param full_path A buffer to store the resultant full path. It should not be NULL.
 * @param max_size The maximum size of the buffer to ensure the resultant path does not exceed
 *                 buffer capacity.
 */
void construct_full_path(const char* directory, const char* filename, char* full_path, size_t max_size) {
    // Return immediately if any input pointers are NULL, to avoid dereferencing NULL.
    if (directory == NULL || filename == NULL || full_path == NULL) return;

    // Check the length of the directory to determine if it ends with a slash.
    size_t len = strlen(directory);
    // If the directory is not empty and does not end with a slash, append one.
    if (len > 0 && directory[len - 1] != '/') {
        // Use snprintf to safely concatenate directory, a slash, and filename within buffer limits.
        snprintf(full_path, max_size, "%s/%s", directory, filename);
    } else {
        // If the directory already ends with a slash, concatenate without adding another slash.
        snprintf(full_path, max_size, "%s%s", directory, filename);
    }

    // Manually ensure the last character in the buffer is '\0' to prevent any string overflow issues.
    full_path[max_size - 1] = '\0'; // Ensure null-termination
}




/**
 * Sets a default path in the provided path buffer if it is currently empty.
 * This utility function is typically used to ensure that a path variable is never
 * empty, assigning a default directory path if no path has been set.
 *
 * @param path A mutable buffer containing the current path which may be empty.
 *             If it is empty, the function fills this buffer with the default path.
 * @param default_path The default path to set if the current path buffer is empty.
 *                     This should be a null-terminated string and not NULL.
 */
void set_default_path(char* path, const char* default_path) {
    // Check if the first character of the path is the null character, indicating an empty string.
    if (path[0] == '\0') {
        // If the path is empty, copy the default path into the buffer.
        strcpy(path, default_path);
    }
    // If the path is not empty, no action is taken, and the original path remains unchanged.
}


 




/**
 * Saves the file system entries to a designated area in flash memory.
 * This function is tasked with serializing the `fileSystem` array and writing it
 * to a fixed address in flash memory, ensuring that file system entries are
 * persisted across power cycles or reboots.
 */
void saveFileEntriesToFileSystem() {
    // Address in flash memory where the file system entries are to be stored.
    // This is set to 262144, assuming this address space is reserved for this purpose.
    uint32_t address = 262144;
    printf("Saving file entries to flash memory...\n");

    // Allocate memory for serialization of the file system entries.
    // This assumes the `fileSystem` structure can be serialized directly.
    uint8_t *serializedData = malloc(sizeof(fileSystem));
    if (serializedData == NULL) {
        printf("Failed to allocate memory for file system serialization.\n");
        return; // Early return if memory allocation fails
    }

    // Copy the file system data into the allocated buffer.
    // This simulates serialization if the data structure allows direct binary copying.
    memcpy(serializedData, fileSystem, sizeof(fileSystem));

    // Write the serialized data to flash memory at the specified address.
    // `flash_write_safe` should ensure that the write operation handles any necessary precautions
    // like erasing the flash sector before writing.
    flash_write_safe(address, serializedData, sizeof(fileSystem));

    // Free the allocated memory after the write operation is complete to avoid memory leaks.
    free(serializedData);
    printf("File entries saved to flash memory.\n");
}



/**
 * Loads file entries from a specific address in flash memory into a local array.
 * This function reads the serialized data representing file system entries from flash
 * memory and populates a local array with this data. It can be used during system
 * initialization to restore the state of the file system from persistent storage.
 */
void loadFileEntriesFromFileSystem() {
    // Address in flash memory where the file system entries are stored.
    uint32_t address = 262144;

    // Local array to hold the file entries recovered from flash memory.
    // This ensures that the file system can be restored to its last known state.
    FileEntry recoveredFileSystem[MAX_FILES];

    // Read the data from flash memory into the local array.
    // This assumes the data at the specified address is a valid serialized array of FileEntry structures.
    flash_read_safe(address, (uint8_t*)recoveredFileSystem, sizeof(recoveredFileSystem));

    // Optionally, iterate over the loaded file entries to verify the integrity and correctness of the data.
    for (int i = 0; i < MAX_FILES; i++) {
        // Print each recovered file entry's name to verify that data has been loaded correctly.
        // This is helpful for debugging and ensuring that the load operation was successful.
        printf("Recovered File Entry %d: %s\n", i, recoveredFileSystem[i].filename);
    }

    // Additional logic can be implemented here to further process or integrate the loaded data
    // into the running application, depending on system requirements.
}



