#include "filesystem.h"
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> // For free()
#include "flash_ops.h"
#include "pico/stdlib.h" // Include Pico SDK stdlib
#include "hardware/sync.h"
#include "hardware/flash.h"
 




#define FLASH_SIZE PICO_FLASH_SIZE_BYTES 
#define FLASH_TARGET_OFFSET (256 * 1024)
// No need to define FLASH_TOTAL_SIZE as it is provided by PICO_FLASH_SIZE_BYTES

#define FLASH_METADATA_SPACE (256 * 1024)  // Space reserved for metadata and wear leveling
#define FLASH_USABLE_SPACE (PICO_FLASH_SIZE_BYTES - FLASH_METADATA_SPACE) // Usable space for files


#define MAX_FILES 10 // Maximum number of files

// Calculate the maximum file size aligned to the flash sector size
#define MAX_FILE_SIZE ((FLASH_USABLE_SPACE / MAX_FILES) & ~(FLASH_SECTOR_SIZE - 1))













FileEntry fileSystem[MAX_FILES];

/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
    // Assume FLASH_TARGET_OFFSET is where your file system starts in flash memory
    // Assume FLASH_SECTOR_SIZE is the size of each sector
    // Define MAX_FILE_SIZE as the max size for each file which should be equal or less than FLASH_SECTOR_SIZE
    // and aligned to the sector boundary if FLASH_SECTOR_SIZE is not a multiple of MAX_FILE_SIZE.

    uint32_t current_flash_address = FLASH_TARGET_OFFSET;

    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;  // Initially, no file is in use
        fileSystem[i].size = 0;        // Initialize the file size to 0

        // Initialize the flash address for each file entry
        fileSystem[i].flash_address = current_flash_address;

        // Move the current flash address to the start of the next sector
        // making sure each file starts at the beginning of a new sector
        current_flash_address += MAX_FILE_SIZE;

        // Check if the new flash address exceeds the flash size, wrap around, or handle as error
        if (current_flash_address >= FLASH_TARGET_OFFSET + FLASH_USABLE_SPACE) {
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
    // Check if the input path and mode are valid
    if (path == NULL || mode == NULL) {
        printf("Error: Path or mode is NULL.\n");
        return NULL;
    }

    // Attempt to find the file entry in the file system
    FileEntry* entry = find_file_by_path(path);

    // Handle opening in read mode
    if (strcmp(mode, "r") == 0) {
        // The file must exist for read mode
        if (entry == NULL) {
            printf("Error: File not found for reading.\n");
            return NULL;
        }
    } else if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0) {
        // For write or append mode, create a new file if it does not exist
        if (entry == NULL) {
            // Find a free slot for the new file
            for (int i = 0; i < MAX_FILES; i++) {
                if (!fileSystem[i].in_use) {
                    entry = &fileSystem[i];
                    // Initialize the new file entry
                    strncpy(entry->filename, path, sizeof(entry->filename) - 1);
                    entry->filename[sizeof(entry->filename) - 1] = '\0'; // Null-terminate the string
                    entry->size = 0; // Set the file size to 0
                    entry->in_use = true; // Mark the file as in use
                    // TODO: Assign entry->flash_address based on your flash memory layout
                    // entry->flash_address = calculate_flash_address_for_file(i);
                    break;
                }
            }
            if (entry == NULL) {
                printf("Error: Filesystem is full, cannot create new file.\n");
                return NULL;
            }
        } else if (strcmp(mode, "w") == 0) {
            // If opening an existing file in write mode, erase its content
            // Erase the file's allocated flash memory before writing new content
            // This is an important step as flash memory must be erased before being rewritten
            flash_erase_safe(entry->flash_address);
            entry->size = 0; // Reset the file size as the content is erased
        }
    } else {
        // Unsupported mode
        printf("Error: Unsupported file mode '%s'.\n", mode);
        return NULL;
    }

    // Allocate and initialize a new FS_FILE structure for the opened file
    FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
    if (file == NULL) {
        printf("Error: Memory allocation failed for FS_FILE.\n");
        return NULL;
    }

    // Initialize the FS_FILE structure fields
    file->entry = entry;
    file->position = (strcmp(mode, "a") == 0) ? entry->size : 0; // If appending, set position to the end of the file
    // Set the mode of the file based on the input parameter
    if (strcmp(mode, "r") == 0) {
        file->mode = MODE_READ;
    } else if (strcmp(mode, "w") == 0) {
        file->mode = MODE_WRITE;
    } else if (strcmp(mode, "a") == 0) {
        file->mode = MODE_APPEND;
    }

    return file;
}


/**
 * Closes the specified file.
 *
 * @param file A pointer to the file to be closed.
 */
void fs_close(FS_FILE* file) {
    // First, check if the provided file pointer is valid. This helps prevent
    // dereferencing a NULL pointer, which could lead to undefined behavior.
    if (file == NULL) {
        printf("Error: Attempted to close a NULL file pointer.\n");
        return; // Early exit if the file pointer is invalid.
    }

    // If the file was opened for writing ('w' mode), you might need to flush
    // any buffered data to the flash memory. This is critical to ensure data
    // integrity and that all writes are finalized before closing the file.
    // Note: This step is simplified in this example, as actual data writing
    // to flash would require integration with the flash_ops provided functions.
    //
    // Example:
    // if (file->mode == MODE_WRITE) {
    //     flash_write_safe(file->entry->start_block * BLOCK_SIZE,
    //                      file->buffer, file->buffer_size);
    // }

    // Free the FS_FILE structure. This is important to avoid memory leaks,
    // ensuring that the memory allocated for the FS_FILE structure during
    // fs_open is properly released when the file is no longer needed.
    free(file);

    // Optionally, you could also set the file pointer to NULL to help prevent
    // use-after-free errors. However, since the pointer is passed by value,
    // this change would not affect the caller's copy of the pointer.
    // A better practice is for the caller to ensure the pointer is not used
    // after calling fs_close.

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
    // Validate input parameters
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid parameters for fs_read.\n");
        return -1;
    }

    // Check if the file is opened in a mode that allows reading
    if (file->mode != MODE_READ && file->mode != MODE_APPEND) {
        printf("Error: File is not open in a readable mode.\n");
        return -1;
    }

    // Calculate the actual number of bytes to read
    // Should not read beyond the end of the file
    int bytesToRead = file->position + size > file->entry->size ? file->entry->size - file->position : size;
    
    // If attempting to read more bytes than available, adjust the count
    if (bytesToRead > file->entry->size - file->position) {
        bytesToRead = file->entry->size - file->position;
    }
    
    // Check if there's anything to read
    if (bytesToRead <= 0) {
        return 0;  // No more data left to read
    }

    // Use flash_read_safe to read from flash memory into the buffer
    flash_read_safe(file->entry->flash_address + file->position, buffer, bytesToRead);

    // Update the file's current read position
    file->position += bytesToRead;

    return bytesToRead;  // Return the number of bytes read
}


/**
 * Writes data from the provided buffer to the specified file.
 *
 * This function writes 'size' bytes from 'buffer' into the file pointed to by 'file'.
 * Writing starts from the current file position. If the write operation goes beyond
 * the end of the file, the file size should be updated accordingly.
 *
 * Note: This simplistic implementation assumes each file occupies a single block and
 * does not handle the case where a write operation spans multiple blocks.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to write to.
 * @param buffer A pointer to the buffer containing the data to be written.
 * @param size   The number of bytes to write.
 * @return The number of bytes written, or -1 if an error occurred.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    printf("Debug: Entering fs_write.\n"); // Entering the function
    // Validate the input parameters
    if (file == NULL || buffer == NULL) {
        printf("Error: Invalid file or buffer pointer.\n");
        return -1;
    }
    printf("Debug: Parameters validated.\n");

    // Check if the file is in write mode
    if (file->mode != MODE_WRITE && file->mode != MODE_APPEND) {
        printf("Error: File is not in write or append mode.\n");
        return -1;
    }
    printf("Debug: File mode checked and is valid.\n");
    // Check if the write size is within the bounds of the file system
    if (size < 0 || size > MAX_FILE_SIZE) {
        printf("Error: Write size is out of bounds.\n");
        return -1;
    }
    printf("Debug: Write size is within bounds.\n"); // Size is valid
    // Calculate the write position and ensure it doesn't exceed the file system limits
    uint32_t writePosition = (file->mode == MODE_APPEND) ? file->entry->size : file->position;
    printf("Debug: Calculated write position: %u\n", writePosition); // Write position calculated

    if (writePosition + size > MAX_FILE_SIZE) {
        printf("Error: Write exceeds maximum file size.\n");
        return -1;
    }
    printf("Debug: Write does not exceed maximum file size.\n"); // File size check passed
    // Flash memory must be erased before writing
    uint32_t sectorStart = writePosition + file->entry->flash_address;
    uint32_t sectorEnd = sectorStart + size;
    printf("Debug: Sector start: %u, Sector end: %u\n", sectorStart, sectorEnd); // Sector start and end calculated
    // Loop through sectors to erase and write data
    while (sectorStart < sectorEnd) {
        // Calculate the start of the sector by aligning the address
        uint32_t sectorBase = sectorStart & ~(FLASH_SECTOR_SIZE - 1);
        printf("Debug: Sector base: %u\n", sectorBase); // Sector base calculated
        // Erase the flash sector
        flash_erase_safe(sectorBase);
        printf("Debug: Sector erased: %u\n", sectorBase); // Sector erased
        // Determine the amount of data to write in this sector
        int writeLength = FLASH_SECTOR_SIZE - (sectorStart - sectorBase);
        if (writeLength > (sectorEnd - sectorStart)) {
            writeLength = sectorEnd - sectorStart;
        }
        printf("Debug: Calculated write length: %d\n", writeLength); // Write length calculated
        printf("Debug: sectorStart IS: %d\n", sectorStart); // Write length calculated

        // Write data to flash
        flash_write_safe(sectorStart, buffer, writeLength);
        printf("Debug: Data written. Sector start: %u, Length: %d\n", sectorStart, writeLength); // Data written
        // Move to the next sector
        sectorStart += writeLength;
        printf("Debug: Sector start updated: %u\n", sectorStart); // Sector start updated
        buffer += writeLength;
    }

    // Update the file's size and current position
    file->entry->size = writePosition + size;
    file->position += size;
    printf("Debug: File size and position updated. Size: %u, Position: %u\n", file->entry->size, file->position); // File size and position updated
    printf("Debug: Exiting fs_write. Bytes written: %d\n", size); // Exiting function
    return size; // Return the number of bytes written
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

    long newPosition; // Declare a variable to calculate the new position

    switch (whence) {
        case SEEK_SET:
            // Set position relative to the beginning of the file
            newPosition = offset;
            break;
        case SEEK_CUR:
            // Set the new position relative to the current position
            newPosition = file->position + offset;
            break;
        case SEEK_END:
            // Set the new position relative to the end of the file
            newPosition = file->entry->size + offset;
            break;
        default:
            printf("Error: Invalid 'whence' argument provided to fs_seek.\n");
            return -1; // Return error if 'whence' is not recognized
    }

    // Check if the new position is valid (not beyond the start or end of the file)
    if (newPosition < 0 || newPosition > file->entry->size) {
        printf("Error: Seek operation results in an invalid file position.\n");
        return -1;
    }

    // Update the file's current position
    file->position = newPosition;
    return 0; // Return success
}


