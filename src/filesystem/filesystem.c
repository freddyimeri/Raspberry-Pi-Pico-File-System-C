 
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


 
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


/////fs_seek/////////////
#define SEEK_SET 0  // Start of the file
#define SEEK_CUR 1  // Current position in the file
#define SEEK_END 2  // End of the file
////////////////////////

 
bool fs_initialized = false;
static mutex_t filesystem_mutex;
bool isValidChar(char c);
bool isValidPath(const char* path);

FileEntry fileSystem[MAX_FILES];
 
 
 

void reset_file_content(FileEntry* entry);
FileMode determine_mode(const char* mode);



bool isValidChar(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '/';
}

// Validates the path format and characters
bool isValidPath(const char* path) {
    // Path must start with a "/" character
    if (path[0] != '/') {
        printf("Error: Path must start with '/'.\n");
        return false;
    }

    // Check each character in the path
    for (int i = 1; path[i] != '\0'; i++) {
        if (!isValidChar(path[i])) {
            printf("Error: Invalid character '%c' in path.\n", path[i]);
            return false;
        }

        // Additional checks can be added here, such as:
        // - Disallowing consecutive slashes
        // - Limiting the total length of the path
        // - Limiting the depth of directories

        // Example: disallow consecutive slashes
        if (path[i] == '/' && path[i-1] == '/') {
            printf("Error: Consecutive slashes are not allowed.\n");
            return false;
        }
    }

    // Path is considered valid
    return true;
}


/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
 
    fat_init();
    mutex_init(&filesystem_mutex);

    fs_initialized = true;
   
    uint32_t current_start_block = FLASH_TARGET_OFFSET;

    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;  // Initially, no file is in use
        fileSystem[i].size = 0;        // Initialize the file size to 0
        // printf("fs_init: fileSystem[%d].size = %d\n", i, fileSystem[i].size);
        fflush(stdout);

        // Initialize the flash address for each file entry
        fileSystem[i].start_block = current_start_block;

        // Move the current flash address to the start of the next sector
        // making sure each file starts at the beginning of a new sector
        current_start_block += MAX_FILE_SIZE;
         
        printf("Current start block: %u\n", current_start_block);
        fflush(stdout);

        // Check if the new flash address exceeds the flash size, wrap around, or handle as error
        if (current_start_block >= FLASH_TARGET_OFFSET + FLASH_USABLE_SPACE) {
            // Handle the case where we've exceeded the flash space
            // This could be an error, or you might want to wrap around
            printf("Error: Not enough flash memory for the number of files.\n");
            fflush(stdout);
            return;
        }
    }

    int resetSuccess = reset_root_directory();
    if (!resetSuccess) {
        printf("Critical error initializing root directory.\n");
        // Handle the error. Consider reverting `fs_initialized` if needed or taking other corrective action.
        fs_initialized = false; // Consider setting this based on your error handling strategy.
        return;
    }
    fs_initialized = true;
    printf("Filesystem initialized.\n");

    
    
}


 
FS_FILE* fs_open(const char* FullPath, const char* mode) {

     if (FullPath == NULL || mode == NULL) {
        printf("Error: Path or mode is NULL.\n");
        fflush(stdout);
        return NULL;
    }

     if (strcmp(mode, "a") == 0) 
    {
        printf("Opening file '%s' in append mode.\n", FullPath);
        const char* path = prepend_slash(FullPath);
        
        printf("Path: %s\n", path);
        FileEntry* entry = FILE_find_file_entry(path);
        
        
        // int len = strlen(entry->buffer);
        int len = entry->size;  // Assuming size is maintained correctly
        printf("len: %d\n", len);


        if (entry == NULL) {
        printf("Error: File '%s' not found for reading.\n", FullPath);
        return NULL;
        }


            // Create and initialize the FS_FILE structure
        FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
        if (file == NULL) {
            printf("Error: Memory allocation failed for FS_FILE.\n");
            fflush(stdout);
            return NULL;
        }

        file->entry = entry;
        file->position = file->position + len; // Append mode sets position to the end
        file->mode = 'a';  // Simplified mode determination
        // strcpy(file->mode, "a"); 
        

    
        return file;

    }
   

    if (strcmp(mode, "r") == 0) 
    {
        printf("Opening file '%s' in read mode.\n", FullPath);
        const char* path = prepend_slash(FullPath);
        
        printf("Path: %s\n", path);
        FileEntry* entry = FILE_find_file_entry(path);


        if (entry == NULL) {
        printf("Error: File '%s' not found for reading.\n", FullPath);
        return NULL;
        }


            // Create and initialize the FS_FILE structure
        FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
        if (file == NULL) {
            printf("Error: Memory allocation failed for FS_FILE.\n");
            fflush(stdout);
            return NULL;
        }

        file->entry = entry;
        file->position = 0; // Append mode sets position to the end
        file->mode = 'r'; // Simplified mode determination
        // strcpy(file->mode, "r"); 
  

    
        return file;

    }
   


    if (strcmp(mode, "w") == 0) {

        printf("Opening file '%s' in write mode.\n", FullPath);
        PathParts parts = extract_last_two_parts(FullPath);
        const char* path = prepend_slash(parts.filename);
        const char* directoryName = parts.directory;
        if (path != NULL) {
        printf("Modified Path: %s\n", path);
        } else {
            printf("Failed to modify the path.\n");
        }

        if (directoryName[0] == '\0') {
            directoryName = "/root";  // Default to the root directory
            printf("No directory specified, defaulting to root: %s\n", directoryName);
        }

        FileEntry* entry = FILE_find_file_entry(path);
        bool isNewFileCreation = (entry == NULL) && (strchr("wa", mode[0]) != NULL);
 
         DirectoryEntry* dirEntry = DIR_find_directory_entry(directoryName);
        
        uint32_t parentID;
        if (dirEntry) {
            parentID = dirEntry->currentDirId;
            // If the directory is found, access its parentDirId and other details
            printf("Directory '%s' found with ParentDirId: %u\n", directoryName, dirEntry->currentDirId);
            free(dirEntry);
        } else {
            // If the directory is not found, handle this case
            printf("EROR: Directory '%s' not found.\n", directoryName);
        }
        printf("code 3\n");
        // Attempt to create a new file entry if the file does not exist and the mode allows for writing or appending
        if (isNewFileCreation) {
            entry = create_new_file_entry(path, parentID);
            if (entry == NULL) {
                return NULL; // Error message is handled in create_new_file_entry
            }
        } else if (entry == NULL) {
            printf("Error: File not found for reading.\n");
            fflush(stdout);
            return NULL;
        }
        printf("code 4\n");
        // Special handling for 'w' mode: reset the file if it exists


        // reset_file_content(entry);
        uint32_t offsetInBytes;
        const uint32_t offsetFromEntry = entry->start_block;
        printf("offsetFromEntry dd: %u\n", offsetFromEntry);
        //print the fileName 
        offsetInBytes =  offsetFromEntry * FILESYSTEM_BLOCK_SIZE;
        printf("offsetInBytes dd: %u\n", offsetInBytes);
        flash_write_safe2(offsetInBytes, (const uint8_t*)entry, sizeof(FileEntry));

            // Create and initialize the FS_FILE structure
        FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
        if (file == NULL) {
            printf("Error: Memory allocation failed for FS_FILE.\n");
            fflush(stdout);
            return NULL;
        }

        file->entry = entry;
        file->position = 0 ;  
        // strcpy(file->mode, "w"); // Simplified mode determination
        file->mode = 'w'; // Simplified mode determination

 
    return file;
 

    }
   
    return NULL;
}




// Function to PRINT write data to a file
int fs_write(FS_FILE* file, const void* buffer, int size) {

    char tempFileName[256];  

    if (strlen(file->entry->filename) < sizeof(tempFileName)) {
        strcpy(tempFileName, file->entry->filename);  // Copy the filename
        printf("Filename copied: %s\n", tempFileName);
    } else {
        printf("Filename too long to fit in tempFileName\n");
    }

    uint32_t numbs = find_file_entry_by_name(tempFileName);
    printf("numbs: %d\n", numbs);

    
    printf("file->mode ss: %c\n", file->mode);
    if (file->mode == 'a') {  
        printf("Append mode fs_write\n");
        // Copy the existing file content to a temporary buffer
        char tempBuffer[256];
        strcpy(tempBuffer, fileSystem[numbs].buffer);
        printf("tempBuffer: %s\n", tempBuffer);
        // Append the new data to the temporary buffer
        strncat(tempBuffer, buffer, size);
        printf("tempBuffer: %s\n", tempBuffer);
        // Copy the updated content back to the file entry buffer
        strcpy(fileSystem[numbs].buffer, tempBuffer);
        printf("fileSystem[numbs].buffer: %s\n", fileSystem[numbs].buffer);

        ////
        // ADD The int size to the fileSystem[numbs].size 
        fileSystem[numbs].size += size;
     
    } else {
           printf("NON Append mode fs_write\n");
        // Copy the new data directly to the file entry buffer
        strcpy(fileSystem[numbs].buffer, buffer); 
    }

    

    uint32_t writeOffset = (fileSystem[numbs].start_block * FILESYSTEM_BLOCK_SIZE);
    
    // Perform the write operation
    // flash_write_safe2(writeOffset, (const uint8_t*)buffer, size);
    printf("start debug fs_write\n");
    printf("fileSystem[numbs]->buffer: %s\n", fileSystem[numbs].buffer);
    printf("fileSystem[numbs]>filename: %s\n", fileSystem[numbs].filename);
    printf("fileSystem[numbs]>size: %u\n", fileSystem[numbs].size);
    printf("fileSystem[numbs]->start_block: %u\n", fileSystem[numbs].start_block);
    printf("fileSystem[numbs]>parentDirId: %u\n", fileSystem[numbs].parentDirId);
    printf("fileSystem[numbs]>is_directory: %d\n", fileSystem[numbs].is_directory);
    printf("fileSystem[numbs]>in_use: %d\n", fileSystem[numbs].in_use);
    printf("writeOffset: %u\n", writeOffset);
    fflush(stdout);
    printf("finish debug fs_write\n\n");


    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[numbs], sizeof(FileEntry));

    // flash_write_safe2(writeOffset, (const uint8_t*)fileSystem[numbs], sizeof(fileSystem[numbs]));
    printf("Data written to file fffffffffffffffffff.\n");
    // Update file position
    file->position += size;

    return size;  // Return the number of bytes written
}
 
 

FileEntry* create_new_file_entry(const char* path, uint32_t parentID) {
    printf("debug create_new_file_entry for path: %s\n", path);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fileSystem[i].in_use) {
            strncpy(fileSystem[i].filename, path, sizeof(fileSystem[i].filename) - 1);
            fileSystem[i].filename[sizeof(fileSystem[i].filename) - 1] = '\0';
            fileSystem[i].in_use = true;
            fileSystem[i].is_directory = false; // Default to file
            fileSystem[i].size = 0;
            fileSystem[i].start_block = fat_allocate_block();
            fileSystem[i].parentDirId = parentID; 

            printf("New file created: %s\n", fileSystem[i].filename);
            printf("Start block: %u\n", fileSystem[i].start_block);
            printf("File size: %u\n", fileSystem[i].size);
            printf("Filesystem entry index: %d\n", i);
            printf("Parent Directory ID: %u\n", fileSystem[i].parentDirId);
            
            fflush(stdout);
            if (fileSystem[i].start_block == FAT_NO_FREE_BLOCKS) {
                printf("Error: No space left on device to create new file.\n");
                fflush(stdout);
                memset(&fileSystem[i], 0, sizeof(FileEntry)); // Cleanup
                fileSystem[i].in_use = false; // Explicitly mark it as not in use
                return NULL;
            }
            return &fileSystem[i];
        }
    }
    printf("Error: Filesystem is full, cannot create new file.\n");
    fflush(stdout);
    return NULL;
}

 

void reset_file_content(FileEntry* entry) {
    uint32_t block = entry->start_block;
    uint32_t nextBlock;
    int result;

    // Continue looping until we reach the end of the file's block chain
    while (block != FAT_ENTRY_END && block < TOTAL_BLOCKS) {
        result = fat_get_next_block(block, &nextBlock);
        // Free the current block before moving to the next
        fat_free_block(block);

        // If we encounter an error or the end of the chain, stop processing
        if (result != FAT_SUCCESS || nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Exit the loop if an error occurs or the end of the chain is reached
        }

        block = nextBlock; // Move to the next block in the chain
    }

    // Allocate a new start block for the file and reset its size
    entry->start_block = fat_allocate_block();
    entry->size = 0;
    if (entry->start_block == FAT_NO_FREE_BLOCKS) {
        printf("Error: Unable to allocate a new block for the file.\n");
        fflush(stdout);
        // Additional error handling could be implemented here if necessary
    }
    printf("File content reset successfully.\n");
    fflush(stdout);
    printf("New start block: %u\n", entry->start_block);
    fflush(stdout);
    printf("New file size: %u\n", entry->size);
    fflush(stdout);
    printf("File content reset successfully.\n");
}


FileMode determine_mode(const char* mode) {
    if (strcmp(mode, "r") == 0) return MODE_READ;
    if (strcmp(mode, "w") == 0) return MODE_WRITE;
    if (strcmp(mode, "a") == 0) return MODE_APPEND;
    printf("Error: Unrecognized file mode '%s'. Defaulting to read mode.\n", mode);
    fflush(stdout);
    return MODE_READ; // Default to read if mode is unrecognized
}



/**
 * Closes the specified file.
 *
 * @param file A pointer to the file to be closed.
 */
void fs_close(FS_FILE* file) {
    if (file == NULL) {
        printf("Error: Attempted to close a NULL file pointer.\n");
        fflush(stdout);
        return;
    }

    free(file);
}

// /**
//  * Reads data from the specified file into the provided buffer.
//  *
//  * @param file   A pointer to the FS_FILE structure representing the file to read from.
//  * @param buffer A pointer to the buffer where the read data should be stored.
//  * @param size   The number of bytes to read.
//  * @return The number of bytes actually read, or -1 if an error occurred.
//  */
// int fs_read(FS_FILE* file, void* buffer, int size) {
//     if (file == NULL || buffer == NULL || size <= 0) {
//         printf("Error: Invalid parameters for fs_read.\n");
//         fflush(stdout);
//         return -1;
//     }

//     // if (file->mode != MODE_READ && file->mode != MODE_APPEND) {
//     //     printf("Error: File is not open in a readable mode.\n");
//     //     fflush(stdout);
//     //     return -1;
//     // }
//     // printf("Starting fs_read: file position before read = %u, read size = %d\n", file->position, size);
//     // fflush(stdout);
//     int totalBytesRead = 0;
//     uint8_t* buf = (uint8_t*)buffer;

//     // Calculate initial blockIndex and blockOffset based on file->position
//     uint32_t blockIndex = file->position / FILESYSTEM_BLOCK_SIZE;
//     uint32_t blockOffset = file->position % FILESYSTEM_BLOCK_SIZE;

//     while (size > 0 && file->position < file->entry->size) {
//         // Navigate through the FAT to find the correct block based on blockIndex
//         uint32_t currentBlock = file->entry->start_block;
//         // Print debug for current block
//         printf("fs_read : Current block: %u\n", currentBlock);
//         uint32_t nextBlock;
//         int result;
//         for (uint32_t i = 0; i < blockIndex; i++) {
//             result = fat_get_next_block(currentBlock, &nextBlock);
//             if (result == FAT_END_OF_CHAIN || result == FAT_CORRUPTED) {
//                 printf("Error: Reached end of file chain or encountered corruption.\n");
//                 fflush(stdout);
//                 return -1; // Handle the error appropriately
//             }
//             currentBlock = nextBlock;
//         }

        
//         if (currentBlock == FAT_ENTRY_END) {
//             printf("Error: Reached end of file chain prematurely.\n");
//             fflush(stdout);
//             break; // Break the loop if the end of the chain is reached unexpectedly
//         }

//         // Calculate the number of bytes to read in this iteration
//         int bytesToRead = min(size, FILESYSTEM_BLOCK_SIZE - blockOffset);
//         bytesToRead = min(bytesToRead, (int)(file->entry->size - file->position));

//         // Calculate the read offset for the current block
//         uint32_t readOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + blockOffset;
//         printf("Performing read from block = %u, offset within block = %u (global readOffset = %u)\n", currentBlock, blockOffset, readOffset);
//         fflush(stdout);

//         // Perform the actual read operation
//         flash_read_safe2(readOffset, buf + totalBytesRead, bytesToRead);

//         // Update counters and positions for the next iteration
//         totalBytesRead += bytesToRead;
//         file->position += bytesToRead;
//         size -= bytesToRead;

//         // Reset blockOffset to 0 for subsequent blocks
//         blockOffset = 0;

//         // Move to the next block in the FAT chain, if needed
//         if (bytesToRead + blockOffset >= FILESYSTEM_BLOCK_SIZE) {
//             blockIndex++;
//         }
//     }

//     // printf("Completed fs_read: totalBytesRead = %d, final file position = %u\n", totalBytesRead, file->position);
//     // fflush(stdout);
//     return totalBytesRead;
// }
   

/**
 * Reads data from the specified file into the provided buffer.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to read from.
 * @param buffer A pointer to the buffer where the data will be stored.
 * @param size   The maximum number of bytes to read.
 * @return The number of bytes actually read, or -1 if an error occurred.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    if (file == NULL || buffer == NULL) {
        printf("Error: Null file or buffer pointer provided.\n");
        return -1;  // Error due to invalid pointers
    }

    if (size <= 0) {
        printf("Error: Invalid size to read (%d).\n", size);
        return -1;  // Error due to non-positive size
    }

    if (file->mode != 'r' && file->mode != 'a') {
        printf("Error: File is not open in a readable or append mode.\n");
        return -1;  // Error due to incorrect mode
    }

    int bytes_read = 0;  // Counter for the number of bytes actually read
    int bytes_to_read = size;  // Adjust the size to not exceed the file size
    char* char_buffer = (char*)buffer;  // Cast buffer to a char pointer for byte-wise manipulation

    // Ensure we do not read past the end of the file
    if (file->position + bytes_to_read > file->entry->size) {
        bytes_to_read = file->entry->size - file->position;
    }

    if (bytes_to_read <= 0) {  // Check if there's nothing to read
        printf("No more data to read from file.\n");
        return 0;  // No data read
    }

    // Simulate the read operation (assuming the data starts right at the beginning of the buffer)
    // Normally you'd perform actual storage read operations here.
    memcpy(char_buffer, file->entry->buffer + file->position, bytes_to_read);
    file->position += bytes_to_read;  // Update the position in the file

    

    printf("Read %d bytes from file '%s'.\n", bytes_to_read, file->entry->filename);
    return bytes_to_read;  // Return the number of bytes read
}

/**
 * Sets the file position indicator for the specified file.
 *
 * @param file A pointer to the FS_FILE structure representing the file.
 * @param offset The number of bytes to offset from the specified position.
 * @param whence The position from which to calculate the offset. 
 *               Can be one of the following:
 *               SEEK_SET - Sets the position relative to the beginning of the file.
 *               SEEK_CUR - Sets the position relative to the current position.
 *               SEEK_END - Sets the position relative to the end of the file.
 * @return 0 if successful, or -1 if an error occurred (such as attempting to seek
 *         to an invalid position or passing an invalid file pointer or whence value).
 */
int fs_seek(FS_FILE* file, long offset, int whence) {
    if (file == NULL) {
        printf("Error: Null file pointer provided.\n");
        return -1;  // Error due to invalid file pointer
    }

    long new_position;  // This will hold the computed new position based on the 'whence' and 'offset'

    switch (whence) {
        case SEEK_SET:
            // Position is set to 'offset' bytes from the start of the file.
            new_position = offset;
            break;
        case SEEK_CUR:
            // Position changes by 'offset' bytes from the current position.
            new_position = file->position + offset;
            break;
        case SEEK_END:
            // Position is set to 'offset' bytes from the end of the file.
            // If offset is negative, it positions backward from the end of the file.
            new_position = file->entry->size + offset;
            break;
        default:
            printf("Error: Invalid 'whence' argument (%d).\n", whence);
            return -1; // Error due to invalid 'whence' value
    }

    // Validate the new position to ensure it is within the valid range of the file.
    if (new_position < 0 || new_position > file->entry->size) {
        printf("Error: Attempted to seek to an invalid position (%ld).\n", new_position);
        return -1; // The new position is out of bounds
    }

    // Successfully set the new position
    file->position = new_position;
    return 0; // Success indicates the new position was set without issues
}////////////////////////////// 
 
int fs_mv(const char* old_path, const char* new_path) {
    // Validate input paths
    if (old_path == NULL || new_path == NULL) {
        printf("Error: Both old and new paths must be provided.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure source and destination are not the same
    if (strcmp(old_path, new_path) == 0) {
        printf("Error: Source and destination paths are the same.\n");
        fflush(stdout);
        return -2; // Source and destination are the same
    }

    // Ensure the source file exists
    FileEntry* srcEntry = FILE_find_file_entry(old_path);
    if (srcEntry == NULL) {
        printf("Error: Source file does not exist.\n");
        fflush(stdout);
        return -3; // Source file not found
    }

    // Ensure the destination file does not exist
    FileEntry* destEntry = FILE_find_file_entry(new_path);
    if (destEntry != NULL) {
        printf("Error: Destination file already exists.\n");
        fflush(stdout);
        return -4; // Destination file exists
    }

    // Perform the move operation
    // In a simple filesystem, this might just be renaming the entry
    // Ensure there's space in the filename field
    if (strlen(new_path) >= sizeof(srcEntry->filename)) {
        printf("Error: New path is too long.\n");
        fflush(stdout);
        return -5; // New path too long
    }

    // Update the file entry with the new path
    strncpy(srcEntry->filename, new_path, sizeof(srcEntry->filename));
    srcEntry->filename[sizeof(srcEntry->filename) - 1] = '\0'; // Ensure null termination

    printf("File moved from '%s' to '%s'.\n", old_path, new_path);
    fflush(stdout);
    return 0; // Success
}

/////////////


int fs_wipe(const char* path) {
    printf("Wiping path: %s\n", path);
    // Check if path is NULL
    if (path == NULL) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Indicate an error
    }

    // Find the file or directory entry
    FileEntry* entry = FILE_find_file_entry(path);
    if (entry == NULL) {
        printf("Error: File or directory not found.\n");
        fflush(stdout);
        return -2; // Indicate file not found
    }

    // If it's a directory, recursively wipe its contents
    if (entry->is_directory) {
        // The implementation here assumes you have a way to list and iterate through directory contents.
        DirectoryEntry* dirEntries = fs_list_directory(path);  
        for (int i = 0; dirEntries[i].name[0] != '\0'; i++) {
            // Construct the full path for each entry
            char fullPath[256]; // Assuming 256 is the max path length
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, dirEntries[i].name);

            // Recursively call fs_wipe on each entry
            int result = fs_wipe(fullPath);
            if (result != 0) {
                printf("Warning: Failed to wipe '%s' within directory.\n", fullPath);
                fflush(stdout);
                // Decide how to handle partial failures. Continue, abort, or revert?
            }
        }
        // Depending on your design, you may need to free any allocated memory for dirEntries
    }

    uint32_t currentBlock = entry->start_block;
    uint32_t nextBlock;
    int result;

    // Deallocate blocks associated with this file or directory
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered FAT_ENTRY_END prematurely. Error code: %d\n", result);
            fflush(stdout);
            break; // Handle error or premature end appropriately
        }
        fat_free_block(currentBlock);
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Properly end loop if at the end or if an invalid next block is encountered
        }
        currentBlock = nextBlock;
    }

    // Mark the file system entry as not in use and optionally clear it
    entry->in_use = false;
    memset(entry, 0, sizeof(FileEntry));

    return 0; // Indicate success
}

///////////////////////////////////////////
 
extern void flash_erase_all(void);

int fs_format(const char* path) {
    // Ensure the format request is intended for the root directory
    if (strcmp(path, "/root") != 0) {
        printf("Error: fs_format only supports formatting the root directory.\n");
        fflush(stdout);
        return -1;
    }

    // Securely erase all flash storage used by the filesystem
    flash_erase_all();

    // Reinitialize the FAT to mark all blocks as free
    fat_init();

    // Reset or reinitialize the root directory
    int result = reset_root_directory();
    if (result != 0) {
        printf("Error: Failed to reset the root directory (error %d).\n", result);
        fflush(stdout);
        return result;
    }

    printf("Filesystem formatted successfully.\n");
    fflush(stdout);
    return 0; // Indicate success
}



////////////////////////////

int fs_cp(const char* source_path, const char* dest_path) {
    // Validate input paths
    if (!source_path || !dest_path) {
        printf("Error: Source or destination path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure source file exists
    FS_FILE* srcFile = fs_open(source_path, "r");
    if (srcFile == NULL) {
        printf("Error: Source file '%s' does not exist.\n", source_path);
        fflush(stdout);
        return -2; // Source file not found
    }

    // Check if destination file already exists to avoid unintentional overwrite
    FS_FILE* destFileCheck = fs_open(dest_path, "r");
    if (destFileCheck != NULL) {
        printf("Error: Destination file '%s' already exists.\n", dest_path);
        fflush(stdout);
        fs_close(destFileCheck); // Close the file opened for checking
        fs_close(srcFile); // Don't forget to close the source file as well
        return -3; // Destination file exists
    }

    // Open destination file for writing (creates the file)
    FS_FILE* destFile = fs_open(dest_path, "w");
    if (destFile == NULL) {
        printf("Error: Failed to create destination file '%s'.\n", dest_path);
        fflush(stdout);
        fs_close(srcFile); // Ensure the source file is closed to prevent resource leakage
        return -4; // Failed to create destination file
    }

    // Copy content from source to destination
    char buffer[512]; // Choose an appropriate buffer size based on your system's constraints
    int bytesRead, bytesWritten;
    while ((bytesRead = fs_read(srcFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = fs_write(destFile, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            printf("Error: Failed to write to destination file '%s'.\n", dest_path);
            fflush(stdout);
            fs_close(srcFile);
            fs_close(destFile);
            return -5; // Write failure
        }
    }

    if (bytesRead < 0) {
        printf("Error: Failed to read from source file '%s'.\n", source_path);
        fflush(stdout);
        fs_close(srcFile);
        fs_close(destFile);
        return -6; // Read failure
    }

    // Close files to release resources
    fs_close(srcFile);
    fs_close(destFile);

    printf("File '%s' successfully copied to '%s'.\n", source_path, dest_path);
    fflush(stdout);
    return 0; // Success
}




//////////////////////////////
 
int fs_rm(const char* path) {
    // Validate the input path
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Invalid arguments
    }

    // Ensure the file exists
    FileEntry* fileEntry = FILE_find_file_entry(path);
    if (!fileEntry) {
        printf("Error: File '%s' not found.\n", path);
        fflush(stdout);
        return -2; // File not found
    }

    // Prevent deletion if the target is a directory (if applicable)
    if (fileEntry->is_directory) {
        printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
        fflush(stdout);
        return -3; // Attempt to remove a directory with a file removal function
    }

    // Free all blocks associated with this file
    uint32_t currentBlock = fileEntry->start_block;
    uint32_t nextBlock;
    int result;

    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
            fflush(stdout);
            // Decide how to handle error: break out of the loop
            break; 
        }

        fat_free_block(currentBlock);

        // Check if the end of the chain is reached or an invalid block index is encountered
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // Properly end loop
        }

        currentBlock = nextBlock;
    }
    // Mark the file entry as not in use
    memset(fileEntry, 0, sizeof(FileEntry));
    fileEntry->in_use = false;

    printf("File '%s' successfully removed.\n", path);
    fflush(stdout);
    return 0; // Success
}








FileEntry* resolve_path(const char* path) {
    if (path == NULL || path[0] != '/') {
        printf("Error: Invalid path. Path must start with '/'\n");
        return NULL; // Invalid path
    }

    if (strcmp(path, "/root") == 0) {
        // Special case for root directory
        return FILE_find_file_entry(path);
    }

    char pathCopy[256]; // Assuming path length won't exceed 255 characters
    strncpy(pathCopy, path, sizeof(pathCopy));
    pathCopy[sizeof(pathCopy) - 1] = '\0'; // Ensure null termination

    char* token = strtok(pathCopy, "/");
    FileEntry* currentEntry = NULL;
    FileEntry* parentEntry = NULL;

    while (token != NULL) {
        char fullPath[256] = "/";
        if (parentEntry != NULL) {
            strncat(fullPath, parentEntry->filename, sizeof(fullPath) - strlen(fullPath) - 1);
            strncat(fullPath, "/", sizeof(fullPath) - strlen(fullPath) - 1);
        }
        strncat(fullPath, token, sizeof(fullPath) - strlen(fullPath) - 1);

        currentEntry = FILE_find_file_entry(fullPath);
        if (currentEntry == NULL) {
            // Path resolution failed, entry not found
            printf("Error: Path resolution failed at '%s'.\n", token);
            return NULL;
        }

        parentEntry = currentEntry; // Move to the next level
        token = strtok(NULL, "/");
    }

    return currentEntry;
}














uint32_t fs_seek_directory(const char* path) {
    // First, find the directory entry for the given path.
    DirectoryEntry* dirEntry = find_directory_entry(path);
    if (dirEntry == NULL || !dirEntry->is_directory) {
        printf("Directory '%s' not found or is not a directory.\n", path);
        return 0; // Indicates failure to find the directory
    }

    // Assuming `start_block` of the DirectoryEntry indicates the first block of the directory in the FAT.
    uint32_t startBlock = dirEntry->start_block;

    // Translate the start block to a physical address in flash memory.
    // This calculation depends on how your flash memory is organized and how blocks are mapped to physical addresses.
    uint32_t address = FLASH_TARGET_OFFSET + startBlock * FILESYSTEM_BLOCK_SIZE;

    // Check if the calculated address is within the flash memory bounds.
    if (address >= FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
        printf("Calculated address for directory '%s' is out of flash memory bounds.\n", path);
        return 0; // Indicates failure due to out-of-bounds address
    }

    // The address is valid; return it.
    return address;
}



// Function to list all files in the filesystem
FileEntry* list_all_files(size_t *count) {
    printf("Scanning all blocks for file entries...\n");
    fflush(stdout);

    // Allocate initial buffer to hold file entries
    size_t capacity = 10;  // Initial capacity for file entries
    FileEntry *files = malloc(capacity * sizeof(FileEntry));
    if (!files) {
        printf("Memory allocation failed for file list.\n");
        fflush(stdout);
        *count = 0;
        return NULL;
    }

    size_t numFiles = 0;  // Number of files found
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {
            uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            FileEntry entry;
            flash_read_safe2(address, (uint8_t *)&entry, sizeof(FileEntry));

            // Check if it's a file entry and it's in use
            if (entry.in_use && !entry.is_directory) {
                if (numFiles >= capacity) {
                    // Increase capacity if needed
                    capacity *= 2;
                    FileEntry *newFiles = realloc(files, capacity * sizeof(FileEntry));
                    if (!newFiles) {
                        printf("Memory reallocation failed.\n");
                        fflush(stdout);
                        free(files);
                        *count = 0;
                        return NULL;
                    }
                    files = newFiles;
                }

                // Store the file entry
                files[numFiles++] = entry;
            }
        }
    }

    *count = numFiles;  // Set the count of files found
    return files;  // Return the array of files
}


FileEntry* FILE_find_file_entry(const char* filename) {
    const char* newfilename = prepend_slash(filename);
    printf("\n\nENTERED FILE_find_file_entry\n");
    fflush(stdout);

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {
            printf("Checking block %u\n", i);
            fflush(stdout);

            uint32_t address = i * FILESYSTEM_BLOCK_SIZE;
            FileEntry fileEntry;  // Temporary storage
            flash_read_safe2(address, (uint8_t *)&fileEntry, sizeof(FileEntry));

            printf("Checking file name: %s\n", fileEntry.filename);
            printf("Checking file size: %u\n", fileEntry.size);
            printf("Checking file start block: %u\n", fileEntry.start_block);
            printf("Checking file in_use: %d\n", fileEntry.in_use);
            printf("Checking if directory flag: %d\n", fileEntry.is_directory);
            printf("buffer: %s\n", fileEntry.buffer);
            fflush(stdout);
            
            if (fileEntry.in_use && !fileEntry.is_directory && strcmp(fileEntry.filename, newfilename) == 0) {
                printf("File entry found: %s\n", newfilename);
                fflush(stdout);
                FileEntry* result = malloc(sizeof(FileEntry));  // Dynamically allocate memory
                if (result) {
                    *result = fileEntry;  // Copy the data
                    return result;
                } else {
                    printf("Memory allocation failed.\n");
                    fflush(stdout);
                    return NULL;
                }
            }
            sleep_ms(200);
        }
    }

    printf("File entry not found: %s\n", filename);
    fflush(stdout);
    return NULL;
}