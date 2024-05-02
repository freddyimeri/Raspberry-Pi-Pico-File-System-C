/**
 * @file filesystem.c
 * @brief Filesystem implementation for microcontroller environments.
 *
 * This module provides a robust filesystem designed for embedded systems, specifically
 * optimized for microcontrollers such as the Raspberry Pi Pico. The filesystem supports
 * essential operations such as file opening, reading, writing, and closing, along with
 * advanced features like directory management and file security functions.
 *
 * Key Features:
 *  - File operations: open, read, write, close. remove, move, copy, and wipe.
 *  - Thread-safety across file and directory operations using mutexes.
 *  - Error management to maintain data integrity and system stability.
 *
 * The implementation leverages a simple yet effective FAT-like system for block management,
 * ensuring efficient use of flash memory. This file includes integration with lower-level
 * hardware abstraction layers that manage direct interactions with the flash memory.
 *
 * @author Alfret Imeri
 * 
 */



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
    // Initialize the FAT table or similar structures needed for managing file allocations.
    fat_init();

    // Initialize a mutex to control access to the filesystem, ensuring thread safety.
    mutex_init(&filesystem_mutex);

    // Mark the filesystem as initialized to prevent reinitialization.
    fs_initialized = true;

    // Initialize any directory entries, setting up the directory structure of the filesystem.
    init_directory_entries();

    // Initialize all file entries, setting them to a default state indicating they are not in use.
    init_file_entries();

    // Start address for file blocks in the flash memory, defined in flash_config.h or similar.
    uint32_t current_start_block = FLASH_TARGET_OFFSET;

    // Loop through each file entry and initialize its start block in the flash storage.
    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false; // Initially, no file is in use.
        fileSystem[i].size = 0;       // Initialize the file size to 0.

        // Set the start block for each file.
        fileSystem[i].start_block = current_start_block;

        // Increment the start block by the maximum file size to allocate space for each file.
        current_start_block += MAX_FILE_SIZE;
         
        // Display the start block address for debugging.
        printf("Current start block: %u\n", current_start_block);
        fflush(stdout);

        // Check if the current start block exceeds the defined usable flash space.
        if (current_start_block >= FLASH_TARGET_OFFSET + FLASH_USABLE_SPACE) {
            // If there isn't enough flash memory for the files, handle it as an error.
            printf("Error: Not enough flash memory for the number of files.\n");
            fflush(stdout);
            return; // Exit if there is not enough memory to avoid further errors.
        }
    }

    // Reset the root directory to ensure it is clear and ready for new entries.
    int resetSuccess = reset_root_directory();
    if (!resetSuccess) {
        // Handle any errors in resetting the root directory.
        printf("Critical error initializing root directory.\n");
        fs_initialized = false; // Mark filesystem as not initialized due to error.
        return; // Exit the function to prevent further operations.
    }
    
    // If all initializations are successful, confirm the filesystem is ready.
    fs_initialized = true;
    printf("Filesystem initialized.\n");
}



/**
 * Performs a clean shutdown of the filesystem by ensuring that all crucial
 * filesystem data structures are saved to non-volatile storage. This function
 * is designed to be called during system shutdown or when a safe state needs
 * to be ensured.
 */
void shutdown() {
    printf("Initiating shutdown process...\n");

    // Save all file entries to non-volatile storage to ensure no data loss.
    printf("Saving file entries...\n");
    saveFileEntriesToFileSystem();

    // Save all directory entries to ensure the directory structure is preserved.
    printf("Saving directory entries...\n");
    saveDirectoriesEntriesToFileSystem();

    // Save the current state of the File Allocation Table to preserve file system integrity.
    printf("Saving FAT entries...\n");
    saveFATEntriesToFileSystem();

    // Add any additional clean-up or save routines here.
    printf("Shutdown process complete. Safe to power off or restart.\n");
}


/**
 * Initializes all file entries in the file system to default, unused states.
 * This function is typically called at the start of the program to prepare the file system.
 */
void init_file_entries() {
    // Loop through each file entry in the file system array.
    for (int i = 0; i < MAX_FILES; i++) {
        fileSystem[i].in_use = false;  // Mark the file as not currently in use.
        fileSystem[i].is_directory = false;  // Indicate that this entry is not a directory.
        fileSystem[i].filename[0] = '\0';  // Set the filename to an empty string to denote an unused entry.
        fileSystem[i].parentDirId = 0;  // Reset the parent directory ID to 0, indicating no parent.
        fileSystem[i].size = 0;  // Set the size of the file to 0, as it is unused.
        fileSystem[i].start_block = 0;  // Set the start block to 0, indicating no data blocks are assigned.
        fileSystem[i].unique_file_id = 0;  // Reset the unique file ID to 0.
    }
}


/**
 * Opens a file based on a specified path and mode.
 * 
 * @param FullPath The complete path of the file to open.
 * @param mode The mode in which to open the file ('r' for read, 'w' for write, 'a' for append).
 * @return A pointer to an FS_FILE structure representing the opened file, or NULL if an error occurs.
 */
FS_FILE* fs_open(const char* FullPath, const char* mode) {
    // Extract the last two components of the path: directory and filename
    PathParts pathExtract = extract_last_two_parts(FullPath);
    char* filename = pathExtract.filename;
    char* directory_path = pathExtract.directory;

    // Set the default directory path to "/root" if not specified
    // if it was specified, it will not be updated to "/root"
    set_default_path(directory_path, "/root");

    // Attempt to find the directory in the filesystem
    DirectoryEntry* directory = DIR_find_directory_entry(directory_path);
    if (!directory) {
        // If the directory is not found, output an error and return NULL
        printf("Error: Directory '%s' not found.\n", directory_path);
        fflush(stdout);
        return NULL;
    }
    // Store the current directory ID from the directory entry
    uint32_t parentDirId = directory->currentDirId;
    // Check if the filename part is empty, which is not allowed
    if (filename[0] == '\0') {
        printf("Error: Path '%s' does not contain a valid file name.\n", FullPath);
        fflush(stdout);
        return NULL;
    }

    FS_FILE* file = NULL;
    FileEntry* entry = NULL;
    // Check if the mode is one of the allowed modes ('r', 'w', 'a')
    if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0 || strcmp(mode, "r") == 0) {
        // Conditionally create a new file entry for 'w' mode or find an existing one for 'r' and 'a' modes
        entry = (strcmp(mode, "w") == 0) ? createFileEntry(filename, parentDirId) : FILE_find_file_entry(filename, parentDirId);
        if (!entry) {
            // If no entry is found or cannot be created, return NULL
            printf("Error: File '%s' not found or cannot be created.\n", filename);
            fflush(stdout);
            return NULL;
        }
        // Allocate memory for the FS_FILE structure
        file = (FS_FILE*)malloc(sizeof(FS_FILE));
        if (!file) {
            // If memory allocation fails, output an error and return NULL
            printf("Error: Memory allocation failed for FS_FILE.\n");
            fflush(stdout);
            return NULL;
        }
        // Initialize the file structure with the found or created entry
        file->entry = entry;
        // Set the initial position in the file. For append mode, set to the file size; for others, set to 0
        file->position = (strcmp(mode, "a") == 0) ? entry->size : 0;
        // Store the mode as a single character ('r', 'w', 'a')
        file->mode = mode[0];
    } else {
        // If the mode string is not recognized, output an error and return NULL
        printf("Error: Invalid mode '%s'.\n", mode);
        fflush(stdout);
        return NULL;
    }

    return file; // Return the pointer to the newly created FS_FILE structure


}  




/**
 * Writes data to an open file.
 * 
 * @param file Pointer to the FS_FILE structure representing the open file.
 * @param buffer Pointer to the data to be written.
 * @param size The number of bytes to write.
 * @return The number of bytes written, or -1 if an error occurs.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    // Validate input parameters to ensure they are correct
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid input parameters.\n");
        return -1;
    }

    // Validate input parameters to ensure they are correct
    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1;
    }
    // Initialize tracking variables for the current block and position within the block
    uint32_t currentBlock = file->entry->start_block;
    uint32_t currentBlockLINK = currentBlock;;
    uint32_t currentBlockPosition = file->position % FILESYSTEM_BLOCK_SIZE;
    const uint8_t* writeBuffer = (const uint8_t*) buffer;
    int bytesWritten = 0;
    bool isFirstBlock = (file->position == 0);  // Identify if we're starting at the beginning of the file.

    while (size > 0) {
        // Initialize tracking variables for the current block and position within the block
        if (currentBlockPosition == FILESYSTEM_BLOCK_SIZE || currentBlock == FAT_ENTRY_END) {
            uint32_t newBlock = fat_allocate_block();
            if (newBlock == FAT_NO_FREE_BLOCKS) {
                printf("Error RUN OUT FROM MEMORY: No free blocks available. \n");
                return -1;
            }//FAT_ENTRY_END
                // Link the current block to the new block in the FAT
                fat_link_blocks(currentBlockLINK, newBlock);
             if (isFirstBlock) {
                file->entry->start_block = newBlock;  // Only set start_block if it's the first block being written to a new file.
                isFirstBlock = false;
            }
            currentBlock = newBlock;
            // Calculate the amount of writable space in the current block
            currentBlockPosition = 0;
        }
        currentBlockLINK = currentBlock;
        // Calculate writable space in the current block
        int remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
        int toWrite = MIN(remainingSpaceInBlock, size);
        

        if (isFirstBlock) {
            // Adjust for the FileEntry metadata space if it's the very first write operation of the file          
            remainingSpaceInBlock = FILESYSTEM_BLOCK_SIZE - currentBlockPosition;
            toWrite = MIN(remainingSpaceInBlock, size);
            isFirstBlock = false;  // Ensure this adjustment only happens once
        }
        
        uint32_t writeOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;
        printf("Write offset: %u\n", writeOffset);
        flash_write_safe(writeOffset, writeBuffer, toWrite);

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
 * Reads data from an open file into a buffer.
 * 
 * @param file Pointer to the FS_FILE structure representing the open file.
 * @param buffer Pointer to the buffer where the read data should be stored.
 * @param size The number of bytes to read.
 * @return The number of bytes actually read, or -1 on error.
 */
int fs_read(FS_FILE* file, void* buffer, int size) {
    // Check for NULL pointers to ensure the file and buffer are valid.
    if (file == NULL || buffer == NULL) {
        printf("Error: Null file or buffer pointer provided.\n");
        return -1; // Return -1 to indicate an error due to invalid input.
    }

    // Validate the requested size and the file mode (must be either 'r' for read or 'a' for append).
    if (size <= 0 || (file->mode != 'r' && file->mode != 'a')) {
        printf("Error: Invalid read request.\n");
        return -1; // Return -1 to indicate an error due to invalid size or inappropriate file mode.
    }

    // Initialize variables to track the current block and the current position within that block.
    uint32_t currentBlock = file->entry->start_block;
    uint32_t currentBlockPosition = file->position % FILESYSTEM_BLOCK_SIZE;
    uint8_t* readBuffer = (uint8_t*)buffer; // Cast buffer to uint8_t* for byte-level operations.
    int totalBytesRead = 0; // Track the total number of bytes successfully read.
    int remainingSize = size; // Track the remaining number of bytes to read.

    // Continue reading while there are bytes remaining and the current block is not the end of the file.
    while (remainingSize > 0 && currentBlock != FAT_ENTRY_END) {
        // Calculate the number of bytes to read in this iteration.
        int bytesToRead = MIN(FILESYSTEM_BLOCK_SIZE - currentBlockPosition, remainingSize);
        // Calculate the offset in flash where the current block's data starts.
        uint32_t readOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;

        // Perform the read operation from flash storage.
        flash_read_safe(readOffset, readBuffer, bytesToRead);

        // Update the buffer pointer, total bytes read, and remaining size.
        readBuffer += bytesToRead;
        totalBytesRead += bytesToRead;
        remainingSize -= bytesToRead;
        // Update the file position.
        file->position += bytesToRead;
        // Recalculate the current block position.
        currentBlockPosition = (file->position % FILESYSTEM_BLOCK_SIZE);

        // If the end of the block is reached and there are still bytes to read, move to the next block.
        if (currentBlockPosition == 0 && remainingSize > 0) {
            uint32_t nextBlock;
            // Fetch the next block from the FAT.
            if (fat_get_next_block(currentBlock, &nextBlock) == FAT_SUCCESS && nextBlock != FAT_ENTRY_END) {
                currentBlock = nextBlock;
            } else {
                printf("End of file chain reached or no next block available. Current block: %u, \n", currentBlock);
                break; // Break the loop if no more blocks are available or an error occurred.
            }
        }
    }
    return totalBytesRead; // Return the total number of bytes read.
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
    // Extract the directory and filename parts from both source and destination paths using custom function.
    PathParts old_path = extract_last_two_parts(source_path);
    PathParts new_path = extract_last_two_parts(dest_path);

    // Store filenames and directory paths from the extracted path parts.
    char* source_filename = old_path.filename;
    char* source_directory_path = old_path.directory;
    char* dest_directory_path = new_path.directory;
    char* dest_filename = new_path.filename;
    
    // Initially set destination filename to be the same as the source filename.
    strcpy(dest_filename, source_filename);

    // Set the default directory path to "/root" if no specific directory is provided in the paths.
    set_default_path(dest_directory_path, "/root");
    set_default_path(source_directory_path, "/root");
     
    // Look up the directory entry of the source to get its directory information.
    DirectoryEntry* directory = DIR_find_directory_entry(source_directory_path);
    if (directory == NULL) {
        // Return error if the source directory does not exist.
        printf("Error: Source directory '%s' does not exist.\n", source_directory_path);
        return -1;
    }
    // Store the parent directory ID from the source directory entry for later use.
    uint32_t source_directory_parentDirId = directory->currentDirId;

    // Validate the source filename extracted from the path.
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", source_path);
        return -1;
    }

    // Find the file entry in the source directory to get its details.
    FileEntry* entry = FILE_find_file_entry(source_filename, source_directory_parentDirId);
    if (entry == NULL) {
        // Return error if the file does not exist in the source directory.
        printf("Error: File '%s' not found.\n", source_filename);
        return -1;
    }
    
    // Look up the directory entry of the destination to get its directory information.
    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path);
    if (!destDirEntry) {
        // Return error if the destination directory does not exist.
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1;
    }
    // Store the parent directory ID from the destination directory entry for later use.
    uint32_t dest_directory_parentDirId = destDirEntry->currentDirId;

    // Check if the filename already exists in the destination directory.
    int check = find_file_existance(dest_filename, dest_directory_parentDirId);
    if (check == 0) {
        // If the filename exists, append "Copy" to the filename to avoid overwriting.
        printf("File name already exists in the destination directory. add Copy extension \n");
        appendCopyToFilename(dest_filename);
        // Check again if the modified filename with "Copy" also exists.
        int checkDEST = find_file_existance(dest_filename, dest_directory_parentDirId);
        if (checkDEST == 0) {
            // If even the modified filename exists, return error.
            printf("ERROR File copy already exists in the destination directory. with name:%s \n", dest_filename);
            return -1;
        }
    } else {
        // If the filename does not exist, keep the original filename.
        strcpy(dest_filename, source_filename);
    }

    // Construct the full paths for the source and destination files based on the directory and filename.
    char dest_full_path[256], source_full_path[256];
    construct_full_path(dest_directory_path, dest_filename, dest_full_path, sizeof(dest_full_path));
    construct_full_path(source_directory_path, source_filename, source_full_path, sizeof(source_full_path));

    // Open the destination file with write permission to create a new or overwrite an existing file.
    FS_FILE* fileCopy = fs_open(dest_full_path, "w");
    if (fileCopy == NULL) {
        // Return error if opening the file fails.
        printf("Error: Failed to open file '%s' for copying.\n", dest_filename);
        return -1;
    }

    // Open the source file with read permission to read the contents.
    FS_FILE* oldfile = fs_open(source_full_path, "r");
    if (oldfile == NULL) {
        // Return error if opening the file fails.
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1;
    }

    // Set the size and start block of the destination file to match those of the source file.
    fileCopy->entry->size = oldfile->entry->size;
    fileCopy->entry->start_block = oldfile->entry->start_block;

    // Close both file handles after copying is complete.
    fs_close(oldfile);
    fs_close(fileCopy);

    return 0;  // Return success after the file is successfully copied.
}

 
/**
 * Moves a file from one location to another within the filesystem.
 * 
 * @param old_path Path to the original file.
 * @param new_path New path for the file after moving.
 * @return Returns 0 on success, -1 on error.
 */
int fs_mv(const char* old_path, const char* new_path){
    // Extract the directory and filename parts from both the old and new paths.
    PathParts old_paths = extract_last_two_parts(old_path);
    PathParts new_paths = extract_last_two_parts(new_path);

    // Store extracted filename and directory path for both source and destination.
    char* source_filename = old_paths.filename;
    char* dest_directory_path = new_paths.directory;
    char* dest_filename = new_paths.filename;

    // Check if the source filename is empty, which indicates an invalid path.
    if (source_filename[0] == '\0') {
        printf("Error: Source path '%s' does not contain a valid file name.\n", old_path);
        return -1; // Return error code.
    }

    // Set the destination directory path to "/root" if it is empty, providing a default path.
    if (dest_directory_path[0] == '\0') {
        strcpy(dest_directory_path, "/root"); // Default path if empty.
    }

    // Lookup the directory entry for the destination path to ensure it exists.
    DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path);
    if (!destDirEntry) {
        printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
        return -1; // Return error if destination directory does not exist.
    }

    // Simulate a delay which may be used for sync purposes or to mimic I/O delay.
    sleep_ms(1000);

    // Get the unique directory ID from the destination directory entry.
    uint32_t parentID = destDirEntry->currentDirId;

    // Open the original file to access its details, including the unique file ID.
    FS_FILE* oldfile = fs_open(source_filename, "r");
    if (oldfile == NULL) {
        printf("Error: Failed to open file '%s' for reading.\n", source_filename);
        return -1; // Return error if file opening fails.
    }

    // Retrieve the unique file ID from the file entry.
    uint32_t uniqueIdFile = oldfile->entry->unique_file_id;

    // Find the index of the file entry using its unique ID.
    int fileIndex = find_file_entry_by_unique_file_id(uniqueIdFile);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", source_filename);
        return -1; // Return error if file not found.
    }

    // Update the parent directory ID in the file system to reflect the new location.
    fileSystem[fileIndex].parentDirId = parentID;

    // Calculate the write offset based on the start block of the file.
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    // Write the updated file entry back to the flash memory to finalize the move.
    flash_write_safe(writeOffset, (const uint8_t*)&fileSystem[fileIndex], sizeof(FileEntry));

    // Confirm the move operation has been completed successfully.
    printf("Data written to file.\n");
    printf("File '%s' successfully copied to '%s'.\n", source_filename, dest_directory_path);
    
    return 0; // Return success.
}




/**
 * Removes a file from the filesystem.
 * 
 * @param path The path of the file to be removed.
 * @return Returns 0 on success, negative values on error.
 */
int fs_rm(const char* path) {
    // First, check if the provided file path is NULL to ensure it is valid.
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Return error for invalid argument.
    }   

    // Extract the last two parts of the path which include the filename and its immediate directory.
    PathParts new_path = extract_last_two_parts(path);
    char* source_filename = new_path.filename;
    char* source_directory_path = new_path.directory;

    // Set the default path to "/root" if the extracted directory path is empty.
    set_default_path(source_directory_path, "/root");

    // Try to find the directory entry using the possibly updated directory path.
    DirectoryEntry* directory = DIR_find_directory_entry(source_directory_path);
    if (directory == NULL) {
        printf("Error: Source directory '%s' does not exist.\n", source_directory_path);
        return -1; // Return error if the directory does not exist.
    }
    uint32_t source_directory_parentDirId = directory->currentDirId;

    // Attempt to find the file entry within the identified directory using the provided path.
    FileEntry* fileEntry = FILE_find_file_entry(path, source_directory_parentDirId);
    if (!fileEntry) {
        printf("Error: File '%s' not found.\n", path);
        fflush(stdout);
        return -2; // Return error if the file does not exist.
    }

    // Check if the file entry is actually a directory, which cannot be removed using this function.
    if (fileEntry->is_directory) {
        printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
        fflush(stdout);
        return -3; // Return error specific to trying to remove a directory.
    }

    // Begin freeing all blocks associated with this file in the filesystem.
    uint32_t currentBlock = fileEntry->start_block;
    uint32_t nextBlock;
    int result;

    // Loop through all blocks assigned to the file.
    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
            fflush(stdout);
            break; // Break out of the loop on error.
        }

        // Free the current block and move to the next.
        fat_free_block(currentBlock);

        // If the next block is the end of the chain or invalid, break the loop.
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break; // End the loop properly if there are no more blocks to free.
        }

        currentBlock = nextBlock;
    }

    // Reset the file entry data to default values and mark it as not in use.
    memset(fileEntry, 0, sizeof(FileEntry));
    fileEntry->in_use = false;

    printf("File '%s' successfully removed.\n", path);

    // Locate the file in the filesystem array and update its status to not in use.
    int fileIndex = find_file_entry_by_name(path);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", path);
        return -1; // Return error if file index is not found.
    }
    fileSystem[fileIndex].in_use = false;

    fflush(stdout);
    return 0; // Return success indicating the file was successfully removed.
}





/**
 * Securely wipes a file from the filesystem, erasing its contents and freeing its blocks.
 *
 * @param path The path of the file to be wiped.
 * @return Returns 0 on success, or negative error codes on failure.
 */
int fs_wipe(const char* path){
    // Check if the provided file path is NULL, ensuring the path is valid before proceeding.
    if (!path) {
        printf("Error: Path is NULL.\n");
        fflush(stdout);
        return -1; // Return error indicating invalid arguments.
    }

    // Extract the directory and filename from the provided path to find the file in the filesystem.
    PathParts new_path = extract_last_two_parts(path);
    char* source_filename= new_path.filename;
    char* source_directory_path= new_path.directory;

    // Default to "/root" directory if no specific directory path is extracted.
    set_default_path(source_directory_path, "/root");
     
    // Attempt to locate the directory in the filesystem where the file should be located.
    DirectoryEntry* directory = DIR_find_directory_entry(source_directory_path);
    if (directory == NULL) {
        printf("Error: Source directory '%s' does not exist.\n", source_directory_path);
        return -1; // Return error if the directory does not exist.
    }
    uint32_t source_directory_parentDirId = directory->currentDirId;

    // Find the file entry within the identified directory.
    FileEntry* fileEntry = FILE_find_file_entry(path, source_directory_parentDirId);
    if (!fileEntry) {
        printf("Error: File '%s' not found.\n", path);
        fflush(stdout);
        return -2; // Return error if the file does not exist.
    }

    // Prevent the deletion if the file entry is actually a directory.
    if (fileEntry->is_directory) {
        printf("Error: '%s' is a directory, not a file. Use a different function to remove directories.\n", path);
        fflush(stdout);
        return -3; // Return error for attempting to remove a directory with a file removal function.
    }

    // Loop through all blocks assigned to the file, freeing each one.
    uint32_t currentBlock = fileEntry->start_block;
    uint32_t nextBlock;
    int result;

    while (currentBlock != FAT_ENTRY_END && currentBlock < TOTAL_BLOCKS) {
        result = fat_get_next_block(currentBlock, &nextBlock);
        if (result != FAT_SUCCESS) {
            printf("Error navigating FAT or encountered invalid next block index while trying to remove '%s'. Error code: %d\n", path, result);
            fflush(stdout);
            break; // Exit the loop on error.
        }

        // Free the current block and prepare to move to the next.
        fat_free_block(currentBlock);

        // Break the loop if there are no more blocks to free.
        if (nextBlock == FAT_ENTRY_END || nextBlock >= TOTAL_BLOCKS) {
            break;
        }

        currentBlock = nextBlock;
    }

    // Locate the file in the filesystem array and mark it as not in use.
    int fileIndex = find_file_entry_by_name(path);
    if (fileIndex == -1) {
        printf("Error: File '%s' not found for reading.\n", path);
        return -1; // Return error if the file index is not found.
    }
    uint32_t writeOffset = (fileSystem[fileIndex].start_block * FILESYSTEM_BLOCK_SIZE);

    // Erase the flash memory at the location of the file to securely wipe its data.
    flash_erase_safe(writeOffset);

    // Reset the file entry to zero, clearing all its properties.
    memset(&fileSystem[fileIndex], 0, sizeof(FileEntry));
    
    fflush(stdout);
    return 0; // Return success after the file has been securely wiped.
}



