
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
 
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
    

    for (int i = 15; i >= 1; i--) {
        printf("Operation starts in: %d\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    fs_init();
    sleep_ms(1000);
    printf("\nFilesystem Test Start\n");
    fflush(stdout);
    
  

 
 


    // char fullPath[] = "directory/AnotherDir/thirdDir/testfile.txt";
    // PathSegments segments = extract_path_segments(fullPath);

    // printf("Path has %zu segments:\n", segments.count);
    // for (size_t i = 0; i < segments.count; i++) {
    //     printf("Segment %zu: %s\n", i + 1, segments.segments[i]);
    // }
    
    // printf("\nFreeing path segments\n");
    // free_path_segments(&segments);
    // printf("Path segments freed\n");
  

    // printf("\nListing all directories\n\n");
    // DIR_all_directory_entries();
 
   
    // // char path[] = "directory/AnotherDir/"; // Valid nested directories ending in a file

 char path[] = "directory/AnotherDir/testfile.txt"; // Valid nested directories ending in a file
    char dir[256];
    char file[256];

    printf("Testing valid path:\n");
    int result1 = split_path(path, dir, file);
    if (result1 != -1) {
        printf("Valid Path: Directory = '%s', File = '%s'\n", dir, file);
    }

    printf("\n\nListing all directories\n\n");
    DIR_all_directory_entries();
    printf("\n\n");
    printf("\n");
    for (int i = 3; i >= 1; i--) {
        printf("Operation FINISH in: %d\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    return 0;
}




