 

#ifndef FAT_FS_TEST_H
#define FAT_FS_TEST_H

#include <stdint.h>
#include <stddef.h>


void run_all_tests_FAT();

 void test_fat_init();
void test_fat_allocate_block();
void test_fat_free_block();
void test_fat_link_blocks();
 

#endif // FILESTYSTEM_HELPER_TEST_H





