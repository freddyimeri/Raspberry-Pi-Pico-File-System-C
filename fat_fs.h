#ifndef FAT_FS_H
#define FAT_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "flash_config.h"  // Include configuration for filesystem parameters

// Special FAT entry values are defined here to manage the state of each block in the filesystem.
#define FAT_ENTRY_FREE 0xFFFFFFFF     // Indicates a block is currently unused and available for allocation.
#define FAT_ENTRY_END 0xFFFFFFFE      // Marks the end of a file chain, indicating no further blocks are chained.
#define FAT_ENTRY_RESERVED 0xFFFFFFFC // Denotes a block reserved for special purposes, not available for general use.
#define FAT_NO_FREE_BLOCKS 0xFFFFFFFB // A return value indicating no available blocks for allocation.
#define FAT_ENTRY_INVALID 0xFFFFFFFA  // Signifies an invalid entry, used for error handling and validation.
#define FAT_ENTRY_FULL 0xFFFFFFFD
#define FAT_DIRECTORY_MARKER 0xFFFFFFFD

// Additional definitions for file attributes not directly related to the FAT but useful for managing file metadata.
#define NO_TIMESTAMP 0xFFFFFFFF // Represents an undefined or invalid timestamp for file metadata.
#define NO_SIZE 0xFFFFFFFF // Used to indicate an undefined or invalid size, possibly for directories.

// Declaration of the FAT array.
// This array is the core of the FAT filesystem, mapping each block to its subsequent block in a file,
// or indicating its status (free, reserved, end of file, etc.).
extern uint32_t FAT[TOTAL_BLOCKS]; // Declare FAT but don't define it here


// Function declarations for managing the FAT and the files/directories within the filesystem.

// Allocates a free block for use by a file or directory, marking it as in use.
// Returns the block number of the allocated block, or FAT_NO_FREE_BLOCKS if none are available.
void fat_init(void);

// Attempts to allocate a block as close as possible to a specified 'hint' block.
// This is useful for minimizing fragmentation and improving access times.
uint32_t fat_allocate_block(void);

// Frees a previously allocated block, returning it to the pool of available blocks.
uint32_t fat_allocate_nearest_block(uint32_t hintBlock);

// Frees a previously allocated block, returning it to the pool of available blocks.
void fat_free_block(uint32_t blockIndex);

// Retrieves the next block in a file's chain, given the current block.
// This supports sequential access to files stored across multiple blocks.
int fat_get_next_block(uint32_t currentBlock, uint32_t* nextBlock);

// Links two blocks together in the FAT, effectively chaining them as part of a file.
// This is crucial for supporting files that span multiple blocks.
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock);

#endif // FAT_FS_H
