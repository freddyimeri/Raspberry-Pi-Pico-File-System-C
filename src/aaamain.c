
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
    
    for (int i = 1; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    char* testPaths[] = {
        "/home/user/documents/file.txt",
        "/documents/file.txt",
        "fileonly.txt",
        "/fileatroot.txt",
        "/",
        "/home/user/",
        NULL
    };

    for (int i = 0; testPaths[i] != NULL; i++) {
        printf("\nTesting path: %s\n", testPaths[i]);
        PathParts result = extract_last_two_parts(testPaths[i]);
        printf("Result - Directory: '%s', Filename: '%s'\n", result.directory, result.filename);
    }

    
    for (int i = 5; i >= 1; i--) {
        printf("Program Finishes in : %d Seconds\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }

    return 0;
}




 







 