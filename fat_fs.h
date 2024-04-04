#ifndef FAT_FS_H
#define FAT_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  
#include "flash_config.h"



// Special FAT entry values
#define FAT_ENTRY_FREE 0xFFFFFFFF // Indicates a block is free
#define FAT_ENTRY_END 0xFFFFFFFE // Indicates the end of a file in the FAT
#define FAT_ENTRY_FULL 0xFFFFFFFD // Indicates no free blocks available
#define FAT_ENTRY_INVALID 0xFFFFFFFF // Indicates an invalid FAT entry
#define NO_TIMESTAMP 0xFFFFFFFF
#define NO_SIZE 0xFFFFFFFF

// In fat_fs.h
extern uint32_t FAT[TOTAL_BLOCKS]; // Declare FAT but don't define it

  
// Function declarations
void fat_init(void);
uint32_t fat_allocate_block(void);
uint32_t fat_allocate_nearest_block(uint32_t hintBlock);
void fat_free_block(uint32_t blockIndex);
uint32_t fat_get_next_block(uint32_t currentBlock);
void fat_link_blocks(uint32_t prevBlock, uint32_t nextBlock);

#endif // FAT_FS_H



