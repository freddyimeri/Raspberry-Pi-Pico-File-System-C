 
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

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
 

/////fs_seek/////////////
#define SEEK_SET 0  // Start of the file
#define SEEK_CUR 1  // Current position in the file
#define SEEK_END 2  // End of the file
////////////////////////
// 1589 LINES 
 
bool fs_initialized = false;
static mutex_t filesystem_mutex;
FileEntry fileSystem[MAX_FILES];


bool isValidChar(char c);
bool isValidChar(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '/';
}

 



/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
 
    fat_init();
    mutex_init(&filesystem_mutex);

    fs_initialized = true;
   
    uint32_t current_start_block = FLASH_TARGET_OFFSET;

    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;  // Initially, no file is in use
        fileSystem[i].size = 0;        // Initialize the file size to 0
        // printf("fs_init: fileSystem[%d].size = %d\n", i, fileSystem[i].size);
        fflush(stdout);

        // Initialize the flash address for each file entry
        fileSystem[i].start_block = current_start_block;

        // Move the current flash address to the start of the next sector
        // making sure each file starts at the beginning of a new sector
        current_start_block += MAX_FILE_SIZE;
         
        printf("Current start block: %u\n", current_start_block);
        fflush(stdout);

        // Check if the new flash address exceeds the flash size, wrap around, or handle as error
        if (current_start_block >= FLASH_TARGET_OFFSET + FLASH_USABLE_SPACE) {
            // Handle the case where we've exceeded the flash space
            // This could be an error, or you might want to wrap around
            printf("Error: Not enough flash memory for the number of files.\n");
            fflush(stdout);
            return;
        }
    }

    int resetSuccess = reset_root_directory();
    if (!resetSuccess) {
        printf("Critical error initializing root directory.\n");
        // Handle the error. Consider reverting `fs_initialized` if needed or taking other corrective action.
        fs_initialized = false; // Consider setting this based on your error handling strategy.
        return;
    }
    fs_initialized = true;
    printf("Filesystem initialized.\n");
    
}

/**
 * Opens a file with the specified mode.
 * 
 * @param FullPath The full path to the file.
 * @param mode The mode to open the file in ('r' for read, 'w' for write, 'a' for append).
 * @return A pointer to an FS_FILE structure if successful, NULL otherwise.
 */
FS_FILE* fs_open(const char* FullPath, const char* mode) {
    // Validate input parameters.
    if (FullPath == NULL || mode == NULL) {
        printf("Error: Path or mode is NULL.\n");
        fflush(stdout);
        return NULL;
    }

    // Ensure the path starts with a slash for consistency.
    const char* path = prepend_slash(FullPath);
    if (path == NULL) {
        printf("Error: Failed to process path '%s'.\n", FullPath);
        fflush(stdout);
        return NULL;
    }

    // Attempt to find the file entry in the filesystem.
    FileEntry* entry = FILE_find_file_entry(path);

    // Allocate memory for the FS_FILE structure.
    FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
    if (!file) {
        printf("Error: Memory allocation failed for FS_FILE.\n");
        fflush(stdout);
        return NULL;
    }

    // Handle different modes of file operation.
    if (strcmp(mode, "w") == 0) {
        // Write mode: reset or create file.
        printf("Opening file '%s' in write mode.\n", FullPath);
        if (entry != NULL) {
            reset_file_content(entry);  // If the file exists, reset its content.
        } else {
            // If the file does not exist, create a new file entry.
            entry = createFileEntry(path);
            if (entry == NULL) {
                free(file); // Free allocated FS_FILE structure on error.
                printf("Error: Failed to create file entry for '%s'.\n", FullPath);
                fflush(stdout);
                return NULL;
            }
        }
        // Calculate the flash storage offset based on the file's start block.
        uint32_t offsetInBytes = entry->start_block * FILESYSTEM_BLOCK_SIZE;
        // Write the initial file entry to flash.
        flash_write_safe2(offsetInBytes, (const uint8_t*)entry, sizeof(FileEntry));
        file->entry = entry;
        file->position = 0;  // Start at the beginning of the file for writing.
        file->mode = 'w';

    } else if (strcmp(mode, "a") == 0) {
        // Append mode: file must exist.
        if (entry == NULL) {
            printf("Error: File '%s' not found.\n", FullPath);
            fflush(stdout);
            free(file); // Ensure allocated memory is freed on error.
            return NULL;
        }
        printf("Opening file '%s' in append mode.\n", FullPath);
        file->entry = entry;
        file->position = entry->size;  // Set the position to the end of the file to append.
        file->mode = 'a';

    } else if (strcmp(mode, "r") == 0) {
        // Read mode: file must exist.
        if (entry == NULL) {
            printf("Error: File '%s' not found.\n", FullPath);
            fflush(stdout);
            free(file); // Ensure allocated memory is freed on error.
            return NULL;
        }
        printf("Opening file '%s' in read mode.\n", FullPath);
        file->entry = entry;
        file->position = 0;  // Start reading from the beginning of the file.
        file->mode = 'r';

    } else {
        // Handle invalid mode.
        free(file);
        printf("Error: Invalid mode '%s'.\n", mode);
        fflush(stdout);
        return NULL;
    }

    // Return the prepared FS_FILE structure.
    return file;
}



/**
 * Writes data to a file.
 *
 * @param file The FS_FILE pointer representing the file to write to.
 * @param buffer The buffer containing data to write to the file.
 * @param size The number of bytes to write.
 * @return The number of bytes actually written, or -1 if an error occurs.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    // Validate the inputs for null pointers and ensure size is non-negative.
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid arguments for fs_write.\n");
        return -1; // Error code for invalid arguments.
    }

    // Check if the file is opened in a mode that allows writing.
    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1; // Error code for incorrect mode.
    }

    int writeSize = size;
    // Adjust the write size based on the file's mode and capacity.
    if (file->mode == 'a') {
        // Calculate maximum size that can be appended without overflowing the buffer.
        writeSize = min(size, sizeof(file->entry->buffer) - file->entry->size);
        if (writeSize < size) {
            printf("Warning: Truncating write size because of buffer limit.\n");
        }
        // Append data to the end of the existing content in the buffer.
        memcpy(file->entry->buffer + file->entry->size, buffer, writeSize);
        file->entry->size += writeSize; // Update the file size.
    } else if (file->mode == 'w') {
        // Overwrite data from the start of the buffer.
        memcpy(file->entry->buffer, buffer, min(size, sizeof(file->entry->buffer)));
        file->entry->size = writeSize; // Reset the file size to the write size.
    }

    // Update the file's position.
    file->position += writeSize;

    // Calculate the flash memory offset where the file's data starts.
    uint32_t writeOffset = file->entry->start_block * FILESYSTEM_BLOCK_SIZE;
    // Write the updated file entry back to the flash memory.
    flash_write_safe2(writeOffset, (const uint8_t*)file->entry, sizeof(FileEntry));

    // Display the number of bytes written to the file.
    printf("fs_write: %d bytes written.\n", writeSize);
    return writeSize; // Return the number of bytes successfully written.
}



/**
 * Closes the specified file.
 * 
 * This function handles the cleanup and release of resources associated with an open file.
 * It is responsible for freeing the memory allocated to the FS_FILE structure.
 *
 * @param file A pointer to the FS_FILE structure representing the file to be closed.
 */
void fs_close(FS_FILE* file) {
    // Check if the file pointer is valid before attempting to close.
    if (file == NULL) {
        // Print an error message and exit the function if the file pointer is NULL.
        printf("Error: Attempted to close a NULL file pointer.\n");
        fflush(stdout);
        return;
    }

    // Free the memory allocated for the FS_FILE structure.
    // This is important to prevent memory leaks.
    free(file);
}


 /**
 * Reads data from the specified file into the provided buffer.
 *
 * This function attempts to read up to 'size' bytes from the file associated with 'file' into 'buffer'.
 * It handles various edge cases such as null pointers, invalid read sizes, and incorrect file modes.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to read from.
 * @param buffer A pointer to the buffer where the data will be stored.
 * @param size   The maximum number of bytes to read.
 * @return The number of bytes actually read, or a negative error code if an error occurred.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    // Validate the file and buffer pointers to ensure they are not NULL.
    if (file == NULL || buffer == NULL) {
        printf("Error: Null file or buffer pointer provided.\n");
        return -1;  // Use a defined error code in practice.
    }

    // Validate the request size to ensure it's positive.
    if (size <= 0) {
        printf("Error: Invalid size to read (%d).\n", size);
        return -1;  // Use a defined error code in practice.
    }

    // Ensure the file is opened in a mode that allows reading.
    if (file->mode != 'r' && file->mode != 'a') {
        printf("Error: File is not open in a readable or append mode.\n");
        return -1;  // Use a defined error code in practice.
    }

    // Calculate the number of bytes that can actually be read, which may be less than requested.
    int bytes_to_read = min(size, file->entry->size - file->position);
    if (bytes_to_read <= 0) {
        // If there is no data left to read, return zero.
        printf("No more data to read from file.\n");
        return 0;
    }

    // Perform the actual data copying from the file's internal buffer to the provided buffer.
    memcpy(buffer, file->entry->buffer + file->position, bytes_to_read);

    // Update the file's current position.
    file->position += bytes_to_read;

    // Provide feedback on how many bytes were read.
    printf("Read %d bytes from file '%s'.\n", bytes_to_read, file->entry->filename);

    // Return the number of bytes read.
    return bytes_to_read;
}

/**
 * Sets the file position indicator for the specified file.
 *
 * @param file A pointer to the FS_FILE structure representing the file.
 * @param offset The number of bytes to offset from the specified position.
 * @param whence The position from which to calculate the offset. 
 *               Can be one of the following:
 *               SEEK_SET - Sets the position relative to the beginning of the file.
 *               SEEK_CUR - Sets the position relative to the current position.
 *               SEEK_END - Sets the position relative to the end of the file.
 * @return 0 if successful, or -1 if an error occurred (such as attempting to seek
 *         to an invalid position or passing an invalid file pointer or whence value).
 */
int fs_seek(FS_FILE* file, long offset, int whence) {
    if (file == NULL) {
        printf("Error: Null file pointer provided.\n");
        return -1;  // Error due to invalid file pointer
    }

    long new_position;  // This will hold the computed new position based on the 'whence' and 'offset'

    switch (whence) {
        case SEEK_SET:
            // Position is set to 'offset' bytes from the start of the file.
            new_position = offset;
            break;
        case SEEK_CUR:
            // Position changes by 'offset' bytes from the current position.
            new_position = file->position + offset;
            break;
        case SEEK_END:
            // Position is set to 'offset' bytes from the end of the file.
            // If offset is negative, it positions backward from the end of the file.
            new_position = file->entry->size + offset;
            break;
        default:
            printf("Error: Invalid 'whence' argument (%d).\n", whence);
            return -1; // Error due to invalid 'whence' value
    }

    // Validate the new position to ensure it is within the valid range of the file.
    if (new_position < 0 || new_position > file->entry->size) {
        printf("Error: Attempted to seek to an invalid position (%ld).\n", new_position);
        return -1; // The new position is out of bounds
    }

    // Successfully set the new position
    file->position = new_position;
    return 0; // Success indicates the new position was set without issues
}
 


 


/**
 * Copies a file from the source path to the destination path, ensuring not to overwrite existing files
 * in the destination by appending "Copy" to the file name if necessary.
 *
 * @param source_path The path to the source file.
 * @param dest_path The path to the destination where the file should be copied.
 * @return Returns 0 on success, -1 on error.
 */
int fs_cp(const char* source_path, const char* dest_path) {
    // Extract the directory and filename parts from the source and destination paths.
    PathParts old_path = extract_last_two_parts(source_path);
    PathParts new_path = extract_last_two_parts(dest_path);


    char* source_filename= old_path.filename;
    char* dest_directory_path= new_path.directory;
    char* dest_filename= new_path.filename;


    
    // Validate the modified source filename.
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", source_path);
        return -1;
    }
    // Find the file entry by name in the filesystem to get the unique file ID.
    int fileIndexSource = find_file_entry_by_name(source_filename);
    if (fileIndexSource == -1) {
        printf("Error: Source file '%s' does not exist.\n", source_filename);
        return -1;
    }


    // Determine the destination directory, default to "/root" if unspecified.
    if (dest_directory_path[0] == '\0') {
        strcpy(dest_directory_path, "/root");  // Set default path if empty
    } 

    // Lookup the directory entry for the destination path. in order to retrieve the parent directory ID.
    // This way we can ensure the file is copied to a valid directory with a valid parent directory ID.
    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path); // Store parent directory ID for later use.
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }
    uint32_t parentID = destDirEntry->currentDirId;
    free(destDirEntry);  

    // Check if the filename already exists in the destination directory to avoid overwriting.
    // Check for duplicate file name in the destination directory
     for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, source_filename) == 0 && fileSystem[i].parentDirId == parentID) {
            printf("File name already exists in the destination directory.\n");
            printf("creating new file name\n");
            char newFileName[256]; // Allocate space for new filename plus "copy"
   

        const char *lastDot = strrchr(source_filename, '.');
        if (lastDot) {
            // There's an extension. Copy up to the dot.
            int basenameLength = lastDot - source_filename;
            strncpy(newFileName, source_filename, basenameLength);
            newFileName[basenameLength] = '\0';  // Null-terminate after the basename

            // Append "Copy" and then the extension.
            strcat(newFileName, "Copy");
            strcat(newFileName, lastDot);  // Append the extension
        } else {
            // No extension found, just append "Copy".
            snprintf(newFileName, 256, "%sCopy", source_filename);
        }
        strcpy(dest_filename, newFileName);
        break;

        }
    }

    // Create a new file entry for the destination file.  
    FS_FILE* fileCopy = fs_open(dest_filename, "w"); // this will be the coppy file
        if (fileCopy == NULL) {
            printf("Error: Failed to open file '%s' for copying.\n", source_filename);
            return -1;
        }

    // now open the previous file to read the contents
    FS_FILE* oldfile = fs_open(source_filename, "r");

    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }
    uint32_t uniqueIdFile = fileCopy->entry->unique_file_id;
    // look though the global fileSystem array to find the index of the file that we want to copy, from oldfile into fileCopy
     int fileIndex = find_file_entry_by_unique_file_id(uniqueIdFile);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", source_filename);
        return -1;
    }

    // now we have the index of the file that we want to copy, we can now copy the contents of the old file into the new file
    // we will copy the contents of the old file into the new file
    fileSystem[fileIndex].size = oldfile->entry->size;
    fileSystem[fileIndex].parentDirId = parentID; /// this one we will change 
    memcpy( fileSystem[fileIndex].buffer, oldfile->entry->buffer, sizeof(oldfile->entry->buffer));

    // calculate the offset of the file in the flash memory
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);
    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry)); 

    // Clean up open file handles.
    fs_close(oldfile);
    fs_close(fileCopy);

   return 0;

}


 int fs_mv(const char* old_path, const char* new_path){


    PathParts old_paths = extract_last_two_parts(old_path);
    PathParts new_paths = extract_last_two_parts(new_path);


    char* source_filename = old_paths.filename;
    char* dest_directory_path= new_paths.directory;
    char* dest_filename= new_paths.filename;


    
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", old_path);
        return -1;
    }


    if (dest_directory_path[0] == '\0') {
        strcpy(dest_directory_path, "/root");  // Set default path if empty
    } 



    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path);
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }
    uint32_t parentID = destDirEntry->currentDirId;
    free(destDirEntry);  // Don't forget to free the dynamically allocated memory

    // now open to get the unique id of the file that we want to copy
    FS_FILE* oldfile = fs_open(source_filename, "r");

    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }
    uint32_t uniqueIdFile = oldfile->entry->unique_file_id;

     int fileIndex = find_file_entry_by_unique_file_id(uniqueIdFile);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", source_filename);
        return -1;
    }
    fileSystem[fileIndex].parentDirId = parentID; /// this one we will change 
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));
    printf("Data written to file.\n");
    printf("File '%s' successfully copied to '%s'.\n", source_filename, dest_directory_path);
    
   return 0;
}




int fs_rm(const char* path) {
    // Validate the input path
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure the file exists
    FileEntry* fileEntry = FILE_find_file_entry(path);
    if (!fileEntry) {
        printf("Error: File '%s' not found.\n", path);
        fflush(stdout);
        return -2; // File not found
    }

    // Prevent deletion if the target is a directory (if applicable)
    if (fileEntry->is_directory) {
        printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
        fflush(stdout);
        return -3; // Attempt to remove a directory with a file removal function
    }

    // Free all blocks associated with this file
    uint32_t currentBlock = fileEntry->start_block;
    uint32_t nextBlock;
    int result;

    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
            fflush(stdout);
            // Decide how to handle error: break out of the loop
            break; 
        }

        fat_free_block(currentBlock);

        // Check if the end of the chain is reached or an invalid block index is encountered
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Properly end loop
        }

        currentBlock = nextBlock;
    }
    // Mark the file entry as not in use
    memset(fileEntry, 0, sizeof(FileEntry));
    fileEntry->in_use = false;

    printf("File '%s' successfully removed.\n", path);

    // find the file in the fileSystem array and update the in-use flag to false
    int fileIndex = find_file_entry_by_name(path);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", path);
        return -1;
    }
    fileSystem[fileIndex].in_use = false;


    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));

    
    fflush(stdout);
    return 0; // Success
}




int fs_wipe(const char* path){
    // Validate the input path
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure the file exists
    FileEntry* fileEntry = FILE_find_file_entry(path);
    if (!fileEntry) {
        printf("Error: File '%s' not found.\n", path);
        fflush(stdout);
        return -2; // File not found
    }

    // Prevent deletion if the target is a directory (if applicable)
    if (fileEntry->is_directory) {
        printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
        fflush(stdout);
        return -3; // Attempt to remove a directory with a file removal function
    }
    // Free all blocks associated with this file
    uint32_t currentBlock = fileEntry->start_block;
    uint32_t nextBlock;
    int result;

    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
            fflush(stdout);
            // Decide how to handle error: break out of the loop
            break; 
        }

        fat_free_block(currentBlock);

        // Check if the end of the chain is reached or an invalid block index is encountered
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Properly end loop
        }

        currentBlock = nextBlock;
    }
   

    // find the file in the fileSystem array and update the in-use flag to false
    int fileIndex = find_file_entry_by_name(path);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", path);
        return -1;
    }
     uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    flash_erase_safe2(writeOffset);

    memset(&fileSystem[fileIndex], 0, sizeof(FileEntry));  // Set the entire entry to zero
    
    fflush(stdout);
    return 0; // Success
}


