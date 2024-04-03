#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
 
 

int main() {
    // Initialize stdio for output
    stdio_init_all();
    sleep_ms(2000);
    int i;

    for(i = 20; i >= 1; i--) {
        printf("Operation starts in: %d\n", i);
        sleep_ms(1200);
    }

    
 
    // Wait a bit for stdio to initialize
    sleep_ms(2000);
    printf("\nFilesystem Test Start\n");

    // Initialize the filesystem
    fs_init();
    printf("Filesystem initialized.\n");

    // Opening a file for writing
    FS_FILE* myFile = fs_open("test.txt", "w");
    if (myFile == NULL) {
        printf("Failed to open 'test.txt' for writing.\n");
        return -1;
    }
    printf("'test.txt' opened for writing.\n");

    // Writing data to the file
    char data[] = "Hello, Pico filesystem!";
    int bytesWritten = fs_write(myFile, data, sizeof(data));
    if (bytesWritten < 0) {
        printf("Failed to write data to 'test.txt'.\n");
        fs_close(myFile);
        return -1;
    }
    printf("Written %d bytes to 'test.txt'.\n", bytesWritten);

    // Closing the file
    fs_close(myFile);
    printf("'test.txt' closed after writing.\n");

    // Re-opening the file for reading
    myFile = fs_open("test.txt", "r");
    if (myFile == NULL) {
        printf("Failed to open 'test.txt' for reading.\n");
        return -1;
    }
    printf("'test.txt' opened for reading.\n");

    // Reading data back from the file
    char buffer[128];
    int bytesRead = fs_read(myFile, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        printf("Failed to read data from 'test.txt'.\n");
        fs_close(myFile);
        return -1;
    }
    printf("Read %d bytes from 'test.txt': %s\n", bytesRead, buffer);

    // Closing the file
    fs_close(myFile);
    printf("'test.txt' closed after reading.\n");

    printf("Filesystem Test Complete\n");

    return 0;
}
