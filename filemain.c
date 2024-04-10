#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);
    

    for (int i = 15; i >= 1; i--) {
        printf("Operation starts in: %d\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    sleep_ms(1000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);



    // Open a file for writing
    FS_FILE* file = fs_open("/testfile.txt", "w");
    if (file == NULL) {
        printf("Failed to open file for writing.\n");
        fflush(stdout);
        return -1;
    }


    fflush(stdout);
    sleep_ms(1000);
    printf("\nStart WRITING  \n");
    fflush(stdout);
    // Data to write to the file
    const char* data = "Hello, filesystem!";
    int dataSize = strlen(data) + 1; // Include the null terminator
    sleep_ms(1000);
    // Write data to the file
    int bytesWritten = fs_write(file, data, dataSize);
    if (bytesWritten != dataSize) {
        printf("Failed to write data to file.\n");
        fflush(stdout);
        fs_close(file); // Close the file before exiting
        return -1;
    }
    printf("FINISH WRITING  \n");
    fflush(stdout);
    sleep_ms(1000);
 
    // Close the file after writing
    fs_close(file);
    sleep_ms(1000);
    // Open the file again, this time for reading
    file = fs_open("/testfile.txt", "r");
    if (file == NULL) {
        printf("Failed to open file for reading.\n");
        fflush(stdout);
        return -1;
    }


    

    // Buffer to read the data back into
    char buffer[256];
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    
    
    // Read data from the file
    int bytesRead = fs_read(file, buffer, sizeof(buffer));
    if (bytesRead != dataSize) {
        printf("Failed to read data from file or read size mismatch.\n");
        fflush(stdout);
        fs_close(file); // Close the file before exiting
        return -1;
    }

    // Close the file after reading
    fs_close(file);
    sleep_ms(1000);
    // Print the read data to the console
    printf("Data read from file: %s\n", buffer);
    fflush(stdout);
 

    sleep_ms(1000);
    // Attempt to remove the file
    if (fs_rm("/testfile.txt") == 0) {
        printf("File successfully removed.\n");
        fflush(stdout);
    } else {
        printf("Failed to remove the file.\n");
        fflush(stdout);
    }

    printf("Finishhhh.\n");
    fflush(stdout);


    return 0;
}
