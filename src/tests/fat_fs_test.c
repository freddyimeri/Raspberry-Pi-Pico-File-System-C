

#include "../FAT/fat_fs.h"
#include <stdio.h>
#include "../tests/fat_fs_test.h"


void run_all_tests_FAT() {
    char slashes[] = "\n/////////////////////////////////////////////\n";

    printf("%s", slashes);
    test_fat_init();
    printf("%s", slashes);
    test_fat_allocate_block();
    printf("%s", slashes);
    test_fat_free_block();
    printf("%s", slashes);
    test_fat_link_blocks();
    printf("%s", slashes);
   




}




void test_fat_init() {
    printf("Testing fat_init...\n");
    fat_init();
    int reserved_blocks_ok = 1;
    for (uint32_t i = 0; i < NUMBER_OF_RESERVED_BLOCKS + 5; i++) {
        if (FAT[i] != FAT_ENTRY_RESERVED) {
            reserved_blocks_ok = 0;
            break;
        }
    }
    if (reserved_blocks_ok) {
        printf("fat_init Test Passed - All reserved blocks are correctly marked.\n");
    } else {
        printf("fat_init Test Failed - Reserved blocks are not correctly set up.\n");
    }
}


void test_fat_allocate_block() {
    printf("Testing fat_allocate_block...\n");
    uint32_t block = fat_allocate_block();
    if (block != FAT_NO_FREE_BLOCKS && FAT[block] == FAT_ENTRY_END) {
        printf("Block Allocation Test Passed - Block %u allocated.\n", block);
    } else {
        printf("Block Allocation Test Failed.\n");
    }
}


void test_fat_free_block() {
    printf("Testing fat_free_block...\n");
    uint32_t block = fat_allocate_block(); // Allocate a block first
    fat_free_block(block);
    if (FAT[block] == FAT_ENTRY_FREE) {
        printf("Block Freeing Test Passed - Block %u freed.\n", block);
    } else {
        printf("Block Freeing Test Failed - Block %u not freed.\n", block);
    }
}

void test_fat_link_blocks() {
    printf("Testing fat_link_blocks...\n");
    uint32_t block1 = fat_allocate_block();
    uint32_t block2 = fat_allocate_block();
    fat_link_blocks(block1, block2);
    if (FAT[block1] == block2) {
        printf("Block Linking Test Passed - Block %u linked to Block %u.\n", block1, block2);
    } else {
        printf("Block Linking Test Failed - Blocks not linked correctly.\n");
    }
}

 

