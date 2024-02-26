#include "filesystem.h"
#include <stdio.h>


/**
 * Opens a file with the specified path and mode.
 *
 * @param path The path of the file to be opened.
 * @param mode The mode in which the file should be opened.
 * @return A pointer to the opened file, or NULL if an error occurred.
 */
FS_FILE* fs_open(const char* path, const char* mode) {
    // TODO: Implement fs_open
    return NULL;
}

/**
 * Closes the specified file.
 *
 * @param file A pointer to the file to be closed.
 */
void fs_close(FS_FILE* file) {
    // TODO: Implement fs_close
}

/**
 * Reads data from the specified file into the provided buffer.
 *
 * @param file   A pointer to the file from which to read.
 * @param buffer A pointer to the buffer where the read data will be stored.
 * @param size   The maximum number of bytes to read.
 * @return The number of bytes read, or -1 if an error occurred.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    // TODO: Implement fs_read
    return -1;
}

/**
 * Writes data from the provided buffer to the specified file.
 *
 * @param file   A pointer to the file to which to write.
 * @param buffer A pointer to the buffer containing the data to be written.
 * @param size   The number of bytes to write.
 * @return The number of bytes written, or -1 if an error occurred.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    // TODO: Implement fs_write
    return -1;
}

/**
 * Sets the file position indicator for the specified file.
 *
 * @param file   A pointer to the file to seek in.
 * @param offset The number of bytes to offset from the specified position.
 * @param whence The position from which to calculate the offset.
 * @return 0 if successful, or -1 if an error occurred.
 */
int fs_seek(FS_FILE* file, long offset, int whence) {
    // TODO: Implement fs_seek
    return -1;
}
