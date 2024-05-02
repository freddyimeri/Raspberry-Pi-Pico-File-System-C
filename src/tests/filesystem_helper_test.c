#include "../filesystem/filesystem.h"  
#include "../filesystem/filesystem_helper.h"
#include "../tests/filesystem_helper_test.h"
#include <string.h>
#include "../directory/directories.h"


void run_all_tests_filesystem_Helper() {
    char slashes[] = "\n/////////////////////////////////////////////\n";

    printf("%s", slashes);
    test_extract_last_two_parts();
    printf("%s", slashes);
    test_file_system_operations();
    printf("%s", slashes);
    test_createFileEntry();
    printf("%s", slashes);
    test_generateUniqueId();
    printf("%s", slashes);
    test_save_and_load_FileEntries();
    printf("%s", slashes);



}




void test_extract_last_two_parts() {
    char* testPaths[] = {
        "/home/user/documents/file.txt",
        "/documents/file.txt",
        "fileonly.txt",
        "/fileatroot.txt",
        "/",
        "/home/user/",
        NULL
    };

    printf("Testing extract_last_two_parts function...\n");
    for (int i = 0; testPaths[i] != NULL; i++) {
        printf("\nTesting path: %s\n", testPaths[i]);
        PathParts result = extract_last_two_parts(testPaths[i]);
        printf("Result - Directory: '%s', Filename: '%s'\n", result.directory, result.filename);
    }
}



 
 
void test_file_system_operations() {
    printf("Starting File System Operations Tests...\n");

    // Test File Creation
    uint32_t parentDirId = 0;  // Assuming root directory ID is 0
    const char* filePath = "newfile.txt";
    FileEntry* fileEntry = createFileEntry(filePath, parentDirId);
    if (fileEntry != NULL) {
        printf("File Creation Test Passed - File created: %s\n", fileEntry->filename);
    } else {
        printf("File Creation Test Failed - File not created.\n");
    }

    // Test File Deletion
    int deleteResult = fs_rm("/newfile.txt");
    if (deleteResult == 0) {
        printf("File Deletion Test Passed - File deleted successfully.\n");
    } else {
        printf("File Deletion Test Failed - Error code: %d\n", deleteResult);
    }

    // Test Directory Handling
    const char* dirPath = "newDir";
    bool dirCreated = fs_create_directory(dirPath);
    if (dirCreated) {
        printf("Directory Creation Test Passed - Directory created: %s\n", dirPath);
    } else {
        printf("Directory Creation Test Failed - Directory not created.\n");
    }

    // Test Unique ID Generation
    uint32_t uniqueId = generateUniqueId();
    printf("Unique ID Test - Generated ID: %u\n", uniqueId);

    // Test Save and Load File System Entries
    saveFileEntriesToFileSystem();
    loadFileEntriesFromFileSystem();

    printf("Completed File System Operations Tests.\n");
}
 



void test_createFileEntry() {
    printf("Testing createFileEntry...\n");
    uint32_t parentDirId = 0;  // Assuming root directory ID is 0
    FileEntry* entry = createFileEntry("testFile.txt", parentDirId);
    if (entry != NULL && entry->in_use) {
        printf("Create File Entry Test Passed - File created with ID: %u\n", entry->unique_file_id);
    } else {
        printf("Create File Entry Test Failed\n");
    }
}


void test_generateUniqueId() {
    printf("Testing generateUniqueId...\n");
    uint32_t id = generateUniqueId();
    if (id >= 1000) {
        printf("Unique ID Generation Test Passed - Generated ID: %u\n", id);
    } else {
        printf("Unique ID Generation Test Failed - Invalid ID: %u\n", id);
    }
}

void test_save_and_load_FileEntries() {
    printf("Testing save and load File Entries...\n");
    createFileEntry("testSaveLoad.txt", 0);  // Set up: create a file to save
    saveFileEntriesToFileSystem();  // Save entries
    loadFileEntriesFromFileSystem();  // Load entries to test recovery
}
