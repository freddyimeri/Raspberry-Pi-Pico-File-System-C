#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);

    for (int i = 15; i >= 1; i--) {
        printf("Operation starts in: %d\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    sleep_ms(2000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);

    fs_init();
    printf("Filesystem initialized.\n");
    fflush(stdout);

    FS_FILE* myFile = fs_open("test.txt", "w");
    if (myFile == NULL) {
        printf("Failed to open 'test.txt' for writing.\n");
        fflush(stdout);
        return -1;
    }
    printf("'test.txt' opened for writing.\n");
    fflush(stdout);
    char data[] = "Hello, Pico filesystem!";
    int bytesWritten = fs_write(myFile, data, sizeof(data));
    if (bytesWritten < 0) {
        printf("Failed to write data to 'test.txt'.\n");
        fflush(stdout);
        fs_close(myFile);
        return -1;
    }
    printf("Written %d bytes to 'test.txt'.\n", bytesWritten);
    fflush(stdout);

    fs_close(myFile);
    printf("'test.txt' closed after writing.\n");
    fflush(stdout);
    myFile = fs_open("test.txt", "r");
    if (myFile == NULL) {
        printf("Failed to open 'test.txt' for reading.\n");
        fflush(stdout);
        return -1;
    }
    printf("'test.txt' opened for reading.\n");
    fflush(stdout);

    char buffer[129]; // Increased size for null-terminator
    memset(buffer, 0, sizeof(buffer)); // Ensure buffer is zeroed out
    int bytesRead = fs_read(myFile, buffer, sizeof(buffer) - 1); // Leave space for null-terminator
    if (bytesRead < 0) {
        printf("Failed to read data from 'test.txt'.\n");
        fflush(stdout);
        fs_close(myFile);
        return -1;
    }
    sleep_ms(1000);
    printf("Read %d bytes from 'test.txt': %s\n", bytesRead, buffer);
    fflush(stdout);
    sleep_ms(1000);
    fs_close(myFile);
    sleep_ms(1000);
    printf("'test.txt' closed after reading.\n");
    fflush(stdout);
    printf("Filesystem Test Complete\n");
    fflush(stdout);
    printf("BYE\n");
    fflush(stdout);
    return 0;
}
