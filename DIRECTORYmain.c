#include "filesystem.h"
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
#include "directories.h"
#include "fat_fs.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected\n");
    

    for (int i = 8; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    sleep_ms(1000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);
    
    // // Open a file for writing
    // FS_FILE* file = fs_open("/testfile.txt", "w");
    // if (file == NULL) {
    //     printf("Failed to open file for writing.\n");
    //     fflush(stdout);
    //     return -1;
    // }
    
    // for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
    //     if (FAT[i] == FAT_ENTRY_END) {
    //         printf("FAT[%d]: %d\n", i, FAT[i]);
    //         sleep_ms(100);
    //     }
 
    // }
    // printf(" fat length: %d\n", sizeof(FAT));


    const char* dirPath = "/testDirectory";
    // Attempt to create the directory
    printf("Attempting to create directory: %s\n", dirPath);
    fflush(stdout);
    if (fs_create_directory(dirPath)) {
        printf("Directory '%s' successfully created.\n", dirPath);
        fflush(stdout);
    } else {
        printf("Failed to create directory '%s'.\n", dirPath);
        fflush(stdout);
        return -1;
    }

    sleep_ms(2000);

    printf("\nListing THE DIRECTORY CONTENTS\n");
    fflush(stdout);

        // Call the search function
    DirectoryEntry* entry = DIR_find_directory_entry(dirPath);
    sleep_ms(200);

    printf("finish of entry DIR_find_directory_entry\n");
    // Check if the directory entry was found
    if (entry != NULL) {
        printf("Directory found: %s\n", entry->name);
        printf("Start block: %u\n", entry->start_block);
        printf("Size/Number of entries: %u\n", entry->size);
        printf("Is directory: %s\n", entry->is_directory ? "Yes" : "No");
    } else {
        printf("Directory '%s' not found.\n", dirPath);
    }
    printf("finish of find_directory_entry\n\n");
 
    printf("\n");




    // // Attempt to delete the directory
    // printf("Attempting to delete directory: %s\n", dirPath);
    // fflush(stdout);
    // if (Dir_remove(dirPath) == 0) {
    //     printf("Directory '%s' successfully removed.\n", dirPath);
    //     fflush(stdout);
    // } else {
    //     printf("Failed to remove directory '%s'.\n", dirPath);
    //     fflush(stdout);
    // }
 

    printf("Attempting to remove directory '%s'...\n", dirPath);
    bool removed = Dir_remove(dirPath);
    if (removed) {
        printf("Directory '%s' was successfully removed.\n", dirPath);
    } else {
        printf("Failed to remove directory '%s'.\n", dirPath);
    }




    fflush(stdout); 
    printf("Operation Completed.\n\n\n");
    fflush(stdout);
    sleep_ms(1000);
    for (int i = 2; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
  
    printf("\nListing THE DIRECTORY CONTENTS\n");
    fflush(stdout);

        // Call the search function
    DirectoryEntry* entry1 = DIR_find_directory_entry(dirPath);
    sleep_ms(200);
    printf("finish of entry DIR_find_directory_entry\n");
    // Check if the directory entry was found
    if (entry1 != NULL) {
        printf("Directory found: %s\n", entry1->name);
        printf("Start block: %u\n", entry1->start_block);
        printf("Size/Number of entries: %u\n", entry1->size);
        printf("Is directory: %s\n", entry1->is_directory ? "Yes" : "No");
    } else {
        printf("Directory '%s' not found.\n", dirPath);
    }
    printf("finish of find_directory_entry\n\n");


    return 0;
}

