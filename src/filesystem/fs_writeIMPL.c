 
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



#include "fat_fs.h"

#define BLOCK_SIZE 256  // Assuming a block size of 256 bytes for file data

// Helper function to handle block allocation and linking
uint32_t manage_blocks(FS_FILE* file, uint32_t* lastBlock, uint32_t* currentBlockPosition) {
    if (*lastBlock == FAT_ENTRY_END || *currentBlockPosition >= BLOCK_SIZE) {
        uint32_t newBlock = fat_allocate_block();
        if (newBlock == FAT_NO_FREE_BLOCKS) {
            printf("Error: No free blocks available.\n");
            return FAT_NO_FREE_BLOCKS;
        }
        if (*lastBlock != FAT_ENTRY_END) {
            fat_link_blocks(*lastBlock, newBlock);
        } else {
            file->entry->start_block = newBlock;  // Update start_block if it was FAT_ENTRY_END
        }
        *lastBlock = newBlock;
        *currentBlockPosition = 0;  // Reset position in the new block
    }
    return *lastBlock;
}

int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid arguments for fs_write.\n");
        return -1;
    }
    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1;
    }

    uint32_t currentBlock = file->entry->start_block;
    uint32_t lastBlock = currentBlock;
    int currentBlockPosition = file->position % BLOCK_SIZE;
    int bytesWritten = 0;

    const char* writeBuffer = (const char*)buffer;
    while (size > 0) {
        currentBlock = manage_blocks(file, &lastBlock, &currentBlockPosition);
        if (currentBlock == FAT_NO_FREE_BLOCKS) break;

        int spaceInCurrentBlock = BLOCK_SIZE - currentBlockPosition;
        int writeSize = (size < spaceInCurrentBlock) ? size : spaceInCurrentBlock;
        
        // Simulate writing to storage
        // Actual storage write code should go here. For now, we'll simulate:
        printf("Writing %d bytes to block %u at position %d.\n", writeSize, currentBlock, currentBlockPosition);
        
        // Update positions and size
        currentBlockPosition += writeSize;
        file->position += writeSize;
        size -= writeSize;
        bytesWritten += writeSize;
    }

    if (file->mode == 'a' || file->mode == 'w') {
        file->entry->size = max(file->entry->size, file->position);
    }

    // Update the entry in storage, assuming `start_block` points to the file's first block
    uint32_t writeOffset = file->entry->start_block * FILESYSTEM_BLOCK_SIZE;
    flash_write_safe2(writeOffset, (const uint8_t*)file->entry, sizeof(FileEntry));
    
    printf("fs_write: %d bytes written.\n", bytesWritten);
    return bytesWritten;
}


uint32_t ensure_space_and_get_block(FS_FILE *file) {
    uint32_t currentBlock = file->entry->start_block;
    uint32_t lastBlock = FAT_ENTRY_END;

    while (currentBlock != FAT_ENTRY_END && currentBlock != FAT_NO_FREE_BLOCKS) {
        lastBlock = currentBlock;
        currentBlock = FAT[currentBlock];  // Get the next block in the chain
    }

    if (currentBlock == FAT_ENTRY_END) {
        uint32_t newBlock = fat_allocate_block();
        if (newBlock == FAT_NO_FREE_BLOCKS) {
            return FAT_NO_FREE_BLOCKS; // No space left
        }
        if (lastBlock != FAT_ENTRY_END) {
            fat_link_blocks(lastBlock, newBlock);
        } else {
            file->entry->start_block = newBlock;  // First block of the file
        }
        return newBlock;
    }
    return currentBlock;
}

/*
Function fs_write(file, buffer, size):
    // Check for invalid input parameters
    If file is NULL or buffer is NULL or size is less than or equal to 0:
        Return -1 // Error code for invalid input

    // Ensure the file is opened in either append ('a') or write ('w') mode
    If file's mode is not 'a' and file's mode is not 'w':
        Return -1 // Error code for incorrect file mode

    // Initialize local variables for tracking the current block and data pointer
    Initialize blockIndex to file's start_block
    Initialize dataPtr to point to the start of the buffer
    Initialize remainingSize to size
    Initialize totalWritten to 0

    // If appending, find the last block of the file
    If file's mode is 'a' and blockIndex is not NO_FREE_BLOCKS:
        While blockIndex is not FAT_ENTRY_END:
            Load block from blockIndex
            If the next_block of the loaded block is not FAT_ENTRY_END:
                Update blockIndex to the next_block of the loaded block
            Else:
                Break from the loop // Found the last block

    // If writing, potentially deallocate old blocks and allocate a new start block
    Else if file's mode is 'w':
        Optionally deallocate all blocks associated with this file
        Allocate a new block and set file's start_block to this new block
        Update blockIndex to this new start block

    // Read the last or new block into 'block' (initial or after finding/appending)
    Load block from blockIndex
    Initialize startOffset to block's size (amount of data already in the block)

    // Loop to write all data from buffer to storage
    While remainingSize > 0:
        Calculate spaceInCurrentBlock as BLOCK_DATA_SIZE - startOffset
        Calculate toWrite as the minimum of spaceInCurrentBlock and remainingSize

        // Check if current block has space to write
        If toWrite <= 0:
            Allocate a new block and store its index in newBlockIndex
            Set the next_block of the current block to newBlockIndex
            Write the current block back to storage to update its next_block link
            Update blockIndex to newBlockIndex
            Reset startOffset to 0 for the new block
            Set block's size to 0 and next_block to FAT_ENTRY_END
            Recalculate spaceInCurrentBlock and toWrite for the new block

        // Copy data from buffer to block's data starting at the current offset
        Copy data from dataPtr to block's data at startOffset for toWrite bytes
        Increment block's size by toWrite
        Increment startOffset by toWrite
        Update dataPtr to point to the next part of the input buffer
        Decrease remainingSize by toWrite
        Increment totalWritten by toWrite

        // Write the updated block back to storage
        Write block to storage at blockIndex

    // Return the total number of bytes successfully written
    Return totalWritten
*/



/**
 * File Allocation Table (FAT) Management for Embedded Systems
 * ============================================================
 * 
 * Overview:
 * This file implements the core functionality for managing the File Allocation Table (FAT),
 * a classic filesystem architecture adapted for use in embedded systems like the Raspberry Pi Pico.
 * The FAT filesystem is renowned for its simplicity and efficiency in environments with limited
 * computational resources and storage capacities.
 *
 * Architecture Highlights:
 * - FAT Array: Central to the filesystem, the FAT array maps data blocks to files, enabling
 *   the tracking of file data across non-contiguous storage blocks.
 * - Thread Safety: Implements mutex-based synchronization to ensure that FAT operations are
 *   safe in a multitasking environment, preventing data corruption during concurrent access.
 * - Efficient Storage: Includes mechanisms for allocating, freeing, and linking blocks with
 *   minimal fragmentation, enhancing storage efficiency and access speed.
 * - Dynamic Allocation: Features an allocation strategy that prioritizes proximity to hint blocks,
 *   reducing access times and further minimizing fragmentation.
 * - Robust Error Handling: Incorporates comprehensive validation and error handling to maintain
 *   filesystem integrity under various operational scenarios.
 *
 * Functionality:
 * - Initialization (`fat_init`): Prepares the FAT for operation, marking all blocks as free and
 *   reserving system blocks as needed.
 * - Block Allocation (`fat_allocate_block`, `fat_allocate_nearest_block`): Allocates blocks for file storage,
 *   optionally near a hint block for optimized placement.
 * - Block Freeing (`fat_free_block`): Releases blocks back to the free pool when no longer needed.
 * - Block Linking (`fat_link_blocks`): Manages the chaining of blocks to accommodate file data that spans
 *   multiple blocks.
 * - Next Block Retrieval (`fat_get_next_block`): Retrieves the next block in a file's chain, supporting
 *   sequential file access.
 *
 * Designed for embedded systems with constrained resources, this FAT management system balances
 * performance and simplicity, making it suitable for a wide range of applications.
 *
 * Note: This implementation assumes a single partition and is tailored to the specific needs and
 * constraints of the Raspberry Pi Pico platform and similar microcontroller-based systems.
 */

#include "../FAT/fat_fs.h"            
#include <stdio.h>
#include "hardware/flash.h"   
#include "pico/mutex.h"
#include "pico/time.h"

#include "../filesystem/filesystem.h"  
#include "../config/flash_config.h"    


#define ALLOCATE_BLOCK_MAX_RETRIES 3 // Max attempts to allocate a block before giving up
#define ALLOCATE_BLOCK_RETRY_DELAY_MS 30 // Delay between allocation retries to allow for block freeing

// The FAT table itself, storing the state of each block in the filesystem
uint32_t FAT[TOTAL_BLOCKS]; 
static mutex_t fat_mutex; // Mutex for thread-safe access to the FAT

// Initializes the FAT system, setting up the filesystem state for use
void fat_init() {
    mutex_init(&fat_mutex); // Initialize the mutex for FAT access control
    mutex_enter_blocking(&fat_mutex); // Ensure exclusive access to the FAT

     // Set all blocks to 'free' state initially
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        FAT[i] = FAT_ENTRY_FREE;
    }

    // Reserve blocks as needed for system use or mark bad blocks
    for (uint32_t i = 0; i < NUMBER_OF_RESERVED_BLOCKS; i++) {
        FAT[i] = FAT_ENTRY_RESERVED;
    }


    mutex_exit(&fat_mutex); // Release the mutex after initializing the FAT

    // Log the successful initialization
    printf("FAT initialization complete. Total blocks: %u\n", TOTAL_BLOCKS);
    fflush(stdout);
}


uint32_t fat_allocate_block() {
    uint32_t block = FAT_NO_FREE_BLOCKS;  // Default to no free blocks.
    int retries = ALLOCATE_BLOCK_MAX_RETRIES;

    while (retries > 0) {
        mutex_enter_blocking(&fat_mutex); // Secure exclusive access to the FAT.

        // Search for a free block starting after reserved blocks.
        for (uint32_t i = NUMBER_OF_RESERVED_BLOCKS; i < TOTAL_BLOCKS; i++) {
            if (FAT[i] == FAT_ENTRY_FREE) {
                FAT[i] = FAT_ENTRY_END; // Mark found block as the end of a file chain.
                block = i;  // Record the block number.
                printf("Allocated block %u\n", block); // Diagnostic log
                fflush(stdout);
                break; // Exit the loop upon finding a free block.
            }
        }

        mutex_exit(&fat_mutex);  // Release the FAT.

        if (block != FAT_NO_FREE_BLOCKS) {
            break; // Success, exit the loop.
        } else {
            // Retry logic, including a delay to allow for potential block freeing.
            retries--;
            printf("Retrying allocation, retries left: %d\n", retries); // Diagnostic log
            fflush(stdout);
            if (retries > 0) {
                // Delay before retrying, if appropriate for the system.
                // This delay gives time for other threads to potentially free up blocks.
                sleep_ms(ALLOCATE_BLOCK_RETRY_DELAY_MS);
            }
        }
    }

    if (block == FAT_NO_FREE_BLOCKS && retries == 0) {
        // After all retries, no free block is available.
        // Handle error for no free blocks available.
        printf("Error: No free blocks available in FAT after retries.\n");
        fflush(stdout);
    }
    // Print debug message from fat_allocate_block with the allocated block value
    printf("DEBUG FROM fat_allocate_block: Allocated block: %u\n", block);
    fflush(stdout);
    return block; // Return the allocated block number or FAT_NO_FREE_BLOCKS if no block was found.
}
 




void fat_free_block(uint32_t blockIndex) {
    // Validate the block index before proceeding.
    if (blockIndex == FAT_NO_FREE_BLOCKS || blockIndex >= TOTAL_BLOCKS) {
        // The block index is either indicating there are no free blocks, or it is out of the valid range.
        printf("Error: Attempted to free an invalid block index (%u). Block index is greater than TOTAL_BLOCKS or no free blocks are indicated.\n", blockIndex);
        fflush(stdout);
        return;
    }

    // Lock the FAT for exclusive access.
    mutex_enter_blocking(&fat_mutex);

    // Check if the block index is invalid or reserved, but do it inside the mutex to avoid race conditions.
    if (FAT[blockIndex] == FAT_ENTRY_INVALID || FAT[blockIndex] == FAT_ENTRY_RESERVED) {
        printf("Error: Attempted to free a reserved or invalid block (%u).\n", blockIndex);
        fflush(stdout);
        mutex_exit(&fat_mutex); // Release the mutex before returning.
        return;
    }

    // Check if the block is already free to avoid double-freeing.
    if (FAT[blockIndex] == FAT_ENTRY_FREE) {
        printf("Warning: Attempted to free a block that is already free (%u).\n", blockIndex);
        fflush(stdout);
        mutex_exit(&fat_mutex); // Release the mutex before returning.
        return;
    }

    // Mark the block as free.
    FAT[blockIndex] = FAT_ENTRY_FREE;

    // Release the FAT lock.
    mutex_exit(&fat_mutex);

    // Log the freeing of the block.
    printf("Block %u successfully freed.\n", blockIndex);
    fflush(stdout);
}

 


int fat_get_next_block(uint32_t currentBlock, uint32_t* nextBlock) {
    if (currentBlock >= TOTAL_BLOCKS) {
        printf("Error: Block index (%u) is out of valid range in fat_get_next_block.\n", currentBlock);
        printf("Diagnostic Info: TOTAL_BLOCKS = %u\n", TOTAL_BLOCKS);
        fflush(stdout);
        return FAT_OUT_OF_RANGE; // Block index is out of range
    }

    mutex_enter_blocking(&fat_mutex); // Secure exclusive access to the FAT

    *nextBlock = FAT[currentBlock]; // Retrieve the next block index from the FAT

    mutex_exit(&fat_mutex); // Release the FAT lock

   
    // Handle special FAT entry values
    switch (*nextBlock) {
        case FAT_ENTRY_END:
            // printf("Info: Reached the end of the file chain starting from block %u.\n", currentBlock);
            // fflush(stdout);
            return FAT_SUCCESS; // Indicate the end of the chain

        case FAT_ENTRY_FREE:
        case FAT_ENTRY_INVALID:
            printf("Error: Encountered unexpected FAT entry (FREE or INVALID) for block %u.\n", currentBlock);
            printf("Diagnostic Info: Current block status is %u.\n", FAT[currentBlock]);
            fflush(stdout);
            return FAT_CORRUPTED; // Indicate an error

        case FAT_DIRECTORY_MARKER:
            printf("Info: Block %u is marked as a directory.\n", currentBlock);
            printf("Diagnostic Info: Block %u is a directory with FAT entry %u.\n", currentBlock, FAT_DIRECTORY_MARKER);
            fflush(stdout);
            return FAT_INVALID_OPERATION; // Directories should be handled differently

        default:
            if (*nextBlock < TOTAL_BLOCKS) {
                // Next block is valid; return success
                return FAT_SUCCESS;
            } else {
                // The nextBlock index is out of range, indicating a corruption or mismanagement in the FAT
                printf("Error: Next block index (%u) retrieved from block %u is out of range.\n", *nextBlock, currentBlock);
                printf("Diagnostic Info: FAT entry for block %u is corrupted.\n", currentBlock);
                fflush(stdout);
                return FAT_CORRUPTED; // Indicate an error
            }
    }
}





// Update the FAT to chain two blocks together
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock) {
    printf("Attempting to link blocks: %u -> %u\n", prevBlock, nextBlock);
    // Validate the block indices
    if (prevBlock >= TOTAL_BLOCKS || nextBlock >= TOTAL_BLOCKS) {
        printf("Error: Attempted to link invalid block indices (%u -> %u).\n", prevBlock, nextBlock);
        fflush(stdout);
        return; // Early return to prevent further invalid operations
    }

  

    // Additionally, ensure that neither of the blocks is reserved or marked as invalid
    if (prevBlock == FAT_ENTRY_RESERVED || nextBlock == FAT_ENTRY_RESERVED ||
        prevBlock == FAT_ENTRY_INVALID || nextBlock == FAT_ENTRY_INVALID ||
        FAT[prevBlock] == FAT_DIRECTORY_MARKER || FAT[nextBlock] == FAT_DIRECTORY_MARKER) {
        printf("Error: Attempted to link reserved, invalid, or directory blocks (%u -> %u).\n", prevBlock, nextBlock);
        fflush(stdout);
        return;
    }

    printf("Acquiring FAT mutex for linking.\n");
    // Lock the FAT for exclusive access
    mutex_enter_blocking(&fat_mutex);

    // Check if the prevBlock is already linked to another block
    if (FAT[prevBlock] != FAT_ENTRY_FREE && FAT[prevBlock] != FAT_ENTRY_END) {
        printf("Warning: Overwriting existing link from block %u to block %u.\n", prevBlock, FAT[prevBlock]);
        fflush(stdout);
        // Depending on your filesystem's design, you might want to handle this differently.
        // For example, you could prevent overwriting or clean up the overwritten chain.
    }

    // Perform the linking
    FAT[prevBlock] = nextBlock;

    // If the next block was marked as free, update it to indicate it's now part of a chain
    // This step depends on your specific FAT implementation and might not be necessary
    if (FAT[nextBlock] == FAT_ENTRY_FREE) {
        FAT[nextBlock] = FAT_ENTRY_END;
    }

    // Release the FAT lock
    mutex_exit(&fat_mutex);

    // Optionally, log the successful linking for debugging or auditing
    printf("Successfully linked block %u to block %u.\n", prevBlock, nextBlock);
    printf("Linking completed. %u -> %u\n", prevBlock, nextBlock);
    fflush(stdout);
}

 
  
uint32_t fat_allocate_nearest_block(uint32_t hintBlock) {
    if (hintBlock >= TOTAL_BLOCKS) {
        printf("Error: Hint block index (%u) out of bounds.\n", hintBlock);
        fflush(stdout);
        return FAT_NO_FREE_BLOCKS; // Indicate failure to allocate
    }

    // Lock the FAT for exclusive access
    mutex_enter_blocking(&fat_mutex);

    if (FAT[hintBlock] == FAT_ENTRY_FREE) {
        // The hint block itself is free, so use it.
        FAT[hintBlock] = FAT_ENTRY_END; // Mark as the end of a file chain
        mutex_exit(&fat_mutex); // Release the FAT lock
        return hintBlock;
    }

    // Search for the nearest free block, starting from the hint block and expanding outward
    for (uint32_t offset = 1; offset < TOTAL_BLOCKS; offset++) {
        uint32_t checkBlockPrev = (hintBlock > offset) ? hintBlock - offset : FAT_NO_FREE_BLOCKS;
        uint32_t checkBlockNext = (hintBlock + offset < TOTAL_BLOCKS) ? hintBlock + offset : FAT_NO_FREE_BLOCKS;

        if (checkBlockPrev < TOTAL_BLOCKS && FAT[checkBlockPrev] == FAT_ENTRY_FREE) {
            // Found a free block before the hint block
            FAT[checkBlockPrev] = FAT_ENTRY_END;
            mutex_exit(&fat_mutex); // Release the FAT lock
            return checkBlockPrev;
        } else if (checkBlockNext < TOTAL_BLOCKS && FAT[checkBlockNext] == FAT_ENTRY_FREE) {
            // Found a free block after the hint block
            FAT[checkBlockNext] = FAT_ENTRY_END;
            mutex_exit(&fat_mutex); // Release the FAT lock
            return checkBlockNext;
        }

        // If the search reaches the end of the FAT without finding a free block, break
        if (checkBlockPrev == FAT_NO_FREE_BLOCKS && checkBlockNext == FAT_NO_FREE_BLOCKS) {
            break;
        }
    }

    mutex_exit(&fat_mutex); // Ensure the FAT lock is always released
    printf("Error: No free blocks available near hint block %u.\n", hintBlock);
    fflush(stdout);
    return FAT_NO_FREE_BLOCKS; // Indicate failure to allocate
}



 #include "filesystem.h"  // Include your file system's header files
#include "filesystem.h"  // Include your file system's header files

/**
 * Writes data to a file using FAT for block management, ensuring that space is reserved for FileEntry in the first block.
 * 
 * @param file The FS_FILE pointer representing the file to write to.
 * @param buffer The buffer containing data to write to the file.
 * @param size The number of bytes to write.
 * @return The number of bytes actually written, or -1 if an error occurs.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid input parameters.\n");
        return -1;  // Error: Invalid input
    }

    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1;  // Error: Incorrect file mode
    }

    // Initialize variables for block management
    // `currentBlock` holds the block number from which the writing should start. This is retrieved from the file's metadata (`file->entry->start_block`),
    // which indicates the first block of the file where the data begins. It's critical to track which block we are currently dealing with during write operations.
    uint32_t currentBlock = file->entry->start_block;

    // `currentBlockPosition` calculates the position within the current block where the writing should start. This is calculated by taking the modulo of
    // the file's current position (`file->position`) with the size of a block (`BLOCK_SIZE`). This helps in determining the exact offset within the block
    // to begin writing, ensuring data continuity and integrity, particularly important in append operations or when writing to a block partially filled.
    uint32_t currentBlockPosition = file->position % BLOCK_SIZE;

    // `writeBuffer` is a pointer to the buffer containing the data to be written. This pointer is used to manage the actual data transfer from the buffer to the flash storage.
    // Type casting to `const uint8_t*` ensures that byte-wise operations can be performed on the buffer, which is necessary for precise data handling during the flash write operations.
    const uint8_t* writeBuffer = (const uint8_t*) buffer;

    // `bytesWritten` keeps a running total of the number of bytes successfully written to the flash. This is initialized to 0 and will be updated continuously
    // throughout the write process as each block of data is written to the flash memory. This count is crucial for reporting back to the calling function how much data
    // was actually written, especially in cases where the write operation might be truncated due to space limitations or errors.
    int bytesWritten = 0;

    // `isFirstBlock` is a flag used to handle special logic on the first block of a file. This includes potentially skipping space for the `FileEntry` metadata at the
    // beginning of the file's first block. It is initially set to true to indicate that the first block handling logic should be executed at least once.
    bool isFirstBlock = true;


    printf("Starting fs_write with file mode %c, initial block %u, position %u, size %d\n", file->mode, currentBlock, currentBlockPosition, size);

    while (size > 0) {
        printf("Loop start: currentBlock=%u, currentBlockPosition=%u, remaining size=%d\n", currentBlock, currentBlockPosition, size);
        
        // Manage file blocks to ensure there's a block to write to
        // This call to 'manage_file_blocks' checks if the current block has sufficient space to continue writing,
        // and allocates new blocks as necessary. It handles the linkage of new blocks in the FAT structure
        // to ensure data is stored contiguously across multiple blocks as needed.
        currentBlock = manage_file_blocks(file, &currentBlock, &currentBlockPosition, size);
        if (currentBlock == FAT_NO_FREE_BLOCKS) {
            // If no blocks are available for allocation, log an error and exit the write loop.
            // This situation might occur if the disk is full or the FAT table is unable to allocate more blocks,
            // possibly due to fragmentation or a full filesystem.
            printf("Error: Unable to allocate necessary blocks.\n");
            break;  // No free blocks available, break the write loop
        }

        // Adjust the first block position to skip the space for FileEntry
        // This adjustment is critical in the first block where the FileEntry structure is stored.
        // Skipping over the size of FileEntry ensures that we do not overwrite the metadata of the file
        // with actual file data, preserving the integrity of the file system.
        if (isFirstBlock) {
            printf("Adjusting first block offset for FileEntry structure.\n");
            currentBlockPosition += sizeof(FileEntry);  // Skip the space used by FileEntry in the first block
            isFirstBlock = false;  // After the first adjustment, set isFirstBlock to false
        }

        // Calculate the number of bytes to write in the current block
        // This calculation determines how much data can be written in the remaining space of the current block.
        // It considers the lesser of two values: the remaining space in the block after the current position,
        // and the remaining amount of data to write. This helps to handle writes that need to span multiple blocks.
        int toWrite = MIN(BLOCK_SIZE - currentBlockPosition, size);

        // Perform the write operation to the block
        // This section calculates the exact offset in the flash memory where data needs to be written.
        // The offset is determined by multiplying the current block number by the size of each block (BLOCK_SIZE),
        // then adding the position within the block where the data write should start. This calculation ensures
        // that data is written to the correct location within the filesystem's allocated blocks.
        uint32_t writeOffset = currentBlock * BLOCK_SIZE + currentBlockPosition;

        // Logging the operation provides visibility into the write process, including the amount of data being written,
        // the block number, the position within the block, and the calculated offset in the filesystem.
        // This debug information is crucial for diagnosing issues related to data corruption or misalignment in the filesystem.
        printf("Writing %d bytes to block %u at position %u (offset %u).\n", toWrite, currentBlock, currentBlockPosition, writeOffset);

        // Perform the actual write operation using the flash_write_safe2 function.
        // This function takes the calculated offset, the buffer of data to be written, and the number of bytes to write.
        // The function should ensure that data integrity and error handling are managed according to the underlying hardware
        // specifications and filesystem architecture. It's crucial that this operation is reliable as it directly affects
        // the filesystem's data integrity.
        flash_write_safe2(writeOffset, writeBuffer, toWrite);  // Actual flash write operation


        // Update the file position and the number of bytes written
        // Increment the file's current position by the number of bytes successfully written in this operation (`toWrite`).
        // This update ensures that subsequent write operations start at the correct location in the file, maintaining the continuity
        // and integrity of the file's data.
        file->position += toWrite;

        // Add the number of bytes written in the current operation to `bytesWritten`, which tracks the total number of bytes
        // written during this call to `fs_write`. This cumulative count is essential for the function to return the total
        // bytes written, providing feedback on the operation's success.
        bytesWritten += toWrite;

        // Move the `writeBuffer` pointer forward by the number of bytes written (`toWrite`). This adjustment ensures that
        // the next write operation, if needed, starts at the correct position in the user-provided input buffer, avoiding
        // re-writing of data already written to flash or skipping of data yet to be written.
        writeBuffer += toWrite;

        // Reduce the remaining size (`size`) of the data to be written by the amount just written (`toWrite`). This decrement
        // is crucial for the loop's condition, determining when all requested data has been written, and helps prevent
        // buffer overruns by ensuring that no more data is written than was originally requested.
        size -= toWrite;

        // Update `currentBlockPosition` to reflect the new position within the current block after writing. It's calculated by taking
        // the sum of the current block position and the number of bytes written, then modulo the block size (`BLOCK_SIZE`).
        // This calculation results in the offset within the current block where the next write should begin, if any more data remains to be written.
        // This step is critical for ensuring that data written to blocks does not exceed the block boundaries and appropriately
        // moves to the next block when needed.
        currentBlockPosition = (currentBlockPosition + toWrite) % BLOCK_SIZE;


        printf("Written %d bytes, new file position %u, bytes remaining %d\n", toWrite, file->position, size);

        // Check if we need to move to the next block
        // This check occurs after writing data to ensure that if the current block is fully utilized (currentBlockPosition == 0),
        // and there is still more data to write (size > 0), the function transitions to the next block in the FAT chain.
        if (currentBlockPosition == 0 && size > 0) {
            uint32_t nextBlock;
            // Retrieve the next block in the chain from the FAT. This step is essential to continue writing remaining data
            // without interruption, ensuring data continuity and integrity within the file system.
            int result = fat_get_next_block(currentBlock, &nextBlock);

            // Check the result of retrieving the next block. If the result indicates an error (FAT_SUCCESS not returned)
            // or the next block marker is FAT_ENTRY_END (signifying no further blocks are linked in the FAT for this file),
            // report an error and break out of the write loop. This condition might indicate a misconfiguration in the FAT
            // or that the file system has run out of space if no more blocks are available.
            if (result != FAT_SUCCESS || nextBlock == FAT_ENTRY_END) {
                printf("Error or end of chain reached without sufficient blocks. NextBlock=%u, Result=%d\n", nextBlock, result);
                break;
            }

            // If a valid next block is found and there are no errors, update the currentBlock to the next block.
            // This update allows the next iteration of the loop (if any) to continue writing data to the correct block.
            currentBlock = nextBlock;
            printf("Moving to next block: %u\n", currentBlock);
        }

    }

    printf("Total %d bytes written.\n", bytesWritten);
    return bytesWritten;
}





/**
 * Writes data to a file using FAT for block management, ensuring that space is reserved for FileEntry in the first block.
 * 
 * @param file The FS_FILE pointer representing the file to write to.
 * @param buffer The buffer containing data to write to the file.
 * @param size The number of bytes to write.
 * @return The number of bytes actually written, or -1 if an error occurs.
 */
int fs_write(FS_FILE* file, const void* buffer, int size) {
    if (file == NULL || buffer == NULL || size < 0) {
        printf("Error: Invalid input parameters.\n");
        return -1;  // Error: Invalid input
    }

    if (file->mode != 'a' && file->mode != 'w') {
        printf("Error: File not open in a writable or appendable mode.\n");
        return -1;  // Error: Incorrect file mode
    }

    // Initialize variables for block management
    // `currentBlock` holds the block number from which the writing should start. This is retrieved from the file's metadata (`file->entry->start_block`),
    // which indicates the first block of the file where the data begins. It's critical to track which block we are currently dealing with during write operations.
    uint32_t currentBlock = file->entry->start_block;

    // `currentBlockPosition` calculates the position within the current block where the writing should start. This is calculated by taking the modulo of
    // the file's current position (`file->position`) with the size of a block (`FILESYSTEM_BLOCK_SIZE`). This helps in determining the exact offset within the block
    // to begin writing, ensuring data continuity and integrity, particularly important in append operations or when writing to a block partially filled.
    uint32_t currentBlockPosition = file->position;

    // `writeBuffer` is a pointer to the buffer containing the data to be written. This pointer is used to manage the actual data transfer from the buffer to the flash storage.
    // Type casting to `const uint8_t*` ensures that byte-wise operations can be performed on the buffer, which is necessary for precise data handling during the flash write operations.
    const uint8_t* writeBuffer = (const uint8_t*) buffer;

    // `bytesWritten` keeps a running total of the number of bytes successfully written to the flash. This is initialized to 0 and will be updated continuously
    // throughout the write process as each block of data is written to the flash memory. This count is crucial for reporting back to the calling function how much data
    // was actually written, especially in cases where the write operation might be truncated due to space limitations or errors.
    int bytesWritten = 0;

    // `isFirstBlock` is a flag used to handle special logic on the first block of a file. This includes potentially skipping space for the `FileEntry` metadata at the
    // beginning of the file's first block. It is initially set to true to indicate that the first block handling logic should be executed at least once.
    bool isFirstBlock = true;


    printf("Starting fs_write with file mode %c, initial block %u, position %u, size %d\n", file->mode, currentBlock, currentBlockPosition, size);

    while (size > 0) {
        printf("Loop start: currentBlock=%u, currentBlockPosition=%u, remaining size=%d\n", currentBlock, currentBlockPosition, size);
        
        // Manage file blocks to ensure there's a block to write to
        // This call to 'manage_file_blocks' checks if the current block has sufficient space to continue writing,
        // and allocates new blocks as necessary. It handles the linkage of new blocks in the FAT structure
        // to ensure data is stored contiguously across multiple blocks as needed.
        currentBlock = manage_file_blocks(file, &currentBlock, &currentBlockPosition, size);
        if (currentBlock == FAT_NO_FREE_BLOCKS) {
            // If no blocks are available for allocation, log an error and exit the write loop.
            // This situation might occur if the disk is full or the FAT table is unable to allocate more blocks,
            // possibly due to fragmentation or a full filesystem.
            printf("Error: Unable to allocate necessary blocks.\n");
            break;  // No free blocks available, break the write loop
        }

        // Adjust the first block position to skip the space for FileEntry
        // This adjustment is critical in the first block where the FileEntry structure is stored.
        // Skipping over the size of FileEntry ensures that we do not overwrite the metadata of the file
        // with actual file data, preserving the integrity of the file system.
        if (isFirstBlock) {
            printf("Adjusting first block offset for FileEntry structure.\n");
            currentBlockPosition += sizeof(FileEntry);  // Skip the space used by FileEntry in the first block
            isFirstBlock = false;  // After the first adjustment, set isFirstBlock to false
        }

        // Calculate the number of bytes to write in the current block
        // This calculation determines how much data can be written in the remaining space of the current block.
        // It considers the lesser of two values: the remaining space in the block after the current position,
        // and the remaining amount of data to write. This helps to handle writes that need to span multiple blocks.
        int toWrite = MIN(FILESYSTEM_BLOCK_SIZE - currentBlockPosition, size);

        // Perform the write operation to the block
        // This section calculates the exact offset in the flash memory where data needs to be written.
        // The offset is determined by multiplying the current block number by the size of each block (FILESYSTEM_BLOCK_SIZE),
        // then adding the position within the block where the data write should start. This calculation ensures
        // that data is written to the correct location within the filesystem's allocated blocks.
        uint32_t writeOffset = currentBlock * FILESYSTEM_BLOCK_SIZE + currentBlockPosition;

        // Logging the operation provides visibility into the write process, including the amount of data being written,
        // the block number, the position within the block, and the calculated offset in the filesystem.
        // This debug information is crucial for diagnosing issues related to data corruption or misalignment in the filesystem.
        printf("Writing %d bytes to block %u at position %u (offset %u).\n", toWrite, currentBlock, currentBlockPosition, writeOffset);

        // Perform the actual write operation using the flash_write_safe2 function.
        // This function takes the calculated offset, the buffer of data to be written, and the number of bytes to write.
        // The function should ensure that data integrity and error handling are managed according to the underlying hardware
        // specifications and filesystem architecture. It's crucial that this operation is reliable as it directly affects
        // the filesystem's data integrity.


        printf("length of writeBuffer: %d\n", strlen(writeBuffer));
        printf("writeBuffer: %s\n", writeBuffer);
        printf("sizeof(writeBuffer): %d\n", sizeof(writeBuffer));
         char destination[256];  // Example size, ensure it is large enough

        // Copying the string from `buffer` to `destination`
        strcpy(destination, writeBuffer);

        flash_write_safe2(writeOffset, destination, strlen(destination));  // Actual flash write operation


        // Update the file position and the number of bytes written
        // Increment the file's current position by the number of bytes successfully written in this operation (`toWrite`).
        // This update ensures that subsequent write operations start at the correct location in the file, maintaining the continuity
        // and integrity of the file's data.
        file->position += toWrite;

        // Add the number of bytes written in the current operation to `bytesWritten`, which tracks the total number of bytes
        // written during this call to `fs_write`. This cumulative count is essential for the function to return the total
        // bytes written, providing feedback on the operation's success.
        bytesWritten += toWrite;

        // Move the `writeBuffer` pointer forward by the number of bytes written (`toWrite`). This adjustment ensures that
        // the next write operation, if needed, starts at the correct position in the user-provided input buffer, avoiding
        // re-writing of data already written to flash or skipping of data yet to be written.
        writeBuffer += toWrite;

        // Reduce the remaining size (`size`) of the data to be written by the amount just written (`toWrite`). This decrement
        // is crucial for the loop's condition, determining when all requested data has been written, and helps prevent
        // buffer overruns by ensuring that no more data is written than was originally requested.
        size -= toWrite;

        // Update `currentBlockPosition` to reflect the new position within the current block after writing. It's calculated by taking
        // the sum of the current block position and the number of bytes written, then modulo the block size (`FILESYSTEM_BLOCK_SIZE`).
        // This calculation results in the offset within the current block where the next write should begin, if any more data remains to be written.
        // This step is critical for ensuring that data written to blocks does not exceed the block boundaries and appropriately
        // moves to the next block when needed.
     

    }

    printf("Total %d bytes written.\n", bytesWritten);
    return bytesWritten;
}
