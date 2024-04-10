#include "filesystem.h"
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> // For free()
#include "flash_ops.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "fat_fs.h"
#include "flash_config.h"
#include "pico/mutex.h"
#include <ctype.h>
#include "Directories.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
 
bool fs_initialized = false;
static mutex_t filesystem_mutex;
bool isValidChar(char c);
bool isValidPath(const char* path);

FileEntry fileSystem[MAX_FILES];
 
 
 

void reset_file_content(FileEntry* entry);
FileMode determine_mode(const char* mode);



bool isValidChar(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '/';
}

// Validates the path format and characters
bool isValidPath(const char* path) {
    // Path must start with a "/" character
    if (path[0] != '/') {
        printf("Error: Path must start with '/'.\n");
        return false;
    }

    // Check each character in the path
    for (int i = 1; path[i] != '\0'; i++) {
        if (!isValidChar(path[i])) {
            printf("Error: Invalid character '%c' in path.\n", path[i]);
            return false;
        }

        // Additional checks can be added here, such as:
        // - Disallowing consecutive slashes
        // - Limiting the total length of the path
        // - Limiting the depth of directories

        // Example: disallow consecutive slashes
        if (path[i] == '/' && path[i-1] == '/') {
            printf("Error: Consecutive slashes are not allowed.\n");
            return false;
        }
    }

    // Path is considered valid
    return true;
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
        printf("fs_init: fileSystem[%d].size = %d\n", i, fileSystem[i].size);
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





 
FileEntry* find_file_by_path(const char* path) {
    if (path == NULL) {
        return NULL; // Early exit if path is NULL
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!fileSystem[i].in_use) {
            continue; // Skip entries not in use, potentially saving string comparison time
        }
        if (strcmp(fileSystem[i].filename, path) == 0) {
            return &fileSystem[i]; // File found
        }
    }
    return NULL; // File not found
}

 





/**
 * Opens a file with the specified path and mode.
 *
 * @param path The path of the file to be opened.
 * @param mode The mode in which the file should be opened.
 * @return A pointer to the opened file, or NULL if an error occurred.
 */
FS_FILE* fs_open(const char* path, const char* mode) {
    if (path == NULL || mode == NULL) {
        printf("Error: Path or mode is NULL.\n");
        fflush(stdout);
        return NULL;
    }



     // Split the path into directory parts and filename
    char dirPath[256]; // Assuming maximum path length
    char fileName[FILENAME_MAX]; // Assuming FILENAME_MAX defines the max filename length
    split_path_into_parent_and_dir(path, dirPath, fileName); // You need to implement split_path
   
    
    FileEntry* entry = find_file_by_path(path);
    bool isNewFileCreation = (entry == NULL) && (strchr("wa", mode[0]) != NULL);

    // Attempt to create a new file entry if the file does not exist and the mode allows for writing or appending
    if (isNewFileCreation) {
        entry = create_new_file_entry(path);
        if (entry == NULL) {
            return NULL; // Error message is handled in create_new_file_entry
        }
    } else if (entry == NULL) {
        printf("Error: File not found for reading.\n");
        fflush(stdout);
        return NULL;
    }

    // Special handling for 'w' mode: reset the file if it exists
    if (strcmp(mode, "w") == 0) {
        reset_file_content(entry);
    }

    // Create and initialize the FS_FILE structure
    FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
    if (file == NULL) {
        printf("Error: Memory allocation failed for FS_FILE.\n");
        fflush(stdout);
        return NULL;
    }

    file->entry = entry;
    file->position = (strcmp(mode, "a") == 0) ? entry->size : 0; // Append mode sets position to the end
    file->mode = determine_mode(mode); // Simplified mode determination

    return file;
}

FileEntry* create_new_file_entry(const char* path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fileSystem[i].in_use) {
            strncpy(fileSystem[i].filename, path, sizeof(fileSystem[i].filename) - 1);
            fileSystem[i].filename[sizeof(fileSystem[i].filename) - 1] = '\0';
            fileSystem[i].in_use = true;
            fileSystem[i].is_directory = false; // Default to file
            fileSystem[i].size = 0;
            fileSystem[i].start_block = fat_allocate_block();

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

// void reset_file_content(FileEntry* entry) {
//     uint32_t block = entry->start_block;
//     uint32_t nextBlock;
//     int result;

//     while (block != FAT_ENTRY_END) {
//         uint32_t nextBlock = fat_get_next_block(block);

//         if (nextBlock == FAT_ENTRY_INVALID || nextBlock >= TOTAL_BLOCKS) {
//             break; // Exit the loop if an invalid block index is encountered
//         }
//         fat_free_block(block);
//         block = nextBlock;
//     }
//     entry->start_block = fat_allocate_block();
//     entry->size = 0;
//     if (entry->start_block == FAT_NO_FREE_BLOCKS) {
//         printf("Error: Unable to allocate a new block for file.\n");
//         // Consider additional error handling here
//     }
// }

void reset_file_content(FileEntry* entry) {
    uint32_t block = entry->start_block;
    uint32_t nextBlock;
    int result;

    // Continue looping until we reach the end of the file's block chain
    while (block != FAT_ENTRY_END && block < TOTAL_BLOCKS) {
        result = fat_get_next_block(block, &nextBlock);
        // Free the current block before moving to the next
        fat_free_block(block);

        // If we encounter an error or the end of the chain, stop processing
        if (result != FAT_SUCCESS || nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Exit the loop if an error occurs or the end of the chain is reached
        }

        block = nextBlock; // Move to the next block in the chain
    }

    // Allocate a new start block for the file and reset its size
    entry->start_block = fat_allocate_block();
    entry->size = 0;
    if (entry->start_block == FAT_NO_FREE_BLOCKS) {
        printf("Error: Unable to allocate a new block for the file.\n");
        fflush(stdout);
        // Additional error handling could be implemented here if necessary
    }
}


FileMode determine_mode(const char* mode) {
    if (strcmp(mode, "r") == 0) return MODE_READ;
    if (strcmp(mode, "w") == 0) return MODE_WRITE;
    if (strcmp(mode, "a") == 0) return MODE_APPEND;
    printf("Error: Unrecognized file mode '%s'. Defaulting to read mode.\n", mode);
    fflush(stdout);
    return MODE_READ; // Default to read if mode is unrecognized
}



/**
 * Closes the specified file.
 *
 * @param file A pointer to the file to be closed.
 */
void fs_close(FS_FILE* file) {
    if (file == NULL) {
        printf("Error: Attempted to close a NULL file pointer.\n");
        fflush(stdout);
        return;
    }

    free(file);
}

/**
 * Reads data from the specified file into the provided buffer.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to read from.
 * @param buffer A pointer to the buffer where the read data should be stored.
 * @param size   The number of bytes to read.
 * @return The number of bytes actually read, or -1 if an error occurred.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    if (file == NULL || buffer == NULL || size <= 0) {
        printf("Error: Invalid parameters for fs_read.\n");
        fflush(stdout);
        return -1;
    }

    if (file->mode != MODE_READ && file->mode != MODE_APPEND) {
        printf("Error: File is not open in a readable mode.\n");
        fflush(stdout);
        return -1;
    }
    // printf("Starting fs_read: file position before read = %u, read size = %d\n", file->position, size);
    // fflush(stdout);
    int totalBytesRead = 0;
    uint8_t* buf = (uint8_t*)buffer;

    // Calculate initial blockIndex and blockOffset based on file->position
    uint32_t blockIndex = file->position / FILESYSTEM_BLOCK_SIZE;
    uint32_t blockOffset = file->position % FILESYSTEM_BLOCK_SIZE;

    while (size > 0 && file->position < file->entry->size) {
        // Navigate through the FAT to find the correct block based on blockIndex
        uint32_t currentBlock = file->entry->start_block;
        // Print debug for current block
        printf("fs_read : Current block: %u\n", currentBlock);
        uint32_t nextBlock;
        int result;
        for (uint32_t i = 0; i < blockIndex; i++) {
            result = fat_get_next_block(currentBlock, &nextBlock);
            if (result == FAT_END_OF_CHAIN || result == FAT_CORRUPTED) {
                printf("Error: Reached end of file chain or encountered corruption.\n");
                fflush(stdout);
                return -1; // Handle the error appropriately
            }
            currentBlock = nextBlock;
        }

        
        if (currentBlock == FAT_ENTRY_END) {
            printf("Error: Reached end of file chain prematurely.\n");
            fflush(stdout);
            break; // Break the loop if the end of the chain is reached unexpectedly
        }

        // Calculate the number of bytes to read in this iteration
        int bytesToRead = min(size, FILESYSTEM_BLOCK_SIZE - blockOffset);
        bytesToRead = min(bytesToRead, (int)(file->entry->size - file->position));

        // Calculate the read offset for the current block
        uint32_t readOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + blockOffset;
        printf("Performing read from block = %u, offset within block = %u (global readOffset = %u)\n", currentBlock, blockOffset, readOffset);
        fflush(stdout);

        // Perform the actual read operation
        flash_read_safe(readOffset, buf + totalBytesRead, bytesToRead);

        // Update counters and positions for the next iteration
        totalBytesRead += bytesToRead;
        file->position += bytesToRead;
        size -= bytesToRead;

        // Reset blockOffset to 0 for subsequent blocks
        blockOffset = 0;

        // Move to the next block in the FAT chain, if needed
        if (bytesToRead + blockOffset >= FILESYSTEM_BLOCK_SIZE) {
            blockIndex++;
        }
    }

    // printf("Completed fs_read: totalBytesRead = %d, final file position = %u\n", totalBytesRead, file->position);
    // fflush(stdout);
    return totalBytesRead;
}

/**
 * Writes data from the provided buffer to the specified file.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to write to.
 * @param buffer A pointer to the buffer containing the data to write.
 * @param size   The number of bytes to write.
 * @return The number of bytes actually written, or -1 if an error occurred.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid parameters for fs_write.\n");
        fflush(stdout);
        return -1;
    }

    if (file->mode != MODE_WRITE && file->mode != MODE_APPEND) {
        printf("Error: File is not open in a write or append mode.\n");
        fflush(stdout);
        return -1;
    }

    const uint8_t* buf = (const uint8_t*)buffer;
    int totalBytesWritten = 0;
    int bytesToWrite = size;
    // printf("Starting fs_write: file position before write = %u, write size = %d\n", file->position, size);
    // fflush(stdout);

    uint32_t currentBlockIndex = file->position / FILESYSTEM_BLOCK_SIZE;
    uint32_t blockOffset = file->position % FILESYSTEM_BLOCK_SIZE;
    uint32_t physicalBlock = file->entry->start_block;
    uint32_t nextBlock;
    int result;

    // Navigate to the current block based on the file position
    for (uint32_t i = 0; i < currentBlockIndex; ++i) {
        result = fat_get_next_block(physicalBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT: Error code %d\n", result);
            fflush(stdout);
            return -1;
        }
        physicalBlock = nextBlock;
    }

    while (bytesToWrite > 0) {
        uint32_t availableInBlock = FILESYSTEM_BLOCK_SIZE - blockOffset;

        // Check if we need to allocate a new block
        if (availableInBlock == 0 || physicalBlock == FAT_ENTRY_END) {
            uint32_t newBlock = fat_allocate_block();
            if (newBlock == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left to allocate new block for writing.\n");
                fflush(stdout);
                return -1;
            }
            if (physicalBlock != FAT_ENTRY_END) {
                fat_link_blocks(physicalBlock, newBlock);
            } else {
                // This case should be handled if we're appending to a new file
                file->entry->start_block = newBlock;
            }
            physicalBlock = newBlock;
            availableInBlock = FILESYSTEM_BLOCK_SIZE;
            blockOffset = 0;
        }

        uint32_t writeSize = (bytesToWrite < availableInBlock) ? bytesToWrite : availableInBlock;
        uint32_t writeOffset = physicalBlock * FILESYSTEM_BLOCK_SIZE + blockOffset;

        flash_write_safe(writeOffset, buf + totalBytesWritten, writeSize);

        totalBytesWritten += writeSize;
        file->position += writeSize;
        bytesToWrite -= writeSize;
        blockOffset += writeSize; // Update blockOffset for next iteration

        // Prepare for next iteration, possibly moving to the next block
        if (blockOffset == FILESYSTEM_BLOCK_SIZE) {
            blockOffset = 0; // Reset offset for next block
            result = fat_get_next_block(physicalBlock, &nextBlock);
            if (result == FAT_SUCCESS) {
                physicalBlock = nextBlock; // Move to the next block in the chain
            }
            // Note: If result is not FAT_SUCCESS, it means we're at the end of the chain, which will trigger a new block allocation in the next loop iteration if needed.
        }
    }

    // Update the file size in its directory entry if we've extended the file
    if (file->position > file->entry->size) {
        file->entry->size = file->position;
        // printf("Updated file size: %u\n", file->entry->size);
        fflush(stdout);
    }

    printf("Completed fs_write: totalBytesWritten = %d, final file position = %u\n", totalBytesWritten, file->position);
    fflush(stdout);
    return totalBytesWritten;
}




/**
 * Sets the file position indicator for the specified file.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to seek in.
 * @param offset The number of bytes to offset from whence.
 * @param whence The reference point from which to offset (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return 0 on success, or -1 if an error occurred.
 */
int fs_seek(FS_FILE* file, long offset, int whence) {
    // Validate the file pointer
    if (file == NULL) {
        printf("Error: Invalid file pointer provided to fs_seek.\n");
        return -1;
    }

    // Calculate the new position based on the 'whence' parameter
    long new_position;
    switch (whence) {
        case SEEK_SET:
            new_position = offset; // Set position relative to file start
            break;
        case SEEK_CUR:
            new_position = file->position + offset; // Set position relative to current position
            break;
        case SEEK_END:
            new_position = file->entry->size + offset; // Set position relative to file end
            break;
        default:
            printf("Error: Invalid 'whence' argument provided to fs_seek (%d).\n", whence);
            fflush(stdout);
            return -1;
    }

    // Validate the new position
    if (new_position < 0) {
        printf("Error: Seek operation results in a negative file position (%ld).\n", new_position);
        fflush(stdout);
        return -1;
    }
    if (new_position > file->entry->size) {
        // Optionally, allow seeking beyond the current file size for write/append operations
        if (file->mode == MODE_WRITE || file->mode == MODE_APPEND) {
            // Extend the file size if in write or append mode
            printf("Notice: Seeking beyond the current file size. File will be extended.\n");
            fflush(stdout);
            file->entry->size = new_position;
        } else {
            printf("Error: Seek operation results in a position beyond the end of the file (%ld).\n", new_position);
            fflush(stdout);
            return -1;
        }
    }

    // Set the new file position
    file->position = new_position;
    return 0; // Indicate success
}


////////////////////////////////////// 
 
int fs_mv(const char* old_path, const char* new_path) {
    // Validate input paths
    if (old_path == NULL || new_path == NULL) {
        printf("Error: Both old and new paths must be provided.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure source and destination are not the same
    if (strcmp(old_path, new_path) == 0) {
        printf("Error: Source and destination paths are the same.\n");
        fflush(stdout);
        return -2; // Source and destination are the same
    }

    // Ensure the source file exists
    FileEntry* srcEntry = find_file_by_path(old_path);
    if (srcEntry == NULL) {
        printf("Error: Source file does not exist.\n");
        fflush(stdout);
        return -3; // Source file not found
    }

    // Ensure the destination file does not exist
    FileEntry* destEntry = find_file_by_path(new_path);
    if (destEntry != NULL) {
        printf("Error: Destination file already exists.\n");
        fflush(stdout);
        return -4; // Destination file exists
    }

    // Perform the move operation
    // In a simple filesystem, this might just be renaming the entry
    // Ensure there's space in the filename field
    if (strlen(new_path) >= sizeof(srcEntry->filename)) {
        printf("Error: New path is too long.\n");
        fflush(stdout);
        return -5; // New path too long
    }

    // Update the file entry with the new path
    strncpy(srcEntry->filename, new_path, sizeof(srcEntry->filename));
    srcEntry->filename[sizeof(srcEntry->filename) - 1] = '\0'; // Ensure null termination

    printf("File moved from '%s' to '%s'.\n", old_path, new_path);
    fflush(stdout);
    return 0; // Success
}

/////////////


int fs_wipe(const char* path) {
    printf("Wiping path: %s\n", path);
    // Check if path is NULL
    if (path == NULL) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Indicate an error
    }

    // Find the file or directory entry
    FileEntry* entry = find_file_by_path(path);
    if (entry == NULL) {
        printf("Error: File or directory not found.\n");
        fflush(stdout);
        return -2; // Indicate file not found
    }

    // If it's a directory, recursively wipe its contents
    if (entry->is_directory) {
        // The implementation here assumes you have a way to list and iterate through directory contents.
        DirectoryEntry* dirEntries = fs_list_directory(path);  
        for (int i = 0; dirEntries[i].name[0] != '\0'; i++) {
            // Construct the full path for each entry
            char fullPath[256]; // Assuming 256 is the max path length
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, dirEntries[i].name);

            // Recursively call fs_wipe on each entry
            int result = fs_wipe(fullPath);
            if (result != 0) {
                printf("Warning: Failed to wipe '%s' within directory.\n", fullPath);
                fflush(stdout);
                // Decide how to handle partial failures. Continue, abort, or revert?
            }
        }
        // Depending on your design, you may need to free any allocated memory for dirEntries
    }

    uint32_t currentBlock = entry->start_block;
    uint32_t nextBlock;
    int result;

    // Deallocate blocks associated with this file or directory
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered FAT_ENTRY_END prematurely. Error code: %d\n", result);
            fflush(stdout);
            break; // Handle error or premature end appropriately
        }
        fat_free_block(currentBlock);
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Properly end loop if at the end or if an invalid next block is encountered
        }
        currentBlock = nextBlock;
    }

    // Mark the file system entry as not in use and optionally clear it
    entry->in_use = false;
    memset(entry, 0, sizeof(FileEntry));

    return 0; // Indicate success
}

///////////////////////////////////////////
 
extern void flash_erase_all(void);

int fs_format(const char* path) {
    // Ensure the format request is intended for the root directory
    if (strcmp(path, "/") != 0) {
        printf("Error: fs_format only supports formatting the root directory.\n");
        fflush(stdout);
        return -1;
    }

    // Securely erase all flash storage used by the filesystem
    flash_erase_all();

    // Reinitialize the FAT to mark all blocks as free
    fat_init();

    // Reset or reinitialize the root directory
    int result = reset_root_directory();
    if (result != 0) {
        printf("Error: Failed to reset the root directory (error %d).\n", result);
        fflush(stdout);
        return result;
    }

    printf("Filesystem formatted successfully.\n");
    fflush(stdout);
    return 0; // Indicate success
}



////////////////////////////

int fs_cp(const char* source_path, const char* dest_path) {
    // Validate input paths
    if (!source_path || !dest_path) {
        printf("Error: Source or destination path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure source file exists
    FS_FILE* srcFile = fs_open(source_path, "r");
    if (srcFile == NULL) {
        printf("Error: Source file '%s' does not exist.\n", source_path);
        fflush(stdout);
        return -2; // Source file not found
    }

    // Check if destination file already exists to avoid unintentional overwrite
    FS_FILE* destFileCheck = fs_open(dest_path, "r");
    if (destFileCheck != NULL) {
        printf("Error: Destination file '%s' already exists.\n", dest_path);
        fflush(stdout);
        fs_close(destFileCheck); // Close the file opened for checking
        fs_close(srcFile); // Don't forget to close the source file as well
        return -3; // Destination file exists
    }

    // Open destination file for writing (creates the file)
    FS_FILE* destFile = fs_open(dest_path, "w");
    if (destFile == NULL) {
        printf("Error: Failed to create destination file '%s'.\n", dest_path);
        fflush(stdout);
        fs_close(srcFile); // Ensure the source file is closed to prevent resource leakage
        return -4; // Failed to create destination file
    }

    // Copy content from source to destination
    char buffer[512]; // Choose an appropriate buffer size based on your system's constraints
    int bytesRead, bytesWritten;
    while ((bytesRead = fs_read(srcFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = fs_write(destFile, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            printf("Error: Failed to write to destination file '%s'.\n", dest_path);
            fflush(stdout);
            fs_close(srcFile);
            fs_close(destFile);
            return -5; // Write failure
        }
    }

    if (bytesRead < 0) {
        printf("Error: Failed to read from source file '%s'.\n", source_path);
        fflush(stdout);
        fs_close(srcFile);
        fs_close(destFile);
        return -6; // Read failure
    }

    // Close files to release resources
    fs_close(srcFile);
    fs_close(destFile);

    printf("File '%s' successfully copied to '%s'.\n", source_path, dest_path);
    fflush(stdout);
    return 0; // Success
}




//////////////////////////////
 
int fs_rm(const char* path) {
    // Validate the input path
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure the file exists
    FileEntry* fileEntry = find_file_by_path(path);
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
    fflush(stdout);
    return 0; // Success
}








FileEntry* resolve_path(const char* path) {
    if (path == NULL || path[0] != '/') {
        printf("Error: Invalid path. Path must start with '/'\n");
        return NULL; // Invalid path
    }

    if (strcmp(path, "/") == 0) {
        // Special case for root directory
        return find_file_by_path(path);
    }

    char pathCopy[256]; // Assuming path length won't exceed 255 characters
    strncpy(pathCopy, path, sizeof(pathCopy));
    pathCopy[sizeof(pathCopy) - 1] = '\0'; // Ensure null termination

    char* token = strtok(pathCopy, "/");
    FileEntry* currentEntry = NULL;
    FileEntry* parentEntry = NULL;

    while (token != NULL) {
        char fullPath[256] = "/";
        if (parentEntry != NULL) {
            strncat(fullPath, parentEntry->filename, sizeof(fullPath) - strlen(fullPath) - 1);
            strncat(fullPath, "/", sizeof(fullPath) - strlen(fullPath) - 1);
        }
        strncat(fullPath, token, sizeof(fullPath) - strlen(fullPath) - 1);

        currentEntry = find_file_by_path(fullPath);
        if (currentEntry == NULL) {
            // Path resolution failed, entry not found
            printf("Error: Path resolution failed at '%s'.\n", token);
            return NULL;
        }

        parentEntry = currentEntry; // Move to the next level
        token = strtok(NULL, "/");
    }

    return currentEntry;
}














