
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
 #include <stdlib.h>
#include "../filesystem/filesystem.h"  
#include "../filesystem/filesystem_helper.h"  

#include "../directory/directories.h"
#include "../directory/directory_helpers.h"



int main() {
    stdio_init_all();
    sleep_ms(2000);
    
        while (!stdio_usb_connected()) {
        sleep_ms(100);
    }


    
    printf("\nFilesystem Test Start\n");
    fflush(stdout);
    fs_init();
    sleep_ms(1000);
    
    for (int i = 8; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

   printf("Starting Filesystem Test...\n");

  // Open a file for writing
    FS_FILE* file = fs_open("/root/testfile.txt", "w");
    if (!file) {
        printf("Failed to open file for writing.\n");
        return -1;
    }
    printf("File opened for writing.\n");

    // Write initial data to the file
    const char* data = "Hello, Pi Pico!";
    if (fs_write(file, data, strlen(data)) < 0) {
        printf("Failed to write to file.\n");
        fs_close(file);
        return -1;
    }
    printf("Data written to file: %s\n", data);

    // Close the file
    fs_close(file);
    printf("File closed after writing.\n");

    // Open the file for appending
    file = fs_open("/root/testfile.txt", "a");
    if (!file) {
        printf("Failed to open file for appending.\n");
        return -1;
    }

    // Seek to the end of the file before appending
    if (fs_seek(file, 0, SEEK_END) != 0) {
        printf("Failed to seek to end of file.\n");
        fs_close(file);
        return -1;
    }

    // Append data
    const char* moreData = " Welcome to the RP2040!";
    if (fs_write(file, moreData, strlen(moreData)) < 0) {
        printf("Failed to append to file.\n");
        fs_close(file);
        return -1;
    }
    printf("Appended data: %s\n", moreData);

    // Close the file
    fs_close(file);
    printf("File closed after appending.\n");

    // Open the file again to read
    file = fs_open("/root/testfile.txt", "r");
    if (!file) {
        printf("Failed to open file for reading.\n");
        return -1;
    }

    // Seek to the beginning of the file to start reading
    if (fs_seek(file, 0, SEEK_SET) != 0) {
        printf("Failed to seek to start of file.\n");
        fs_close(file);
        return -1;
    }

    // Read the content of the file
    char buffer[128];
    int bytesRead = fs_read(file, buffer, sizeof(buffer)-1);
    if (bytesRead < 0) {
        printf("Failed to read from file.\n");
        fs_close(file);
        return -1;
    }

    buffer[bytesRead] = '\0'; // Ensure the buffer is null-terminated
    printf("Read data: %s\n", buffer);

    // Close the file
    fs_close(file);
    printf("File closed after reading.\n");
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 







 