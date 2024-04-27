#ifndef VISUAL_H
#define VISUAL_H

#include "../filesystem/filesystem.h" 
#include "../config/flash_config.h" 

 void print_indent(int level);
// void print_directory(uint32_t dirId, int level);
void print_filesystem_structure();


void print_directory(uint32_t parentDirId, int level, bool *visited);


#endif // FILESYSTEM_HELPER_H