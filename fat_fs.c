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

#include "fat_fs.h"
#include <stdio.h>
#include "flash_config.h"
#include "hardware/flash.h"   
#include "filesystem.h"
#include "pico/mutex.h"
#include "pico/time.h"


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
