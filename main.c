#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
#include "Directories.h"
int main() {
    stdio_init_all();
    sleep_ms(2000);
    

    for (int i = 15; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    sleep_ms(1000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);


      // Define paths for the directory and the file
    const char* dirPath = "/myDirectory";
    const char* filePath = "/myDirectory/myFile.txt";
    const char* content = "Hello, Raspberry Pi Pico!";

    // Create a directory
    printf("Creating directory '%s'...\n", dirPath);
    if (!fs_create_directory(dirPath)) {
        printf("Failed to create directory '%s'.\n", dirPath);
        fflush(stdout);
        return -1;
    }

    // List all directories and files
    printf("Listing all directories and files...\n");
    fs_list_all();

    printf("FINISH Creating directory '%s'...\n", dirPath);
    fflush(stdout);
    printf("Start opening the file path\n");    
    // Create and open a new file in write mode
    FS_FILE* file = fs_open(filePath, "w");
    if (file == NULL) {
        printf("Failed to create file '%s'.\n", filePath);
        fflush(stdout);
        return -1;
    }

     printf("FINISH opening the file path\n");  
    sleep_ms(1000);
    // Write some content to the file
    printf("Writing to file '%s'...\n", filePath);
    fflush(stdout);
    if (fs_write(file, content, strlen(content)) < 0) {
        printf("Failed to write to file '%s'.\n", filePath);
        fflush(stdout);
        fs_close(file);
        return -1;
    }
    printf("FINISH  Writing to file '%s'...\n", filePath);
    fflush(stdout);
    // Close the file after writing
    fs_close(file);
    printf("open file '%s'...\n", filePath);
    fflush(stdout);
    // Reopen the file in read mode
    file = fs_open(filePath, "r");
    if (file == NULL) {
        printf("Failed to open file '%s' for reading.\n", filePath);
        fflush(stdout);
        return -1;
    }
    sleep_ms(1000);

    // Read back the content
    char buffer[128] = {0};
    printf("Reading from file '%s'...\n", filePath);
    fflush(stdout);
    if (fs_read(file, buffer, sizeof(buffer)) < 0) {
        printf("Failed to read from file '%s'.\n", filePath);
        fflush(stdout);
        fs_close(file);
        return -1;
    }
    sleep_ms(1000);

    // Output the read content
    printf("Content read from file: %s\n", buffer);
    fflush(stdout);

    // Close the file
    fs_close(file);
    sleep_ms(1000);
    // Delete the file
    printf("Deleting file '%s'...\n", filePath);
    fflush(stdout);
    if (fs_rm(filePath) != 0) {
        printf("Failed to delete file '%s'.\n", filePath);
        fflush(stdout);
    }
    sleep_ms(1000);
    // Delete the directory
    printf("Deleting directory '%s'...\n", dirPath);
    fflush(stdout);
    if (fs_remove_directory(dirPath) != 0) {
        printf("Failed to delete directory '%s'.\n", dirPath);
        fflush(stdout);
    }

    
    printf("Operation Completed.\n\n\n");
    fflush(stdout);
    
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}