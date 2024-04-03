#include "filesystem.h"
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> // For free()
#include "flash_ops.h"
#include "pico/stdlib.h" // Include Pico SDK stdlib
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "fat_fs.h"

#include "flash_config.h"


FileEntry fileSystem[MAX_FILES];

/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
 
    fat_init();

    uint32_t current_start_block = FLASH_TARGET_OFFSET;

    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;  // Initially, no file is in use
        fileSystem[i].size = 0;        // Initialize the file size to 0

        // Initialize the flash address for each file entry
        fileSystem[i].start_block = current_start_block;

        // Move the current flash address to the start of the next sector
        // making sure each file starts at the beginning of a new sector
        current_start_block += MAX_FILE_SIZE;

        // Check if the new flash address exceeds the flash size, wrap around, or handle as error
        if (current_start_block >= FLASH_TARGET_OFFSET + FLASH_USABLE_SPACE) {
            // Handle the case where we've exceeded the flash space
            // This could be an error, or you might want to wrap around
            printf("Error: Not enough flash memory for the number of files.\n");
            return;
        }
    }
}



FileEntry* find_file_by_path(const char* path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, path) == 0) {
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
        return NULL;
    }

    FileEntry* entry = find_file_by_path(path);

    // If the file does not exist and the mode is not for reading, attempt to create a new file entry
    if (entry == NULL && (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0)) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!fileSystem[i].in_use) {
                entry = &fileSystem[i];
                strncpy(entry->filename, path, sizeof(entry->filename) - 1);
                entry->filename[sizeof(entry->filename) - 1] = '\0';
                entry->size = 0;
                entry->in_use = true;
                // Allocate the first block for the file
                entry->start_block = fat_allocate_block();
                if (entry->start_block == FAT_ENTRY_FULL) {
                    printf("Error: No space left on device to create new file.\n");
                    entry->in_use = false; // Reset the file entry as allocation failed
                    return NULL;
                }
                break;
            }
        }
        if (entry == NULL) {
            printf("Error: Filesystem is full, cannot create new file.\n");
            return NULL;
        }
    } else if (entry == NULL) {
        printf("Error: File not found for reading.\n");
        return NULL;
    }

    // If opening a file in write mode, reset its content
    if (strcmp(mode, "w") == 0 && entry != NULL) {
        // Free all blocks currently allocated to this file
        uint32_t currentBlock = entry->start_block;
        while (currentBlock != FAT_ENTRY_END) {
            uint32_t nextBlock = fat_get_next_block(currentBlock);
            fat_free_block(currentBlock);
            currentBlock = nextBlock;
        }
        // Allocate a new starting block for the file
        entry->start_block = fat_allocate_block();
        if (entry->start_block == FAT_ENTRY_FULL) {
            printf("Error: No space left on device to allocate new block.\n");
            return NULL;
        }
        entry->size = 0;
    }

    FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
    if (file == NULL) {
        printf("Error: Memory allocation failed for FS_FILE.\n");
        return NULL;
    }

    file->entry = entry;
    file->position = 0; // Default position is the start of the file
    if (strcmp(mode, "a") == 0) {
        // For append mode, set position to the end of the file
        file->position = entry->size;
    }
    file->mode = (strcmp(mode, "r") == 0) ? MODE_READ : (strcmp(mode, "w") == 0) ? MODE_WRITE : MODE_APPEND;

    return file;
}


/**
 * Closes the specified file.
 *
 * @param file A pointer to the file to be closed.
 */
void fs_close(FS_FILE* file) {
    if (file == NULL) {
        printf("Error: Attempted to close a NULL file pointer.\n");
        return;
    }

    free(file);
}

/**
 * Reads data from the specified file into the provided buffer.
 *
 * @param file   A pointer to the FS_FILE structure representing the file from which to read.
 * @param buffer A pointer to the buffer where the read data will be stored.
 * @param size   The maximum number of bytes to read.
 * @return The number of bytes actually read, or -1 if an error occurred.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid parameters for fs_read.\n");
        return -1;
    }

    if (file->mode != MODE_READ && file->mode != MODE_APPEND) {
        printf("Error: File is not open in a readable mode.\n");
        return -1;
    }

    // Initialize variables to keep track of how much data to read and how much has been read
    int totalBytesRead = 0;
    int bytesToRead = size;
    uint8_t* buf = (uint8_t*)buffer;

    // Calculate starting block and offset within that block based on the file's current position
    uint32_t blockIndex = file->entry->start_block;
    uint32_t blockOffset = file->position % FLASH_BLOCK_SIZE;
    uint32_t remainingFileSize = file->entry->size - file->position;

    // Adjust bytesToRead if the request exceeds the file size
    if (bytesToRead > remainingFileSize) {
        bytesToRead = remainingFileSize;
    }

    // Loop until all requested data is read or the end of the file is reached
    while (bytesToRead > 0 && blockIndex != FAT_ENTRY_END) {
        uint32_t bytesReadFromBlock = 0;
        if (bytesToRead + blockOffset <= FLASH_BLOCK_SIZE) {
            // If the remaining data to read fits within the current block
            bytesReadFromBlock = bytesToRead;
        } else {
            // If the data to read spans across blocks
            bytesReadFromBlock = FLASH_BLOCK_SIZE - blockOffset;
        }

        // Read data from the current block
        flash_read_safe(blockIndex * FLASH_BLOCK_SIZE + blockOffset, buf + totalBytesRead, bytesReadFromBlock);

        // Update counters and pointers for the next iteration
        totalBytesRead += bytesReadFromBlock;
        bytesToRead -= bytesReadFromBlock;
        file->position += bytesReadFromBlock;

        // Move to the next block in the chain, reset blockOffset for subsequent blocks
        blockIndex = fat_get_next_block(blockIndex);
        blockOffset = 0;
    }

    return totalBytesRead;
}


/**
 * Writes data from the provided buffer to the specified file.
 *
 * This function writes 'size' bytes from 'buffer' into the file pointed to by 'file'.
 * Writing starts from the current file position. If the write operation goes beyond
 * the end of the file, the file size should be updated accordingly.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to write to.
 * @param buffer A pointer to the buffer containing the data to be written.
 * @param size   The number of bytes to write.
 * @return The number of bytes written, or -1 if an error occurred.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid parameters for fs_write.\n");
        return -1;
    }

    if (file->mode != MODE_WRITE && file->mode != MODE_APPEND) {
        printf("Error: File is not open in a write or append mode.\n");
        return -1;
    }

    // Initialize variables for tracking write operations
    const uint8_t* buf = (const uint8_t*)buffer;
    int totalBytesWritten = 0;
    int bytesToWrite = size;
    uint32_t currentBlock = file->entry->start_block;
    uint32_t blockOffset = file->position % FLASH_BLOCK_SIZE;
    uint32_t previousBlock = FAT_ENTRY_END;

    // Loop to write data across multiple blocks if necessary
    while (bytesToWrite > 0) {
        // Check if we need to allocate a new block
        if (currentBlock == FAT_ENTRY_END) {
            currentBlock = fat_allocate_block();
            if (currentBlock == FAT_ENTRY_FULL) {
                printf("Error: No space left to allocate new block for writing.\n");
                return -1; // Returning the number of bytes written so far might be more useful in some contexts
            }

            // Link the newly allocated block to the previous block in the chain
            if (previousBlock != FAT_ENTRY_END) {
                fat_link_blocks(previousBlock, currentBlock);
            } else {
                // This is the first block of the file
                file->entry->start_block = currentBlock;
            }
        }

        // Calculate how much data to write to the current block
        int writeSize = FLASH_BLOCK_SIZE - blockOffset;
        if (writeSize > bytesToWrite) {
            writeSize = bytesToWrite;
        }

        // Perform the write operation
        flash_write_safe(currentBlock * FLASH_BLOCK_SIZE + blockOffset, buf + totalBytesWritten, writeSize);

        // Update counters and move to the next block if necessary
        totalBytesWritten += writeSize;
        bytesToWrite -= writeSize;
        file->position += writeSize;
        if (file->position > file->entry->size) {
            // Update the file size if we've extended it
            file->entry->size = file->position;
        }

        // Prepare for the next iteration
        previousBlock = currentBlock;
        currentBlock = fat_get_next_block(currentBlock);
        blockOffset = 0; // Reset offset for new blocks
    }

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
    if (file == NULL) {
        printf("Error: Invalid file pointer provided to fs_seek.\n");
        return -1;
    }

    long new_position = 0; // Initialize new position

    switch (whence) {
        case SEEK_SET:
            // Offset from the beginning of the file
            new_position = offset;
            break;
        case SEEK_CUR:
            // Current position plus offset
            new_position = file->position + offset;
            break;
        case SEEK_END:
            // End of the file plus offset
            new_position = file->entry->size + offset;
            break;
        default:
            printf("Error: Invalid 'whence' argument provided to fs_seek.\n");
            return -1;
    }

    // Check if the new position is valid
    if (new_position < 0 || new_position > file->entry->size) {
        printf("Error: Seek operation results in an invalid file position (%ld).\n", new_position);
        return -1;
    }

    // Set the file's current position to the new position
    file->position = new_position;
    return 0; // Seek operation successful
}
