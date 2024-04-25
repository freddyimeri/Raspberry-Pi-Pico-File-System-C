    #ifndef FLASH_CONFIG_H
    #define FLASH_CONFIG_H

    #include "pico/stdlib.h"  
    
    // The size in bytes of each block used in the filesystem.
    // A standard size for file systems to enable efficient use of space and management.
    #define FILESYSTEM_BLOCK_SIZE 4096

    // The total size of the Flash memory available on the Pico, as defined by the SDK.
    // This is used to determine the amount of space we have for storing files.
    #define FLASH_MEMORY_SIZE_BYTES PICO_FLASH_SIZE_BYTES
    
    // The total number of blocks in the Flash memory.
    // This is calculated by dividing the total flash size by the size of each block,
    // providing a count of how many individual blocks we can manage.
    #define TOTAL_BLOCKS (FLASH_MEMORY_SIZE_BYTES / FILESYSTEM_BLOCK_SIZE)

    // The offset in bytes from the start of the Flash memory where user data can begin.
    // This reserves the first portion of Flash for system use, such as a bootloader,
    // ensuring that user data does not overwrite crucial system information.
    #define FLASH_TARGET_OFFSET (256 * 1024) // Reserve the first 256KB for system use or bootloader

    // The space in bytes reserved at the beginning of the flash memory. This is effectively
    // the same value as FLASH_TARGET_OFFSET, defined separately for clarity.
    #define FLASH_RESERVED_SPACE FLASH_TARGET_OFFSET 

    // The maximum number of blocks allowed within the physical limits of the Flash memory.
    // This helps in ensuring we do not attempt to allocate more blocks than physically available.
    #define MAX_ALLOWED_BLOCKS (FLASH_MEMORY_SIZE_BYTES / FILESYSTEM_BLOCK_SIZE)



    // The number of blocks reserved at the beginning of the Flash memory.
    // This calculates how many blocks are taken up by the reserved space (FLASH_RESERVED_SPACE),
    // ensuring they are not used for storing user files.
    #define NUMBER_OF_RESERVED_BLOCKS (FLASH_RESERVED_SPACE / FILESYSTEM_BLOCK_SIZE)

    // The total usable size of the Flash memory for user data, after subtracting space reserved
    // for metadata or wear leveling. This helps in managing the actual available space for user files.
    #define FLASH_METADATA_SPACE (256 * 1024)  // Space reserved for metadata and wear leveling
    #define FLASH_USABLE_SPACE (FLASH_MEMORY_SIZE_BYTES - FLASH_METADATA_SPACE - FLASH_TARGET_OFFSET) // Usable space for user data
    
    // Defines how many directory entries can fit within a single block, calculated by dividing
    // the block size by the size of a single directory entry. This is used to optimize directory storage.
    #define ENTRIES_PER_BLOCK (FILESYSTEM_BLOCK_SIZE / sizeof(DirectoryEntry))

    // The maximum number of files the filesystem can support. This is determined by the available
    // space and how the filesystem is structured, ensuring a limit to prevent overallocation.
    #define MAX_FILES 20

    // Calculates the maximum size of a file, aligning it to the block size. This ensures that
    // file sizes are optimized for the block-based storage system, avoiding unnecessary fragmentation. 
    #define MAX_FILE_SIZE ((FLASH_USABLE_SPACE / MAX_FILES) & ~(FILESYSTEM_BLOCK_SIZE - 1))


    // A special value used in the FAT to indicate reserved entries. It can be set to an appropriate
    // value based on the system's needs, helping manage special cases or reserved blocks.
    #define FAT_ENTRY_RESERVED 0xFFFFFFFC // You can choose an appropriate value


    #define MAX_DIRECTORY_ENTRIES 20



    #define FAT_SUCCESS 0
    #define FAT_END_OF_CHAIN 1
    #define FAT_CORRUPTED -1
    #define FAT_OUT_OF_RANGE -2
    #define FAT_INVALID_OPERATION -3




#define READ_ERROR_NULL_POINTER -1
#define READ_ERROR_INVALID_SIZE -2
#define READ_ERROR_INCORRECT_MODE -3
#define READ_SUCCESS_NO_DATA 0

    #endif // FLASH_CONFIG_H
