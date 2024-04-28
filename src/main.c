
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
 #include <stdlib.h>
#include "../filesystem/filesystem.h"  
#include "../filesystem/filesystem_helper.h"  
#include "../FAT/fat_fs.h"   
#include "../directory/directories.h"
#include "../directory/directory_helpers.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
 
void review_block(uint32_t first_block);



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

    // Open a file for writing
    FS_FILE* file = fs_open("/root/testfile.txt", "w");
    if (!file) {
        printf("Failed to open file for writing.\n");
        return -1;
    }
    printf("File opened for writing.\n");

    // Prepare data to write
    const char* baseData = "Hello, Pi Pico! This is a test of the FAT file system handling in embedded systems.";
    int baseDataLength = strlen(baseData);
    int dataSize = 18006;  // Size to demonstrate spanning multiple blocks

    char* data = malloc(dataSize + 1);  // +1 for null terminator
    if (!data) {
        printf("Failed to allocate memory for data.\n");
        fs_close(file);
        return -1;
    }

    // Fill the data buffer with repeated base data
    for (int i = 0; i < dataSize; i += baseDataLength) {
        strncpy(data + i, baseData, MIN(baseDataLength, dataSize - i));
    }
    data[dataSize] = '\0';  // Null-terminate the data

    // Write data to the file
    int bytesWritten = fs_write(file, data, dataSize);
    if (bytesWritten < 0) {
        printf("Failed to write to file.\n");
        free(data);
        fs_close(file);
        return -1;
    }
    printf("Data written to the file: %d bytes.\n", bytesWritten);

    // Close the file after writing
    fs_close(file);
    printf("File closed after writing.\n");
    /////////////////////////////////////
    printf("\n\n\nFile Entries :\n");
    fs_all_files_entries();

    printf("\n\n\nFile Entries :\n");

   
    file = fs_open("/root/testfile.txt", "r");
    if (!file) {
        printf("Failed to open file for reading.\n");
        free(data);
        return -1;
    }
    printf("File opened for reading.\n");


     ///////////////////////////////////
    printf("\n\nREVIEW BLOCKS\n");
    review_block(file->entry->start_block);
    printf("\nREVIEW BLOCKS\n\n\n\n");
    ///////////////////////////////
    ///////////////////////////
    // Re-open the file for reading



    // Buffer to read back the data
    char* readData = malloc(dataSize + 1);
    // int dataSizessssss = 2096;  // Size to demonstrate spanning multiple blocks
    // char* readData = malloc(dataSizessssss + 1);
    if (!readData) {
        printf("Failed to allocate memory for reading data.\n");
        free(data);
        fs_close(file);
        return -1;
    }

    // Read back the data
    int bytesRead = fs_read(file, readData, dataSize);
    // int bytesRead = fs_read(file, readData, dataSizessssss);

    if (bytesRead < 0) {
        printf("Failed to read from file.\n");
    } else {
        readData[bytesRead] = '\0';  // Null-terminate the read data
        printf("Data read from the file: %d bytes.\n", bytesRead);
        // Uncomment below to print the read data
        printf("Read data: %s\n", readData);
    }

    // Clean up
    free(data);
    free(readData);
    fs_close(file);
 
    printf("File closed after reading.\n");
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 







 




// Function to create a file with 'num_blocks' blocks
void review_block(uint32_t first_block) {
 
        // Print the structure of the file
    printf("File blocks:");
    uint32_t block = first_block;
    while (block != FAT_ENTRY_END) {
        printf(" %u ->", block);
        uint32_t next;
        if (fat_get_next_block(block, &next) != FAT_SUCCESS) {
            printf(" Error retrieving block.");
            break;
        }
        block = next;
    }
    printf(" END\n");
}



