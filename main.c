#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
 
 
// Prototype for the fs_init function if it's not declared in filesystem.h
void fs_init(void);

int main() {
    // Initialize the filesystem
    fs_init();

    // Test opening a new file for writing
    FS_FILE* file = fs_open("testfile.txt", "w");
    if (file == NULL) {
        printf("Failed to open file for writing.\n");
        return -1;
    }
    printf("Opened 'testfile.txt' for writing.\n");

    // Test writing data to the file
    const char* data = "Hello, filesystem!";
    int bytesWritten = fs_write(file, data, strlen(data));
    if (bytesWritten < 0) {
        printf("Failed to write data to file.\n");
        fs_close(file);
        return -1;
    }
    printf("Wrote %d bytes to 'testfile.txt'.\n", bytesWritten);

    // Test seeking to the beginning of the file
    if (fs_seek(file, 0, SEEK_SET) != 0) {
        printf("Failed to seek to the beginning of the file.\n");
        fs_close(file);
        return -1;
    }
    printf("Seeked to the beginning of 'testfile.txt'.\n");

    // Test reading back the data
    char buffer[64] = {0};
    int bytesRead = fs_read(file, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        printf("Failed to read data from file.\n");
        fs_close(file);
        return -1;
    }
    printf("Read %d bytes from 'testfile.txt': %s\n", bytesRead, buffer);

    // Test closing the file
    fs_close(file);
    printf("Closed 'testfile.txt'.\n");

    return 0;
}
