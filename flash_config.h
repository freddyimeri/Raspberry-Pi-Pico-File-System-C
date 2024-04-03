#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include "pico/stdlib.h"  
 
// Use the SDK-provided definition for the total flash size
#define FLASH_MEMORY_SIZE_BYTES PICO_FLASH_SIZE_BYTES
 


// Define the total number of blocks in the flash memory
#define TOTAL_BLOCKS (FLASH_MEMORY_SIZE_BYTES / FLASH_BLOCK_SIZE)

// Define the offset from the start of the flash memory where user data can start.
#define FLASH_TARGET_OFFSET (256 * 1024) // Reserve the first 256KB for system use or bootloader

// Define the total usable size of the flash memory for user data,
// considering any space reserved for metadata or wear leveling.
#define FLASH_METADATA_SPACE (256 * 1024)  // Additional space reserved for metadata and wear leveling
#define FLASH_USABLE_SPACE (FLASH_MEMORY_SIZE_BYTES - FLASH_METADATA_SPACE - FLASH_TARGET_OFFSET) // Usable space for user data

// Maximum number of files supported by the filesystem
#define MAX_FILES 10

// Calculate the maximum file size, aligning to the block size as necessary.
#define MAX_FILE_SIZE ((FLASH_USABLE_SPACE / MAX_FILES) & ~(FLASH_BLOCK_SIZE - 1))

#endif // FLASH_CONFIG_H
