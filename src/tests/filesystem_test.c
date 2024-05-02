


#include "../filesystem/filesystem.h"  
#include "../tests/filesystem_test.h" 
#include <string.h>
#include "../directory/directories.h"


void run_all_tests_filesystem() {
    char slashes[] = "\n/////////////////////////////////////////////\n";
   
    printf("%s", slashes);
    test_fs_write_and_read();
    printf("%s", slashes);
    test_fs_append_to_file(); 
    printf("%s", slashes);
    test_fs_large_file_handling();
    printf("%s", slashes);
    test_fs_file_seek();
    printf("%s", slashes);
    test_fs_open_nonexistent_file();
    printf("%s", slashes);
    test_fs_cp_function();
    printf("%s", slashes);
    test_fs_rm();
}




void test_fs_write_and_read(void) {
    fs_init();
    FS_FILE *file = fs_open("/root/testfile.txt", "w");
    char *data = "Hello, Pi Pico";
    int written = fs_write(file, data, strlen(data));
    printf("Write Test - Expected: %d, Actual: %d\n", strlen(data), written);

    fs_close(file);
    file = fs_open("/root/testfile.txt", "r");
    char buffer[100];
    int read = fs_read(file, buffer, strlen(data));
    buffer[read] = '\0';  // Null terminate the string read

    printf("Read Test - Expected: %s, Actual: %s\n", data, buffer);
    fs_close(file);
}



 
void test_fs_append_to_file(void) {
    // Step 1: Create a file and write initial data
    FS_FILE *file = fs_open("/root/appendTest.txt", "w");
    if (file == NULL) {
        printf("Append Test - Failed to open file for writing.\n");
        return;
    }
    char *initialData = "Hello, ";
    int bytesWritten = fs_write(file, initialData, strlen(initialData));
    printf("Append Test - Initial write: %d bytes.\n", bytesWritten);
    fs_close(file);

    // Step 2: Open the file in append mode and write more data
    file = fs_open("/root/appendTest.txt", "a");
    if (file == NULL) {
        printf("Append Test - Failed to open file for appending.\n");
        return;
    }
    char *additionalData = "Pi Pico!";
    bytesWritten = fs_write(file, additionalData, strlen(additionalData));
    printf("Append Test - Appended write: %d bytes.\n", bytesWritten);
    fs_close(file);

    // Step 3: Read back the entire file to verify the contents
    file = fs_open("/root/appendTest.txt", "r");
    if (file == NULL) {
        printf("Append Test - Failed to open file for reading.\n");
        return;
    }
    char readBuffer[100];  // Adjust buffer size based on expected data length
    int bytesRead = fs_read(file, readBuffer, sizeof(readBuffer) - 1);
    readBuffer[bytesRead] = '\0'; // Null-terminate the string read
    printf("Append Test - Read back: '%s'\n", readBuffer);
    fs_close(file);
}


void test_fs_large_file_handling(void) {
    FS_FILE *file = fs_open("/root/largeFileTest.txt", "w");
    if (!file) {
        printf("Large File Handling Test - Failed to create file.\n");
        return;
    }

    // Create a large buffer close to the file system block size
    char data[FILESYSTEM_BLOCK_SIZE];
    memset(data, 'A', sizeof(data)); // Fill with 'A'

    int bytesWritten = fs_write(file, data, sizeof(data));
    printf("Large File Handling Test - Bytes written: %d\n", bytesWritten);
    fs_close(file);

    
}


void test_fs_file_seek(void) {
    FS_FILE *file = fs_open("/root/seekTest.txt", "w");
    char *data = "1234567890"; // 10 bytes
    fs_write(file, data, strlen(data));
    fs_close(file);

    file = fs_open("/root/seekTest.txt", "r");

    // Seek to the middle of the file
    fs_seek(file, 5, SEEK_SET);
    char buffer[6]; // Read the second half
    fs_read(file, buffer, 5);
    buffer[5] = '\0';
    printf("File Seek Test (Middle) - Expected: '67890', Actual: '%s'\n", buffer);

    // Seek back to the beginning
    fs_seek(file, 0, SEEK_SET);
    fs_read(file, buffer, 5);
    buffer[5] = '\0';
    printf("File Seek Test (Start) - Expected: '12345', Actual: '%s'\n", buffer);

    // Seek to the end and try to read (should read 0 bytes)
    fs_seek(file, 0, SEEK_END);
    int bytesRead = fs_read(file, buffer, 5);
    printf("File Seek Test (End) - Bytes read: %d\n", bytesRead);

    fs_close(file);
}


void test_fs_open_nonexistent_file(void) {
    FS_FILE *file = fs_open("/root/nonexistent.txt", "r");
    if (file == NULL) {
        printf("Open Non-existent File Test - Passed\n");
    } else {
        printf("Open Non-existent File Test - Failed\n");
        fs_close(file);
    }
}

void test_fs_cp_function(void) {
    

    // Step 2: Create a source file and write data
    FS_FILE *srcFile = fs_open("/root/sourceFile.txt", "w");
    if (!srcFile) {
        printf("Copy Test - Failed to open source file for writing.\n");
        return;
    }
    char *data = "Hello, Pi Pico!";
    int writeResult = fs_write(srcFile, data, strlen(data));
    if (writeResult < 0) {
        printf("Copy Test - Failed to write to source file.\n");
        fs_close(srcFile);
        return;
    }
    fs_close(srcFile);

    // Step 3: Copy the file to the new directory
    int cpResult = fs_cp("/root/sourceFile.txt", "/root");
    if (cpResult != 0) {
        printf("Copy Test - Failed to copy file to directory '%s'.\n", "/root");
        return;
    }

    // Step 4: Verify the copied file in the new directory
    char copiedFilePath[128];
    snprintf(copiedFilePath, sizeof(copiedFilePath), "%s/%s", "/root", "sourceFile.txt"); // Constructing the path to check
    FS_FILE *copiedFile = fs_open(copiedFilePath, "r");
    if (!copiedFile) {
        printf("Copy Test - Failed to open copied file for reading at '%s'.\n", copiedFilePath);
        return;
    }
    char readBuffer[100];
    int bytesRead = fs_read(copiedFile, readBuffer, sizeof(readBuffer) - 1);
    if (bytesRead < 0) {
        printf("Copy Test - Failed to read from copied file.\n");
        fs_close(copiedFile);
        return;
    }
    readBuffer[bytesRead] = '\0'; // Null-terminate the read data

    // Check if the data matches
    if (strcmp(data, readBuffer) == 0) {
        printf("Copy Test - Passed, data matches.\n");
    } else {
        printf("Copy Test - Failed, data does not match.\n");
    }

    fs_close(copiedFile);
}



void test_fs_rm(void) {
    fs_init();  // Make sure the file system is initialized if not already done in main.

    // Test 1: Attempt to remove a file using a NULL path
    int result = fs_rm(NULL);
    printf("Test 1: Remove NULL path - Expected: -1, Actual: %d\n", result);

    // Test 2: Attempt to remove a non-existent file
    result = fs_rm("/nonexistentfile.txt");
    printf("Test 2: Remove non-existent file - Expected: -2, Actual: %d\n", result);

    // Setup: Create a directory to test directory removal prevention
    const char* dirPath = "/testDir";
    fs_create_directory(dirPath);  // Assuming this function has been implemented correctly
    result = fs_rm(dirPath);
    printf("Test 3: Attempt to remove directory - Expected: -3, Actual: %d\n", result);

    // Setup: Create a file to test successful removal
    FS_FILE *file = fs_open("/testFile.txt", "w");
    char *data = "Data to be removed";
    fs_write(file, data, strlen(data));
    fs_close(file);

    result = fs_rm("/testFile.txt");
    printf("Test 4: Remove existing file - Expected: 0, Actual: %d\n", result);

    // Verification: Attempt to reopen the removed file
    file = fs_open("/testFile.txt", "r");
    if (file == NULL) {
        printf("Verification: File '/testFile.txt' correctly removed\n");
    } else {
        printf("Verification: Failed to remove file '/testFile.txt'\n");
        fs_close(file);
    }
}


