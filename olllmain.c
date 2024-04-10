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
    printf("\nDirectory Test Start\n");
    fflush(stdout);

    // Define the directory path
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

    // Attempt to delete the directory
    printf("Attempting to delete directory: %s\n", dirPath);
    fflush(stdout);
    if (fs_remove_directory(dirPath) == 0) {
        printf("Directory '%s' successfully removed.\n", dirPath);
        fflush(stdout);
    } else {
        printf("Failed to remove directory '%s'.\n", dirPath);
        fflush(stdout);
    }

    printf("Directory Test Completed.\n");
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


