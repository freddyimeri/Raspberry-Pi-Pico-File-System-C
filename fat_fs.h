#ifndef FAT_FS_H
#define FAT_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  
 

#define FLASH_MEMORY_SIZE_BYTES 2 * 1024 * 1024 // Example: 2MB flash size
#define FLASH_BLOCK_SIZE 4096 
#define TOTAL_BLOCKS (FLASH_MEMORY_SIZE_BYTES / FLASH_BLOCK_SIZE)

// Special FAT entry values
#define FAT_ENTRY_END 0xFFFFFFFE // Indicates the end of a file in the FAT
#define FAT_ENTRY_FULL 0xFFFFFFFD // Indicates no free blocks available
#define FAT_ENTRY_INVALID 0xFFFFFFFF // Indicates an invalid FAT entry

typedef uint32_t FAT_Entry;

// Function declarations
void fat_init(void);
uint32_t fat_allocate_block(void);
void fat_free_block(uint32_t blockIndex);
uint32_t fat_get_next_block(uint32_t currentBlock);
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock);

#endif // FAT_FS_H
