
#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
 
#include "../filesystem/filesystem.h"  
#include "../filesystem/filesystem_helper.h"  

#include "../directory/directories.h"

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

    printf("\n FIRST TEST 1\n\n");
    char path[] = "directory/AnotherDir/thirdDir/testfile.txt";
    PathParts parts = extract_last_two_parts(path);

    printf("Directory: %s\n", parts.directory); // Should print "thirdDir"
    printf("Filename: %s\n", parts.filename);   // Should print "testfile.txt"

    printf("\nNEXT TEST 2\n\n");

    char path1[] = "directory/AnotherDir/thirdDir/thirdDir/testfile.txt";
    PathParts parts1 = extract_last_two_parts(path1);

    printf("Directory: %s\n", parts1.directory); // Should print "thirdDir"
    printf("Filename: %s\n", parts1.filename);   // Should print "testfile.txt"
  
  
     printf("\nNEXT TEST 3\n\n");

    char path2[] = "directory/AnotherDir/thirdDir/thirdDir/testfile.txt";
    PathParts parts2 = extract_last_two_parts(path2);

    printf("Directory: %s\n", parts2.directory); // Should print "thirdDir"
    printf("Filename: %s\n", parts2.filename);   // Should print "testfile.txt"

    printf("\nNEXT TEST 4\n\n");
  
    char path3[] = "thirdDir/testfile.txt";
    PathParts parts3 = extract_last_two_parts(path3);
    printf("Directory: %s\n", parts3.directory); // Should print "thirdDir"
    printf("Filename: %s\n", parts3.filename);   // Should print "testfile.txt"

    printf("\nNEXT TEST 5\n\n");

    char path4[] = "testfile.txt";
    PathParts parts4 = extract_last_two_parts(path4);

    printf("Directory: %s\n", parts4.directory); // Should print "thirdDir"
    printf("Filename: %s\n", parts4.filename);   // Should print "testfile.txt"
    // char path1[] = "directory/AnotherDir/testfile.txt"; // Valid nested directories ending in a file
    // char path2[] = "directory/testfile.txt/AnotherDir"; // Invalid: file followed by directory

    // char dir[256];
    // char file[256];

    // printf("Testing valid path:\n");
    // int result1 = split_path(path1, dir, file);
    // if (result1 != -1) {
    //     printf("Valid Path: Directory = '%s', File = '%s'\n", dir, file);
    // }

    // printf("\nTesting invalid path:\n");
    // int result2 = split_path(path2, dir, file);
    // if (result2 == -1) {
    //     printf("Error processing path: %s\n", path2);
    // }

    for (int i = 3; i >= 1; i--) {
        printf("Operation FINISH in: %d\n", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    return 0;
}




