
#include <stdio.h>
#include "pico/stdlib.h"
#include "../filesystem/filesystem.h"  
 #include "../tests/filesystem_test.h" 
#include "../tests/fat_fs_test.h"
#include "../tests/filesystem_helper_test.h"


int main() {
    stdio_init_all();
    sleep_ms(2000);
    
        while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    for (int i = 8; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    
    printf("\nFilesystem Test Start\n");
    fflush(stdout);
    
    sleep_ms(1000);
    
     printf("Testing write, read, and erase cycle...\n");
    
   
    //run_all_tests_filesystem();
    run_all_tests_FAT();
    run_all_tests_filesystem_Helper();


    printf("File closed after reading.\n");
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 







 
 