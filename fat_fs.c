#include "fat_fs.h"
#include <stdio.h>
#include "flash_config.h"
#include "hardware/flash.h"  // This is where FLASH_BLOCK_SIZE should be defined.

FAT_Entry FAT[TOTAL_BLOCKS]; // The FAT table

// Initialize the FAT system
void fat_init() {
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        FAT[i] = FAT_ENTRY_FREE; // Initially, all blocks are free
    }
}

// Allocate a block and mark it as used
uint32_t fat_allocate_block() {
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (FAT[i] == FAT_ENTRY_FREE) {
            FAT[i] = FAT_ENTRY_END; // Mark this block as the end of a file
            return i;
        }
    }
    return FAT_ENTRY_FULL; // No free blocks available
}

// Free a block for future use
void fat_free_block(uint32_t blockIndex) {
    if (blockIndex < TOTAL_BLOCKS) {
        FAT[blockIndex] = FAT_ENTRY_FREE;
    } else {
        printf("Error: Attempted to free an invalid block index (%u).\n", blockIndex);
    }
}

// Find the next block in a file chain, given the current block
uint32_t fat_get_next_block(uint32_t currentBlock) {
    if (currentBlock < TOTAL_BLOCKS) {
        return FAT[currentBlock];
    }
    return FAT_ENTRY_INVALID; // Invalid block index
}

// Update the FAT to chain two blocks together
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock) {
    if (prevBlock < TOTAL_BLOCKS && nextBlock < TOTAL_BLOCKS) {
        FAT[prevBlock] = nextBlock;
    } else {
        printf("Error: Attempted to link invalid blocks (%u -> %u).\n", prevBlock, nextBlock);
    }
}

// Attempts to allocate the nearest free block to the hintBlock
uint32_t fat_allocate_nearest_block(uint32_t hintBlock) {
    int distance = 1;
    while (distance < TOTAL_BLOCKS) {
        int prevBlock = (hintBlock - distance + TOTAL_BLOCKS) % TOTAL_BLOCKS;
        int nextBlock = (hintBlock + distance) % TOTAL_BLOCKS;

        if (FAT[prevBlock] == FAT_ENTRY_FREE) {
            FAT[prevBlock] = FAT_ENTRY_END; // Mark as used
            return prevBlock;
        }

        if (FAT[nextBlock] == FAT_ENTRY_FREE) {
            FAT[nextBlock] = FAT_ENTRY_END; // Mark as used
            return nextBlock;
        }

        distance++;
    }
    return FAT_ENTRY_FULL; // No free blocks available
}
