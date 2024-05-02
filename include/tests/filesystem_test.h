 

#ifndef FILESTYSTEM_TEST_H
#define FILESTYSTEM_TEST_H

#include <stdint.h>
#include <stddef.h>

void run_all_tests_filesystem();



void test_fs_write_and_read(void);
 void test_fs_append_to_file(void);
void test_fs_large_file_handling(void);
void test_fs_file_seek(void);
void test_fs_open_nonexistent_file(void);

void test_fs_cp_function(void);

void test_fs_rm(void);

#endif // FILESTYSTEM_TEST_H

