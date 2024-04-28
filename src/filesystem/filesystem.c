 
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

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
 

/////fs_seek/////////////
#define SEEK_SET 0  // Start of the file
#define SEEK_CUR 1  // Current position in the file
#define SEEK_END 2  // End of the file
////////////////////////
// 1589  
 
bool fs_initialized = false;
static mutex_t filesystem_mutex;
FileEntry fileSystem[MAX_FILES];


bool isValidChar(char c);
bool isValidChar(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '/';
}




/**
 * Initializes the filesystem - this function should be called at the start of your program.
 * It sets all file entries to not in use, preparing the file system for operation.
 */
void fs_init() {
 
    fat_init();
    mutex_init(&filesystem_mutex);

    fs_initialized = true;
    
    init_directory_entries();
    init_file_entries();

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

// we can
void init_file_entries() {
    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;
        fileSystem[i].is_directory = false;
        fileSystem[i].filename[0] = '\0';  // Empty string indicates unused
        fileSystem[i].parentDirId = 0;
        fileSystem[i].size = 0;
        fileSystem[i].start_block = 0;
        // memset(fileSystem[i].buffer, 0, sizeof(fileSystem[i].buffer));  // Clear the buffer
        fileSystem[i].unique_file_id = 0;
    }
}


FS_FILE* fs_open(const char* FullPath, const char* mode) {
    PathParts pathExtract = extract_last_two_parts(FullPath);
    char* filename = pathExtract.filename;
    char* directory_path = pathExtract.directory;

    set_default_path(directory_path, "/root");

    DirectoryEntry* directory = DIR_find_directory_entry(directory_path);
    if (!directory) {
        printf("Error: Directory '%s' not found.\n", directory_path);
        fflush(stdout);
        return NULL;
    }
    uint32_t parentDirId = directory->currentDirId;

    if (filename[0] == '\0') {
        printf("Error: Path '%s' does not contain a valid file name.\n", FullPath);
        fflush(stdout);
        return NULL;
    }

    FS_FILE* file = NULL;
    FileEntry* entry = NULL;
    
    if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0 || strcmp(mode, "r") == 0) {
        entry = (strcmp(mode, "w") == 0) ? createFileEntry(filename, parentDirId) : FILE_find_file_entry(filename, parentDirId);
        if (!entry) {
            printf("Error: File '%s' not found or cannot be created.\n", filename);
            fflush(stdout);
            return NULL;
        }

        file = (FS_FILE*)malloc(sizeof(FS_FILE));
        if (!file) {
            printf("Error: Memory allocation failed for FS_FILE.\n");
            fflush(stdout);
            return NULL;
        }

        file->entry = entry;
        file->position = (strcmp(mode, "a") == 0) ? entry->size : 0;
        file->mode = mode[0];
    } else {
        printf("Error: Invalid mode '%s'.\n", mode);
        fflush(stdout);
        return NULL;
    }

    return file;


} 
// int fs_write(FS_FILE* file, const void* buffer, int size) {
//     if (file == NULL || buffer == NULL || size < 0) {
//         printf("Error: Invalid input parameters.\n");
//         return -1;
//     }

//     if (file->mode != 'a' && file->mode != 'w') {
//         printf("Error: File not open in a writable or appendable mode.\n");
//         return -1;
//     }

//     uint32_t currentBlock = file->entry->start_block;
//     uint32_t currentBlockPosition = file->position % FILESYSTEM_BLOCK_SIZE;
//     const uint8_t* writeBuffer = (const uint8_t*) buffer;
//     int bytesWritten = 0;
//     bool isFirstBlock = (file->position == 0);  // Identify if we're starting at the beginning of the file.

//     while (size > 0) {
//         printf("size FATTTT: %d\n", size); 
//         // Check if a new block is needed
//         if (currentBlockPosition == FILESYSTEM_BLOCK_SIZE || currentBlock == FAT_ENTRY_END) {
//             printf("Allocating new block for file '%s'.\n", file->entry->filename);
//             uint32_t newBlock = fat_allocate_block();
//             if (newBlock == FAT_NO_FREE_BLOCKS) {
//                 printf("Error: No free blocks available.\n");
//                 return -1;
//             }
//             if (currentBlock != FAT_ENTRY_END) {
//                 printf("Linking block %u to new block %u.\n", currentBlock, newBlock);
//                 fat_link_blocks(currentBlock, newBlock);
//             } else if (isFirstBlock) {
//                 printf("Setting start block of file to %u.\n", newBlock);
//                 file->entry->start_block = newBlock;  // Only set start_block if it's the first block being written to a new file.
//                 isFirstBlock = false;
//             }
//             currentBlock = newBlock;
//             currentBlockPosition = 0;
//         }

//         // Calculate writable space in the current block
//         int remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
//         int toWrite = MIN(remainingSpaceInBlock, size);

//         if (isFirstBlock) {
//             // Adjust for the FileEntry metadata space if it's the very first write operation of the file
//             currentBlockPosition += sizeof(FileEntry);
//             remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
//             toWrite = MIN(remainingSpaceInBlock, size);
//             isFirstBlock = false;  // Ensure this adjustment only happens once
//         }

//         uint32_t writeOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;
//         printf("Writing %d bytes to block %u at offset %u.\n", toWrite, currentBlock, writeOffset);
//         flash_write_safe2(writeOffset, writeBuffer, toWrite);

//         writeBuffer += toWrite;
//         bytesWritten += toWrite;
//         size -= toWrite;
//         file->position += toWrite;
//         currentBlockPosition += toWrite;

//         // Check if we've written to the end of the block
//         if (currentBlockPosition >= FILESYSTEM_BLOCK_SIZE) {
//             currentBlockPosition = 0; // Reset for next block
//             currentBlock = FAT_ENTRY_END; // Force allocation of new block on next iteration
//         }
//     }

//     printf("Total %d bytes written.\n", bytesWritten);
//     return bytesWritten;
// }



int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid input parameters.\n");
        return -1;
    }

    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1;
    }

    uint32_t currentBlock = file->entry->start_block;
    uint32_t currentBlockLINK = currentBlock;;
    uint32_t currentBlockPosition = file->position % FILESYSTEM_BLOCK_SIZE;
    const uint8_t* writeBuffer = (const uint8_t*) buffer;
    int bytesWritten = 0;
    bool isFirstBlock = (file->position == 0);  // Identify if we're starting at the beginning of the file.

    while (size > 0) {
        printf("size FATTTT: %d\n", size); 
        // Check if a new block is needed
        if (currentBlockPosition == FILESYSTEM_BLOCK_SIZE || currentBlock == FAT_ENTRY_END) {
            printf("Allocating new block for file '%s'.\n", file->entry->filename);
            uint32_t newBlock = fat_allocate_block();
            printf("newBlock: %u\n", newBlock);
            printf("currentBlockLINK: %u\n", currentBlockLINK);
            if (newBlock == FAT_NO_FREE_BLOCKS) {
                printf("Error: No free blocks available.\n");
                return -1;
            }//FAT_ENTRY_END

                printf("Linking block %u to new block %u.\n", currentBlock, newBlock);
                fat_link_blocks(currentBlockLINK, newBlock);
             if (isFirstBlock) {
                printf("Setting start block of file to %u.\n", newBlock);
                file->entry->start_block = newBlock;  // Only set start_block if it's the first block being written to a new file.
                isFirstBlock = false;
            }
            currentBlock = newBlock;
            currentBlockPosition = 0;
        }
        currentBlockLINK = currentBlock;
        // Calculate writable space in the current block
        int remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
        int toWrite = MIN(remainingSpaceInBlock, size);

        if (isFirstBlock) {
            // Adjust for the FileEntry metadata space if it's the very first write operation of the file
            // currentBlockPosition += sizeof(FileEntry);
            remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
            toWrite = MIN(remainingSpaceInBlock, size);
            isFirstBlock = false;  // Ensure this adjustment only happens once
        }

        uint32_t writeOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;
        printf("Writing %d bytes to block %u at offset %u.\n", toWrite, currentBlock, writeOffset);
        flash_write_safe2(writeOffset, writeBuffer, toWrite);

        writeBuffer += toWrite;
        bytesWritten += toWrite;
        size -= toWrite;
        file->position += toWrite;
        currentBlockPosition += toWrite;

        // Check if we've written to the end of the block
        if (currentBlockPosition >= FILESYSTEM_BLOCK_SIZE) {
            currentBlockPosition = 0; // Reset for next block
            currentBlock = FAT_ENTRY_END; // Force allocation of new block on next iteration
        }
    }

    printf("Total %d bytes written.\n", bytesWritten);
    return bytesWritten;
}




/**
 * Closes the specified file.
 * 
 * This function handles the cleanup and release of resources associated with an open file.
 * It is responsible for freeing the memory allocated to the FS_FILE structure.
 *
 * @param file A pointer to the FS_FILE structure representing the file to be closed.
 */
void fs_close(FS_FILE* file) {
    // Check if the file pointer is valid before attempting to close.
    if (file == NULL) {
        // Print an error message and exit the function if the file pointer is NULL.
        printf("Error: Attempted to close a NULL file pointer.\n");
        fflush(stdout);
        return;
    }

    // Free the memory allocated for the FS_FILE structure.
    // This is important to prevent memory leaks.
    free(file);
}


 /**
 * Reads data from the specified file into the provided buffer.
 *
 * This function attempts to read up to 'size' bytes from the file associated with 'file' into 'buffer'.
 * It handles various edge cases such as null pointers, invalid read sizes, and incorrect file modes.
 *
 * @param file   A pointer to the FS_FILE structure representing the file to read from.
 * @param buffer A pointer to the buffer where the data will be stored.
 * @param size   The maximum number of bytes to read.
 * @return The number of bytes actually read, or a negative error code if an error occurred.
 */
// int fs_read(FS_FILE* file, void* buffer, int size) {
//     // Validate the file and buffer pointers to ensure they are not NULL.
//     if (file == NULL || buffer == NULL) {
//         printf("Error: Null file or buffer pointer provided.\n");
//         return -1;  // Use a defined error code in practice.
//     }

//     // Validate the request size to ensure it's positive.
//     if (size <= 0) {
//         printf("Error: Invalid size to read (%d).\n", size);
//         return -1;  // Use a defined error code in practice.
//     }

//     // Ensure the file is opened in a mode that allows reading.
//     if (file->mode != 'r' && file->mode != 'a') {
//         printf("Error: File is not open in a readable or append mode.\n");
//         return -1;  // Use a defined error code in practice.
//     }

//     // Calculate the number of bytes that can actually be read, which may be less than requested.
//     // int bytes_to_read = min(size, file->entry->size - file->position);
//     // if (bytes_to_read <= 0) {
//     //     // If there is no data left to read, return zero.
//     //     printf("No more data to read from file.\n");
//     //     return 0;    
//     // }
//     int bytes_to_read = 20;

//     // Perform the actual data copying from the file's internal buffer to the provided buffer.
//     // memcpy(buffer, file->entry->buffer + file->position, bytes_to_read);
//     uint32_t readOffset = (file->entry->start_block * FILESYSTEM_BLOCK_SIZE);
//     printf("Read offset: %u\n", readOffset);
//     printf("sizeof(buffer): %d\n", sizeof(buffer)); 

//     flash_read_safe2(readOffset, (uint8_t*)buffer, size);
//     // Update the file's current position.
//     file->position += bytes_to_read;
//     printf("buffer: %s\n", buffer); 

//     // Provide feedback on how many bytes were read.
//     printf("Read %d bytes from file '%s'.\n", bytes_to_read, file->entry->filename);

//     // Return the number of bytes read.
//     return bytes_to_read;
// }


// int fs_read(FS_FILE* file, void* buffer, int size) {
//     if (file == NULL || buffer == NULL || size <= 0) {
//         printf("Error: Invalid parameters provided.\n");
//         return -1;
//     }

//     if (file->mode != 'r' && file->mode != 'a') {
//         printf("Error: File is not open in a readable mode.\n");
//         return -1;
//     }

//     printf("Reading %d bytes from file '%s'.\n", size, file->entry->filename);
//     printf("File size: %d\n", file->entry->size);
//     printf("File position: %d\n", file->position);
//     printf("File start block: %u\n", file->entry->start_block);
//     printf("File unique ID: %u\n", file->entry->unique_file_id);
//     printf("File parent directory ID: %u\n", file->entry->parentDirId);
//     printf("File is directory: %d\n", file->entry->is_directory);
//     printf("File in use: %d\n", file->entry->in_use);
//     printf("File mode: %c\n", file->mode);
 
//     fflush(stdout);


//     uint8_t* readBuffer = (uint8_t*)buffer;
//     int bytesRead = 0;
//     uint32_t currentBlock = file->entry->start_block;
//     printf("\nCurrent block: %u\n", currentBlock);
//     uint32_t blockOffset = file->position % FILESYSTEM_BLOCK_SIZE; // Calculate initial offset within the block

//     while (bytesRead < size && currentBlock != FAT_ENTRY_END) {
//         uint32_t readOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + blockOffset; // Calculate the physical offset to read from
//         int spaceInBlock = FILESYSTEM_BLOCK_SIZE - blockOffset; // Remaining space in the current block
//         int bytesToRead = MIN(spaceInBlock, size - bytesRead); // Determine the number of bytes to read from the current block

//         flash_read_safe2(readOffset, readBuffer + bytesRead, bytesToRead);

//         bytesRead += bytesToRead;
//         file->position += bytesToRead; // Update the file position

//         // Check if we need to move to the next block
//         if (bytesToRead == spaceInBlock && bytesRead < size) {
//             uint32_t nextBlock;
//             if (fat_get_next_block(currentBlock, &nextBlock) != FAT_SUCCESS) {
//                 printf("Failed to get next block from block %u.\n", currentBlock);
//                 break;
//             }
//             currentBlock = nextBlock;
//             blockOffset = 0; // Reset offset for new block
//         } else {
//             blockOffset += bytesToRead;
//         }
//     }

//     printf("Read %d bytes from file '%s'.\n", bytesRead, file->entry->filename);
//     return bytesRead;
// }

int fs_read(FS_FILE* file, void* buffer, int size) {
    if (file == NULL || buffer == NULL) {
        printf("Error: Null file or buffer pointer provided.\n");
        return -1;
    }

    if (size <= 0 || file->mode != 'r' && file->mode != 'a') {
        printf("Error: Invalid read request.\n");
        return -1;
    }


        printf("Reading %d bytes from file '%s'.\n", size, file->entry->filename);
    printf("File size: %d\n", file->entry->size);
    printf("File position: %d\n", file->position);
    printf("File start block: %u\n", file->entry->start_block);
    printf("File unique ID: %u\n", file->entry->unique_file_id);
    printf("File parent directory ID: %u\n", file->entry->parentDirId);
    printf("File is directory: %d\n", file->entry->is_directory);
    printf("File in use: %d\n", file->entry->in_use);
    printf("File mode: %c\n", file->mode);

    uint32_t currentBlock = file->entry->start_block;
    uint32_t currentBlockPosition = file->position % FILESYSTEM_BLOCK_SIZE;
    printf("Current block: %u\n", currentBlock);
    uint8_t* readBuffer = (uint8_t*)buffer;
    int totalBytesRead = 0;
    int remainingSize = size;

    while (remainingSize > 0 && currentBlock != FAT_ENTRY_END) {
        int bytesToRead = MIN(FILESYSTEM_BLOCK_SIZE - currentBlockPosition, remainingSize);
        uint32_t readOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;

        flash_read_safe2(readOffset, readBuffer, bytesToRead);

        readBuffer += bytesToRead;
        totalBytesRead += bytesToRead;
        remainingSize -= bytesToRead;
        file->position += bytesToRead;
    printf("Successfully read from flash, total bytes read: %d, remaining size: %d\n", totalBytesRead, remainingSize);
        // Update position within block and check if we need to move to next block
        currentBlockPosition = (file->position % FILESYSTEM_BLOCK_SIZE);
        if (currentBlockPosition == 0 && remainingSize > 0) {  // We're at the end of a block
            uint32_t nextBlock;
            if (fat_get_next_block(currentBlock, &nextBlock) == FAT_SUCCESS && nextBlock != FAT_ENTRY_END) {
                    printf("Transitioning from block %u to next block %u\n", currentBlock, nextBlock);
                currentBlock = nextBlock;
            } else {
                    printf("End of file chain reached or no next block available. Current block: %u, \n", currentBlock);
                break;  // No more blocks to read or end of chain reached
            }
        }
    }

    printf("Read %d bytes from file '%s'.\n", totalBytesRead, file->entry->filename);
    return totalBytesRead;
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
}
 


 



/**
 * Copies a file from the source path to the destination path, ensuring not to overwrite existing files
 * in the destination by appending "Copy" to the file name if necessary.
 *
 * @param source_path The path to the source file.
 * @param dest_path The path to the destination where the file should be copied.
 * @return Returns 0 on success, -1 on error.
 */




int fs_cp(const char* source_path, const char* dest_path) {
    // Extract the directory and filename parts from the source and destination paths.
    PathParts old_path = extract_last_two_parts(source_path);
    PathParts new_path = extract_last_two_parts(dest_path);

    char* source_filename= old_path.filename;
    char* source_directory_path= old_path.directory;
    char* dest_directory_path= new_path.directory;
    char* dest_filename= new_path.filename;
    
    strcpy(dest_filename, source_filename);

    // check if the  directories are empty, if so set it to root
    set_default_path(dest_directory_path, "/root");
    set_default_path(source_directory_path, "/root");
     

    DirectoryEntry* directory = DIR_find_directory_entry(source_directory_path);
    if (directory == NULL) {
    printf("Error: Source directory '%s' does not exist.\n", source_directory_path);
    return -1;
    }
     uint32_t source_directory_parentDirId = directory->currentDirId;

    // Validate the modified source filename.
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", source_path);
        return -1;
    }
    // Find the file entry by name in the filesystem to get the unique file ID.

    FileEntry* entry = FILE_find_file_entry(source_filename, source_directory_parentDirId);

    if (entry == NULL) {
        printf("Error: File '%s' not found.\n", source_filename);
        return -1;
    }
    
    // Lookup the directory entry for the destination path. in order to retrieve the parent directory ID.
    // This way we can ensure the file is copied to a valid directory with a valid parent directory ID.
    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path); // Store parent directory ID for later use.
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }
    uint32_t dest_directory_parentDirId = destDirEntry->currentDirId;
    printf("Parent ID: %u\n", dest_directory_parentDirId);
   

    int check = find_file_existance(dest_filename, dest_directory_parentDirId);
    if (check == 0) {
        printf("File name already exists in the destination directory. add Copy extension \n");
        appendCopyToFilename(dest_filename);
        int checkDEST = find_file_existance(dest_filename, dest_directory_parentDirId);
        if (checkDEST == 0) {
            printf("ERROR File copy already exists in the destination directory. with name:%s \n", dest_filename);
            return -1;
        }
    }else{
        strcpy(dest_filename, source_filename);
    }

    char dest_full_path[256], source_full_path[256];
    construct_full_path(dest_directory_path, dest_filename, dest_full_path, sizeof(dest_full_path));
    construct_full_path(source_directory_path, source_filename, source_full_path, sizeof(source_full_path));



    // Create a new file entry for the destination file.  
    FS_FILE* fileCopy = fs_open(dest_full_path, "w"); // this will be the coppy file
        if (fileCopy == NULL) {
            printf("Error: Failed to open file '%s' for copying.\n", dest_filename);
            return -1;
        }

    // now open the previous file to read the contents
    FS_FILE* oldfile = fs_open(source_full_path, "r");

    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }

     // Corrected to use strlen for string copying
    fileCopy->entry->size = oldfile->entry->size;

    // memcpy(fileCopy->entry->buffer, oldfile->entry->buffer, sizeof(oldfile->entry->buffer));

    // calculate the offset of the file in the flash memory
    uint32_t writeOffset = (fileCopy->entry->start_block * FILESYSTEM_BLOCK_SIZE);
    flash_write_safe2(writeOffset, (const uint8_t*)&fileCopy->entry, sizeof(fileCopy->entry)); 

    // Clean up open file handles.
    fs_close(oldfile);
    fs_close(fileCopy);

   return 0;

}
 

 int fs_mv(const char* old_path, const char* new_path){


    PathParts old_paths = extract_last_two_parts(old_path);
    PathParts new_paths = extract_last_two_parts(new_path);


    char* source_filename = old_paths.filename;
    char* dest_directory_path= new_paths.directory;
    char* dest_filename= new_paths.filename;


    
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", old_path);
        return -1;
    }


    if (dest_directory_path[0] == '\0') {
        strcpy(dest_directory_path, "/root");  // Set default path if empty
    } 



    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path);
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }
    sleep_ms(1000);
    uint32_t parentID = destDirEntry->currentDirId;

    // now open to get the unique id of the file that we want to copy
    FS_FILE* oldfile = fs_open(source_filename, "r");

    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }
    uint32_t uniqueIdFile = oldfile->entry->unique_file_id;

     int fileIndex = find_file_entry_by_unique_file_id(uniqueIdFile);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", source_filename);
        return -1;
    }
    fileSystem[fileIndex].parentDirId = parentID; /// this one we will change 
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));
    printf("Data written to file.\n");
    printf("File '%s' successfully copied to '%s'.\n", source_filename, dest_directory_path);
    
   return 0;
}




// int fs_rm(const char* path) {
//     // Validate the input path
//     if (!path) {
//         printf("Error: Path is NULL.\n");
//         fflush(stdout);
//         return -1; // Invalid arguments
//     }

//     // Ensure the file exists
//     FileEntry* fileEntry = FILE_find_file_entry(path);
//     if (!fileEntry) {
//         printf("Error: File '%s' not found.\n", path);
//         fflush(stdout);
//         return -2; // File not found
//     }

//     // Prevent deletion if the target is a directory (if applicable)
//     if (fileEntry->is_directory) {
//         printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
//         fflush(stdout);
//         return -3; // Attempt to remove a directory with a file removal function
//     }

//     // Free all blocks associated with this file
//     uint32_t currentBlock = fileEntry->start_block;
//     uint32_t nextBlock;
//     int result;

//     while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
//         result = fat_get_next_block(currentBlock, &nextBlock);
//         if (result != FAT_SUCCESS) {
//             printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
//             fflush(stdout);
//             // Decide how to handle error: break out of the loop
//             break; 
//         }

//         fat_free_block(currentBlock);

//         // Check if the end of the chain is reached or an invalid block index is encountered
//         if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
//             break; // Properly end loop
//         }

//         currentBlock = nextBlock;
//     }
//     // Mark the file entry as not in use
//     memset(fileEntry, 0, sizeof(FileEntry));
//     fileEntry->in_use = false;

//     printf("File '%s' successfully removed.\n", path);

//     // find the file in the fileSystem array and update the in-use flag to false
//     int fileIndex = find_file_entry_by_name(path);
//     if (fileIndex == -1) {
//         printf("Error: File '%s' not found for reading.\n", path);
//         return -1;
//     }
//     fileSystem[fileIndex].in_use = false;


//     uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

//     flash_write_safe2(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));

    
//     fflush(stdout);
//     return 0; // Success
// }




// int fs_wipe(const char* path){
//     // Validate the input path
//     if (!path) {
//         printf("Error: Path is NULL.\n");
//         fflush(stdout);
//         return -1; // Invalid arguments
//     }

//     // Ensure the file exists
//     FileEntry* fileEntry = FILE_find_file_entry(path );
//     if (!fileEntry) {
//         printf("Error: File '%s' not found.\n", path);
//         fflush(stdout);
//         return -2; // File not found
//     }

//     // Prevent deletion if the target is a directory (if applicable)
//     if (fileEntry->is_directory) {
//         printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
//         fflush(stdout);
//         return -3; // Attempt to remove a directory with a file removal function
//     }
//     // Free all blocks associated with this file
//     uint32_t currentBlock = fileEntry->start_block;
//     uint32_t nextBlock;
//     int result;

//     while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
//         result = fat_get_next_block(currentBlock, &nextBlock);
//         if (result != FAT_SUCCESS) {
//             printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
//             fflush(stdout);
//             // Decide how to handle error: break out of the loop
//             break; 
//         }

//         fat_free_block(currentBlock);

//         // Check if the end of the chain is reached or an invalid block index is encountered
//         if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
//             break; // Properly end loop
//         }

//         currentBlock = nextBlock;
//     }
   

//     // find the file in the fileSystem array and update the in-use flag to false
//     int fileIndex = find_file_entry_by_name(path);
//     if (fileIndex == -1) {
//         printf("Error: File '%s' not found for reading.\n", path);
//         return -1;
//     }
//      uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

//     flash_erase_safe2(writeOffset);

//     memset(&fileSystem[fileIndex], 0, sizeof(FileEntry));  // Set the entire entry to zero
    
//     fflush(stdout);
//     return 0; // Success
// }


