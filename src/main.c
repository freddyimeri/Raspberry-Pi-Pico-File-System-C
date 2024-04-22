
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
    
    for (int i = 5; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    sleep_ms(1000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);


      // Define paths for the directory and the file
     const char* filePath = "myFile.txt";
    const char* content = "Hello, Raspberry Pi Pico!";
    // this is for append 
     const char* content2 = " hhhhhhhhhhhhhhhhh";

     char directoryPath[256]; // Buffer for directory part
    char fileName[256];      // Buffer for file name part


 
    // Try to split the path and handle file creation
    FS_FILE* file = split_path(filePath, directoryPath, fileName);


    if (file != NULL) {
        printf("File '%s' is opened CREATED successfully!\n", fileName);

    }

     printf("FINISH opening the file path\n");  
    sleep_ms(1000);
    // Write some content to the file
    printf("Writing to file '%s'...\n", filePath);
    fflush(stdout);

 
    int s = fs_write(file, content, strlen(content));
    printf("s: %d\n", s);

    fs_close(file);
   
    printf("FINISH  Writing to file '%s'...\n", filePath);
     fflush(stdout);


    FS_FILE* fileHandle = fs_open(filePath, "a");
  

    if (fileHandle) {
        printf("File '%s' opened successfully.\n", filePath);
    } else {
        printf("Failed to open file '%s'.\n", filePath);
    }
    sleep_ms(1000);
    int p = fs_write(fileHandle, content2, strlen(content2));
    printf("s: %d\n", p);

    fs_close(fileHandle);

    
    //////////////////////////////////////////////////////
    FS_FILE* fileHandle1 = fs_open(filePath, "r");


    char read_buffer[256];  // Buffer to hold read data
    memset(read_buffer, 0, sizeof(read_buffer));  // Clear the buffer

    // Read data from the file
    int read_bytes = fs_read(fileHandle1, read_buffer, sizeof(read_buffer));
    if (read_bytes > 0) {
        printf("Successfully read %d bytes: %s\n", read_bytes, read_buffer);
    } else {
        printf("Failed to read from file or no data left.\n");
    }




    // // Read back the content
    // char buffer[128] = {0};
    // printf("Reading from file '%s'...\n", filePath);
    // fflush(stdout);
    // if (fs_read(file, buffer, sizeof(buffer)) < 0) {
    //     printf("Failed to read from file '%s'.\n", filePath);
    //     fflush(stdout);
    //     fs_close(file);
    //     return -1;
    // }
    // sleep_ms(1000);

    // // Output the read content
    // printf("Content read from file: %s\n", buffer);
    // fflush(stdout);

    // // Close the file
    // fs_close(file);
    // sleep_ms(1000);
    // // Delete the file
    // printf("Deleting file '%s'...\n", filePath);
    // fflush(stdout);
    // if (fs_rm(filePath) != 0) {
    //     printf("Failed to delete file '%s'.\n", filePath);
    //     fflush(stdout);
    // }
    // sleep_ms(1000);
    // DIR_all_directory_entries();
    // printf("FINISH LISTING ALL DIRECTORIES\n");
    printf("\n\nSTART LISTING ALL FILES\n");
    FileEntry* entryTEST = FILE_find_file_entry("myFile.txt");
    printf("\nFINISH LISTING ALL FILES\n");
    printf("File Name: %s\n", entryTEST->filename);
    printf("File Size: %d\n", entryTEST->size);
    printf("File Start Block: %d\n", entryTEST->start_block);
    printf("File Parent Directory ID: %d\n", entryTEST->parentDirId);
    printf("File In Use: %d\n", entryTEST->in_use);
    printf("File is Directory: %d\n", entryTEST->is_directory);
    printf("File Buffer: %s\n", entryTEST->buffer);
    printf("\n\n");
    fflush(stdout);
    

    
    printf("Operation Completed.\n\n\n");
    fflush(stdout);
    
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 