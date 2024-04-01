#include "filesystem.h"
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> // For free()
#include "flash_ops.h"


#define USER_DATA_SIZE (2 * 1024 * 1024 - 256 * 1024) // Calculate usable space for files
#define MAX_FILE_SIZE (USER_DATA_SIZE / MAX_FILES)





FileEntry fileSystem[MAX_FILES];

/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false; // Initially, no file is in use
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
    FileEntry* entry = find_file_by_path(path);

    if (strcmp(mode, "r") == 0) {
        // For read mode, the file must exist
        if (entry == NULL) {
            printf("Error: File not found for reading.\n");
            return NULL;
        }
    } else if (strcmp(mode, "w") == 0) {
        // For write mode, create the file if it does not exist
        if (entry == NULL) {
            for (int i = 0; i < MAX_FILES; i++) {
                if (!fileSystem[i].in_use) {
                    // Found a free slot, use it for the new file
                    entry = &fileSystem[i];
                    strncpy(entry->filename, path, sizeof(entry->filename) - 1);
                    entry->filename[sizeof(entry->filename) - 1] = '\0'; // Ensure null termination
                    entry->size = 0; // New file, size is 0
                    entry->in_use = true;
                    break;
                }
            }
        }
        if (entry == NULL) {
            printf("Error: Filesystem is full, cannot create new file.\n");
            return NULL;
        }
    } else {
        // This block handles unsupported modes. It's good practice to handle this explicitly.
        printf("Error: Unsupported file mode '%s'.\n", mode);
        return NULL;
    }

    // Allocate a new FS_FILE structure for the opened file
    FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
    if (!file) {
        printf("Error: Memory allocation failed for FS_FILE.\n");
        return NULL;
    }

    // Initialize the FS_FILE structure
    file->entry = entry;
    file->position = 0; // Start at the beginning of the file

    // Set the mode for the file based on the input parameter
    if (strcmp(mode, "r") == 0) {
        file->mode = MODE_READ;
    } else if (strcmp(mode, "w") == 0) {
        file->mode = MODE_WRITE;
    } else if (strcmp(mode, "a") == 0) {
        file->mode = MODE_APPEND;
    }
    // The earlier else block remains to handle unsupported modes

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
    // Validate the input parameters to ensure they are not NULL and the request is valid
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid input parameters for fs_read.\n");
        return -1;
    }

    // Ensure the file is opened in a mode that allows reading
    if (file->mode != MODE_READ && file->mode != MODE_APPEND) { // Append mode may also allow reading
        printf("Error: File is not in a readable mode.\n");
        return -1;
    }

    // Calculate the actual number of bytes to read
    // It should not exceed the size of the file from the current position
    int bytesToRead = size;
    if (file->position + size > file->entry->size) {
        bytesToRead = file->entry->size - file->position;
    }

    // If there's nothing to read, return 0
    if (bytesToRead <= 0) {
        return 0;
    }

    // Perform the actual read operation
    memcpy(buffer, file->entry->data + file->position, bytesToRead);

    // Update the current position in the file
    file->position += bytesToRead;

    // Return the number of bytes read
    return bytesToRead;
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
    // Validate the input parameters
    if (file == NULL || buffer == NULL) {
        printf("Error: Invalid file or buffer pointer.\n");
        return -1;
    }

    // Check if the file is in write mode
    if (file->mode != MODE_WRITE) {
        printf("Error: File is not in write mode.\n");
        return -1;
    }

    // Calculate the write position and ensure it doesn't exceed the file system limits
    uint32_t writePosition = file->entry->size;
    if (writePosition + size > MAX_FILE_SIZE) { // Assuming MAX_FILE_SIZE is defined as the max file size
        printf("Error: Write exceeds maximum file size.\n");
        return -1;
    }

    // Perform the write operation to the simulated flash memory
    // For a real implementation, you would call flash_write_safe from flash_ops.h
    // This example directly manipulates the simulated flash memory for illustration
    memcpy((void *)(file->entry->data + writePosition), buffer, size);

    // Update the file's size if the write operation extends it
    if (writePosition + size > file->entry->size) {
        file->entry->size = writePosition + size;
    }

    // Update the file position
    file->position += size;

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


