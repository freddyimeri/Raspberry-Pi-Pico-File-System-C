
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

   // Create a directory for testing
    printf("Creating directory '/newfolder'\n");
    if (fs_create_directory("/newfolder")) {
        printf("Directory '/newfolder' created successfully.\n");
    } else {
        printf("Failed to create directory '/newfolder'.\n");
        return 1;  // Stop execution if directory creation fails
    }
    printf("\n\n------------------------------------\n");
    printf("showing all directory entries\n");
    DIR_all_directory_entries();
    printf("\n------------------------------------\n");


printf("\n\n------------------------------------\n");
    printf("showing allDIR_all_directory_entriesEX  directory entries\n");
    DIR_all_directory_entriesEX();
    printf("\n------------------------------------\n");



    // Create a few test files using fs_open
    printf("Creating test files...\n");
    FS_FILE* file1 = fs_open("/testfile1.txt", "w");
    if (file1) {
        printf("Test file '/testfile1.txt' created successfully.\n");
    } else {
        printf("Failed to create test file '/testfile1.txt'.\n");
    }





    // // Write some data to the files
    // const char* data1 = "Hello, World!";

    // if (file1) {
    //     fs_write(file1, data1, strlen(data1));
    //     fs_close(file1);
    // }
    // Display the initial filesystem state
    printf("Initial filesystem state:\n");
    fs_all_files_entries();

    printf("\n\n------------------------------------\n");
    printf("start-copying\n");
    // Test copying a file
    printf("Copying '/testfile1.txt' to '/newfolder/testfile1.txt'\n");
    int copyResult = fs_cp("/testfile1.txt", "/newfolder/testfile1.txt");
    if (copyResult == 0) {
        printf("File copied successfully.\n");
    } else {
        printf("Failed to copy file.\n");
    }

    // Display the final filesystem state
    printf("Final filesystem state:\n");
    fs_all_files_entries();

    printf("Program Finished\n");
    printf("------------------------------------\n");   
    fs_all_files_entrieszzzz();
    printf("------------------------------------\n");





    
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 







 