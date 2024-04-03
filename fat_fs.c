#include "fat_fs.h"

FAT_Entry FAT[TOTAL_BLOCKS];

// Initialize the FAT system
void fat_init() {
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        FAT[i] = FAT_ENTRY_END;
    }
}

// Allocate a block and mark it as used
uint32_t fat_allocate_block() {
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_END) {
            // This block is now the end of a file, but could later be linked to another block
            FAT[i] = FAT_ENTRY_END;
            return i;
        }
    }
    // Return an error code if no blocks are available
    return FAT_ENTRY_FULL; // Ensure FAT_ENTRY_FULL is defined as an error condition in your header file
}

// Free a block for future use
void fat_free_block(uint32_t blockIndex) {
    if (blockIndex < TOTAL_BLOCKS) {
        FAT[blockIndex] = FAT_ENTRY_END;
    } else {
        // Handle error condition if an invalid block index is passed
        // For now, we'll just print an error. Adjust as necessary for your system's error handling
        printf("Error: Attempted to free an invalid block index (%u).\n", blockIndex);
    }
}

// Find the next block in a file chain, given the current block
uint32_t fat_get_next_block(uint32_t currentBlock) {
    if (currentBlock < TOTAL_BLOCKS) {
        return FAT[currentBlock];
    }
    // Return an error code if an invalid block index is requested
    return FAT_ENTRY_INVALID; // Ensure FAT_ENTRY_INVALID is defined as an error condition in your header file
}

// Update the FAT to chain two blocks together
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock) {
    if (prevBlock < TOTAL_BLOCKS && nextBlock < TOTAL_BLOCKS) {
        FAT[prevBlock] = nextBlock;
    } else {
        // Handle error condition if an invalid block index is passed
        printf("Error: Attempted to link invalid blocks (%u -> %u).\n", prevBlock, nextBlock);
    }
}

