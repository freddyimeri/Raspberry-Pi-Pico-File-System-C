 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/mutex.h"
#include <ctype.h>
 
#include "../config/flash_config.h"    
#include "../FAT/fat_fs.h"            
#include "../flash/flash_ops.h"       
#include "../filesystem/filesystem.h"  
#include "../directory/directories.h"
 #include "../filesystem/filesystem_helper.h"  

 
#include "../directory/directory_helpers.h"
#include "../filesystem/filesystem_helper.h"  


 int fs_cp(const char* source_path, const char* dest_path) {

    // parantID id for the file where is going to be saved 
    uint32_t fileparentid = 0;

    char source_path_directory_path[256]; // Buffer for directory part
    char source_path_fileName[256];      // Buffer for source_path file name part

    char dest_path_directory_path[256]; // Buffer for directory part
    char dest_path_fileName[256];      // Buffer for source_path file name part

    split_path_fs_copy(source_path, source_path_directory_path, source_path_fileName);
    split_path_fs_copy(dest_path, dest_path_directory_path, dest_path_fileName);

    if (source_path_fileName == NULL) {
        printf("Error: Source path '%s' does not contain a file name.\n", source_path);
        return -1;
    }


    int sourcePathExist = check_full_file_existance(source_path_fileName);
    if (sourcePathExist == -1) {
        printf("Error: Source path '%s' does not exist.\n", source_path);
        return -1;
    }



    if (dest_path_directory_path == NULL){
        dest_path_directory_path = "/root"
     }
    else{
        // split dest_path_directory_path into parts and take the last part,

        // take the last part and check if it exists
        const char *dest_last_dir = strrchr(dest_path_directory_path, '/');
        if (dest_last_dir != NULL) {
        printf("Last directory: %s\n", dest_last_dir + 1);
        // coppy the last part of the dest_last_dir to the dest_path_directory_path
        dest_path_directory_path = dest_last_dir + 1;// to do needs to be fixed 
        } else {
        // If no slash is found, the whole path is a single directory name
        printf("Last directory: %s\n", dest_path_directory_path);
         }
         
    }

        DirectoryEntry* dirEntry = DIR_find_directory_entry(dest_path_directory_path);
        if (dirEntry == NULL) {
            printf("Directory '%s' does not exist\n", dest_path_directory_path);
            free(dirEntry)
            return -1;
        } 

         fileparentid = dirEntry->currentDirId;


        // now we have the fileparentid with that we can procced and recover the file from the source path
        // and search though the FileEntry fileSystem[MAX_FILES]; to find out in which index is found, once is found, 
        // we will update the filesystem[index]-> parentDirId = fileparentid
        uint32_t numbs = find_file_entry_by_name(source_path_fileName);
        if (numbs == -1) {
            printf("Error: File '%s' not found for reading.\n", source_path_fileName);
            return -1;
        }
        ///////////////////////////////////////////////////////////
        // now we will check if the file name exists in the directory
        uint32_t parent_ID_souce_dir = fileSystem[numbs].parentDirId;
        if (parent_ID_souce_dir == fileparentid) {
            // add a number to the filename
            char* newFileName = malloc(strlen(source_path_fileName) + 2);
            sprintf(newFileName, "%s1", source_path_fileName);
            strcpy(fileSystem[numbs].filename, newFileName);
        }
        ///////////////////////////////////////////////
        fileSystem[numbs].parentDirId = fileparentid;
        
        
        return 1;

 }
 





int check_full_file_existance(const char* fullPath) {

    FileEntry* entry = FILE_find_file_entry(fullPath);

    if (entry == NULL) {
    printf("Error: File '%s' not found for reading.\n", fileName);
    return -1;
    }
    return 1;

} 




// checks if the file exists
void split_path_fs_copy(const char* fullPath, char* directoryPath, char* fileName){
    char directoryPath[256]; // Buffer for directory part
    char fileName[256];      // Buffer for file name part
    char tempPath[256];
    strcpy(tempPath, fullPath); // Make a copy of fullPath for manipulation

    char *lastSlash = strrchr(tempPath, '/');
    bool fileEncountered = false; // Flag to mark when a file has been encountered

    // Initialize the outputs
    directoryPath[0] = '\0';
    fileName[0] = '\0';

    if (lastSlash) {
        *lastSlash = '\0'; // Terminate the directory path
        strcpy(fileName, lastSlash + 1); // Copy the file name part
        strcpy(directoryPath, tempPath); // Copy the directory part
    } else {
        // No slashes found, it's a single segment
        strcpy(fileName, fullPath);
    }
    
    // Check each segment for a file pattern
    strcpy(tempPath, fullPath); // Reset tempPath
    char *segment = strtok(tempPath, "/");
    while (segment) {
        if (strrchr(segment, '.') != NULL) { // Check if the segment looks like a file
            fileEncountered = true;
        } else if (fileEncountered) { // A directory or another segment follows a file
            printf("Invalid path: '%s'. A file segment cannot be followed by another segment.\n", fullPath);
            
        }
        segment = strtok(NULL, "/");
    }

    // Determine if the final part is a file or directory
    bool finalPartIsFile = strrchr(fileName, '.') != NULL; 
    if (!finalPartIsFile) {
        fileName == NULL;
   
}





// checks if the file exists
void split_path_fs_copy(const char* fullPath, char* directoryPath, char* fileName){
 
    char tempPath[256];
    strcpy(tempPath, fullPath); // Make a copy of fullPath for manipulation

    char *lastSlash = strrchr(tempPath, '/');
    bool fileEncountered = false; // Flag to mark when a file has been encountered

    // Initialize the outputs
    directoryPath[0] = '\0';
    fileName[0] = '\0';

    if (lastSlash) {
        *lastSlash = '\0'; // Terminate the directory path
        strcpy(fileName, lastSlash + 1); // Copy the file name part
        strcpy(directoryPath, tempPath); // Copy the directory part
    } else {
        // No slashes found, it's a single segment
        strcpy(fileName, fullPath);
    }
    
    // Check each segment for a file pattern
    strcpy(tempPath, fullPath); // Reset tempPath
    char *segment = strtok(tempPath, "/");
    while (segment) {
        if (strrchr(segment, '.') != NULL) { // Check if the segment looks like a file
            fileEncountered = true;
        } else if (fileEncountered) { // A directory or another segment follows a file
            printf("Invalid path: '%s'. A file segment cannot be followed by another segment.\n", fullPath);
            
        }
        segment = strtok(NULL, "/");
    }

    // Determine if the final part is a file or directory
    bool finalPartIsFile = strrchr(fileName, '.') != NULL; 
    if (!finalPartIsFile) {
        fileName == NULL;
    }
   
}




// int fs_cp(const char* source_path, const char* dest_path) 
    // check if the source path exists
       // if it is empty then return 0(error message)
       // int a = check_path_existence(source_path)
       // if( a == 0):
            // printf("The source path does not exist")
            // return 0          


        // copy the index number of the FileEntry fileSystem[MAX_FILES];
        // now we know where the file is located in the ram memory

    
    // find out if the dest_path contains a directory  
    // directoryentry == directory fun
    //fileentry == file fun

    // if dest_path is only a file name, then it is in the root directory
    //if directoryentry is empty:
        // directoryentry = root directory
        // fileParentid = get directoryentry currentDirId 
        


    //else, it is in a subdirectory
        //  check if it exist the subdirectory
        // if it does get the currentDirId of the subdirectory
        //fileParentid = get directoryentry currentDirId 
    
    // now we have the fileparentid 

    // check if the file name exist in the directory
        // have a function that lists all the files in a directory
        //   if search_file_existence_in_dir returns 1, then the file exists
                // if exist then add on the filename a number
                   // filename = filename + 1

    // now we will create the file in the destination path


 




// function that lists all the files in a directory
// int search_file_existence_in_dir(const  uint32_t parentPath)
    // look for every entry of files where the parentDirId is equal to the parentPath
    // call FileEntry* list_all_files(size_t *count) 
    // loop thouhh the entries and find out if a file with same name exists, 
    // if it does return 1
     


// check existance of path 
// int check_path_existence(const char* path)
    // check if the path exists
    // split the in direcories and files 
    // directoryentry ==  return_directories(path)
    // fileentry == return_files(path)
    
    // check if the  directory exists
    //int dir = find_free_directory_entry(
        // if it does return 1
        //  

    // if it does return 1
    // if it does not return 0







int checkDirectoryExistance(const char* path) {
    printf("Attempting to check directory exiatance %s\n", path);

    PathSegments segments = extract_path_segments(path);
    if (segments.count == 0) {
        printf("Invalid directory path: %s\n", path);
        return -1;
    }

    char* parentPath = NULL;  // Start with no parent path
    char* currentPath = strdup(""); // Start with an empty path for appending

    for (size_t i = 0; i < segments.count; ++i) {
        // Construct the current path for checking/creation
        char* updatedPath = malloc(strlen(currentPath) + strlen(segments.segments[i]) + 2);
        sprintf(updatedPath, "%s%s%s", currentPath, (*currentPath) ? "/" : "", segments.segments[i]);

        free(currentPath);
        currentPath = updatedPath;

        printf("Checking directory: %s\n", segments.segments[i]);
        DirectoryEntry* dirEntry = DIR_find_directory_entry(segments.segments[i]);
        if (dirEntry == NULL) {
            printf("Directory '%s' does not exist\n", segments.segments[i]);
            free(dirEntry)
            return -1;
        } else {
            printf("Directory '%s' exists.\n", segments.segments[i]);
            free(dirEntry); // Assuming dirEntry was dynamically allocated
        }

        // Update parentPath to the last valid directory
        free(parentPath);
        parentPath = strdup(segments.segments[i]);
    }

    free(currentPath);
    free(parentPath);
    free_path_segments(&segments);
    return 1;
}

/////////////////////////////////////////////////////////////

 int fs_cp(const char* source_path, const char* dest_path) {

    // parantID id for the file where is going to be saved 
    uint32_t fileparentid = 0;

    char source_path_directory_path[256]; // Buffer for directory part
    char source_path_fileName[256];      // Buffer for source_path file name part

    char dest_path_directory_path[256]; // Buffer for directory part
    char dest_path_fileName[256];      // Buffer for source_path file name part

    split_path_fs_copy(source_path, source_path_directory_path, source_path_fileName);
    split_path_fs_copy(dest_path, dest_path_directory_path, dest_path_fileName);

    if (source_path_fileName == NULL) {
        printf("Error: Source path '%s' does not contain a file name.\n", source_path);
        return -1;
    }


    int sourcePathExist = check_full_file_existance(source_path_fileName);
    if (sourcePathExist == -1) {
        printf("Error: Source path '%s' does not exist.\n", source_path);
        return -1;
    }



    if (dest_path_directory_path == NULL){
        dest_path_directory_path = "/root"
     }
    else{
        // split dest_path_directory_path into parts and take the last part,

        // take the last part and check if it exists
        const char *dest_last_dir = strrchr(dest_path_directory_path, '/');
        if (dest_last_dir != NULL) {
        printf("Last directory: %s\n", dest_last_dir + 1);
        // coppy the last part of the dest_last_dir to the dest_path_directory_path
        dest_path_directory_path = dest_last_dir + 1;// to do needs to be fixed 
        } else {
        // If no slash is found, the whole path is a single directory name
        printf("Last directory: %s\n", dest_path_directory_path);
         }
         
    }

        DirectoryEntry* dirEntry = DIR_find_directory_entry(dest_path_directory_path);
        if (dirEntry == NULL) {
            printf("Directory '%s' does not exist\n", dest_path_directory_path);
            free(dirEntry)
            return -1;
        } 

         fileparentid = dirEntry->currentDirId;


        // now we have the fileparentid with that we can procced and recover the file from the source path
        // and search though the FileEntry fileSystem[MAX_FILES]; to find out in which index is found, once is found, 
        // we will update the filesystem[index]-> parentDirId = fileparentid
        uint32_t numbs = find_file_entry_by_name(source_path_fileName);
        if (numbs == -1) {
            printf("Error: File '%s' not found for reading.\n", source_path_fileName);
            return -1;
        }
        ///////////////////////////////////////////////////////////
        // now we will check if the file name exists in the directory
        uint32_t parent_ID_souce_dir = fileSystem[numbs].parentDirId;
        if (parent_ID_souce_dir == fileparentid) {
            // add a number to the filename
            char* newFileName = malloc(strlen(source_path_fileName) + 2);
            sprintf(newFileName, "%s1", source_path_fileName);
            strcpy(fileSystem[numbs].filename, newFileName);
        }
        ///////////////////////////////////////////////
        fileSystem[numbs].parentDirId = fileparentid;
        
        
        return 1;

 }
 

 int fs_mv(const char* old_path, const char* new_path){




    char source_directory_path[256] = {0};
    char source_filenames[256] = {0};
    
    char dest_directory_path[256] = {0};
    char dest_filename[256] = {0};


    


    split_path_fs_copy(source_path, source_directory_path, source_filenames);
    split_path_fs_copy(dest_path, dest_directory_path, dest_filename);

    // char source_filename[256] = prepend_slash(source_filenames);

    char source_filename[256]; // Declaration without initializer
    const char* modified_filename = prepend_slash(source_filenames); // Function call

    if (modified_filename != NULL) {
        strcpy(source_filename, modified_filename); // Copying the result into the array
    } else {
        printf("Failed to modify the source filename.\n");
    }
    


    printf("Source Directory: %s\n", source_directory_path);
    printf("Source Filename: %s\n", source_filename);
    printf("Destination Directory: %s\n", dest_directory_path);
    printf("Destination Filename: %s\n", dest_filename);


    
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", source_path);
        return -1;
    }

    // int sourcePathExist = check_full_file_existance(source_path_fileName);
    // if (sourcePathExist == -1) {
    //     printf("Error: Source path '%s' does not exist.\n", source_path);
    //     return -1;
    // }


    int fileIndexSource = find_file_entry_by_name(source_filename);
    if (fileIndexSource == -1) {
        printf("Error: Source file '%s' does not exist.\n", source_filename);
        return -1;
    }
    printf("File index of: %d\n", fileIndexSource);

    // if (dest_directory_path[0] == '\0') {
    //     strcpy(dest_directory_path, "/root");  // Default to root if no path provided
    // }

    if (dest_directory_path[0] == '\0') {
        strcpy(dest_directory_path, "/root");  // Set default path if empty
    } else {
        // Attempt to find the last slash in the path
        const char *lastSlash = strrchr(dest_directory_path, '/');
        if (lastSlash != NULL) {
            // There is at least one slash in the path
            lastSlash++;  // Move past the slash to the name of the last directory
            printf("Last directory: %s\n", lastSlash);
            strcpy(dest_directory_path, lastSlash);  // Copy the last directory name back to the array
        } else {
            // No slash found, the whole path is a single directory name
            printf("Last directory: %s\n", dest_directory_path);
        }
    }


    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path);
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }

    uint32_t parentID = destDirEntry->currentDirId;
    printf("BEFORE FREE Parent Directory ID: %u\n", parentID);
    free(destDirEntry);  // Don't forget to free the dynamically allocated memory
    printf("AFTER FREE Parent Directory ID: %u\n", parentID);

    // Check for duplicate file name in the destination directory
    printf("\n\nChecking for duplicate file name in the destination directory...\n");
     for (int i = 0; i < MAX_FILES; i++) {
        printf("Checking fileSystem[%d].in_use: %d\n", i, fileSystem[i].in_use);
        printf("Checking fileSystem[%d].filename: %s\n", i, fileSystem[i].filename);
        printf("Checking fileSystem[%d].parentDirId: %u\n", i, fileSystem[i].parentDirId);
        printf("Checking parentID: %u\n", parentID);
        printf("Checking source_filename: %s\n", source_filename);
        printf("Checking i: %d\n", i);
        if (fileSystem[i].in_use && strcmp(fileSystem[i].filename, source_filename) == 0 && fileSystem[i].parentDirId == parentID) {
            printf("File name already exists in the destination directory.\n");
            printf("creating new file name\n");
            char newFileName[256]; // Allocate space for new filename plus "copy"
            // snprintf(newFileName, sizeof(newFileName), "%scopy", source_filename);
            // strcpy(fileSystem[fileIndex].filename, newFileName);
            // printf("File name already exists, renamed to '%s'.\n", newFileName);
            // break;


        const char *lastDot = strrchr(source_filename, '.');
        if (lastDot) {
            // There's an extension. Copy up to the dot.
            int basenameLength = lastDot - source_filename;
            strncpy(newFileName, source_filename, basenameLength);
            newFileName[basenameLength] = '\0';  // Null-terminate after the basename

            // Append "Copy" and then the extension.
            strcat(newFileName, "Copy");
            strcat(newFileName, lastDot);  // Append the extension
        } else {
            // No extension found, just append "Copy".
            snprintf(newFileName, 256, "%sCopy", source_filename);
        }
        strcpy(dest_filename, newFileName);
        break;

        }
    }
    ////////////////////////////////////////////////////////////////
    // the file has to be coppy anyway, meaning the file has to created, so we will create a new entry where we will copy the contents 
    // of the old file, that contents will be size, buffer, in_use, is_directory
    printf("destination filename11: %s\n", dest_filename);    

    FS_FILE* fileCopy = fs_open_for_coppy(dest_filename, "w"); // this will be the coppy file
        if (fileCopy == NULL) {
            printf("Error: Failed to open file '%s' for copying.\n", source_filename);
            return -1;
        }

    // now open the previous file to read the contents
    FS_FILE* oldfile = fs_open(source_filename, "r");

    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }
    uint32_t uniqueIdFile = fileCopy->entry->unique_file_id;
    // look though the global fileSystem array to find the index of the file that we want to copy, from oldfile into fileCopy
     int fileIndex = find_file_entry_by_unique_file_id(uniqueIdFile);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", source_filename);
        return -1;
    }

    // now we have the index of the file that we want to copy, we can now copy the contents of the old file into the new file
    // we will copy the contents of the old file into the new file
    fileSystem[fileIndex].in_use = true;
    fileSystem[fileIndex].is_directory = false;
    fileSystem[fileIndex].size = oldfile->entry->size;
    fileSystem[fileIndex].parentDirId = parentID; /// this one we will change 
    memcpy( fileSystem[fileIndex].buffer, oldfile->entry->buffer, sizeof(oldfile->entry->buffer));



    printf("Checking for duplicate file name in the destination directory...DONE\n\n");

    // Copy the file to the new directory
    // fileSystem[fileIndex].parentDirId = parentID;
    printf("File '%s' successfully copied to '%s'.\n", source_filename, dest_directory_path);
 
    printf("\n\nDEBUG section Befote write fs_cp\n");
    printf("fileIndex: %d\n", fileIndex);
    printf("fileSystem[fileIndex].filename: %s\n", fileSystem[fileIndex].filename);
    printf("fileSystem[fileIndex].size: %u\n", fileSystem[fileIndex].size);
    printf("fileSystem[fileIndex].start_block: %u\n", fileSystem[fileIndex].start_block);
    printf("fileSystem[fileIndex].in_use: %d\n", fileSystem[fileIndex].in_use);
    printf("fileSystem[fileIndex].is_directory: %d\n", fileSystem[fileIndex].is_directory);
    printf("fileSystem[fileIndex].buffer: %s\n", fileSystem[fileIndex].buffer);
    printf("fileSystem[fileIndex].unique_file_id: %u\n", fileSystem[fileIndex].unique_file_id);
    printf("DEBUG FINISH\n\n");

    
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);
    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));
    printf("Data written to file.\n");
    printf("File '%s' successfully copied to '%s'.\n", source_filename, dest_directory_path);
    



   return 0;




    
 }




