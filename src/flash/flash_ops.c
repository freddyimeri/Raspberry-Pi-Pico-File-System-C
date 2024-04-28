#include <stdio.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
 
#include "pico/stdlib.h"
 

#include "../config/flash_config.h"    
#include "../flash/flash_ops.h"   
// Define the min macro if it's not already defined
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


void flash_write_safe_struct(uint32_t offset, flash_data_t *new_data) {
    uint32_t flash_offset = offset;

    printf("\n\n\nDebug section in write_safe_struct\n");
    printf("offset: %d\n", offset);
    printf("sizeof(flash_data_t): %d\n", sizeof(flash_data_t));
    printf("FLASH_TARGET_OFFSET: %d\n", FLASH_TARGET_OFFSET);
    printf("FLASH_MEMORY_SIZE_BYTES: %d\n", FLASH_MEMORY_SIZE_BYTES);
    printf("FLASH_SECTOR_SIZE: %d\n", FLASH_SECTOR_SIZE);
    printf("XIP_BASE: %d\n", XIP_BASE);
    printf("END Debug section in write_safe_struct\n\n\n");
    fflush(stdout);
    // Ensure we don't exceed flash memory limits
    if (offset + sizeof(flash_data_t) > FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
        printf("Error: Attempt to write beyond flash memory limits.\n");
        fflush(stdout);
        return;
    }
   
    uint32_t ints = save_and_disable_interrupts();
    // Temporary structure to hold the current data
    flash_data_t current_data;
    memset(&current_data, 0, sizeof(flash_data_t));
    // Read the existing data (if any)
    memcpy(&current_data, (const void *)(XIP_BASE + offset), sizeof(flash_data_t));
    // Increment the write count based on existing data
    new_data->write_count = current_data.write_count + 1;
    // Erase the sector before writing new data
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    // Write the new data, including the updated write count
    // Print the new data
    // printf("New Data:\n");
 

    // printf("Data to write: ");
    // fflush(stdout);
    // for(size_t i = 0; i < new_data->data_len; i++) {
    //     printf("%c", new_data->data[i]);
    // }
    // printf("\n");
    fflush(stdout);
    // Write the new data to flash memory
    flash_range_program(offset, (const uint8_t *)new_data, sizeof(flash_data_t));
    restore_interrupts(ints);
    printf("\n Debug flash_write_safe_struct : Data written to flash memory.\n");
}



 


void flash_write_safe(uint32_t offset, const uint8_t *data, size_t data_len) {
    flash_data_t flashData;
    flashData.write_count = 0; 
    memcpy(flashData.data, data, data_len);
    flashData.data_len = data_len;
    flash_write_safe_struct(offset, &flashData);
}




void flash_read_safe(uint32_t offset, uint8_t *buffer, size_t buffer_len) {
    // First, read the flash_data_t struct to understand the data layout
    flash_data_t flashData;
    flash_read_safe_struct(offset, &flashData);

    // Now that we have the flash_data_t, we can determine how much data to actually read
    // Ensure we don't read beyond the available data or the buffer's capacity
    size_t readLen = min(flashData.data_len, buffer_len);


    printf("\n\nDebug section in flash_read_safe\n");
  
    printf("Offset: %d\n", offset);
    printf("readLen %zu\n", readLen);
  
    
    printf("END Debug section in flash_read_safe\n\n");

    // memcpy(buffer, flashData.data, readLen);
}


void flash_read_safe_struct(uint32_t offset, flash_data_t *data) {
    printf("\n\nDebug section in flash_read_safe_struct\n");
    if (offset + sizeof(flash_data_t) > FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
        printf("Error: Attempt to read beyond flash memory limits.\n");
        fflush(stdout);
        return;
    }

    // Ensure we're reading from a valid area within the flash memory designated for user data
    uint32_t absoluteAddress = XIP_BASE + offset;
    printf("Absolute Address: %d\n", absoluteAddress);

    // Reading the entire flash_data_t struct from flash into the provided data pointer
    memcpy(data, (const void *)absoluteAddress, sizeof(flash_data_t));
     printf("END Debug section in flash_read_safe_struct\n\n");
}

void flash_erase_safe(uint32_t offset) {
    uint32_t ints = save_and_disable_interrupts();
    uint32_t sector_start = offset & ~(FLASH_SECTOR_SIZE - 1);
    if (sector_start < FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
    	flash_range_erase(sector_start, FLASH_SECTOR_SIZE);
    } 
    else 
    {
        printf("Error: Sector start address is out of bounds.\n");
        fflush(stdout);
        // Restore interrupts before returning due to error
        restore_interrupts(ints);
        return;
    }
    restore_interrupts(ints);
}


// Function: flash_read_safe2
// Reads data from flash memory into a buffer.
//
// Parameters:
// - offset: The offset from FLASH_TARGET_OFFSET where data is to be read.
// - buffer: Pointer to the buffer where read data will be stored.
// - buffer_len: Number of bytes to read.
//
// Note: The function performs bounds checking to ensure safe access.
void flash_read_safe2(uint32_t offset, uint8_t *buffer, size_t buffer_len) {

    // Calculate absolute flash offset
    uint32_t flash_offset =  offset;

    // printf("\n\n\nDebug section in flash_read_safe2\n");
    // printf("flash_offset: %d\n", flash_offset);
    // printf("offset: %d\n", offset);
    // printf("buffer_len: %d\n", buffer_len);
    // printf("END Debug section in flash_read_safe2\n\n\n");
 
    
    // Perform the memory copy from flash to buffer
    memcpy(buffer, (void *)(XIP_BASE + flash_offset), buffer_len);
}



// Function: flash_write_safe
// Writes data to flash memory at a specified offset, ensuring safety checks.
//
// Parameters:
// - offset: The offset from FLASH_TARGET_OFFSET where data is to be written.
// - data: Pointer to the data to be written.
// - data_len: Length of the data to be written.
//
// Note: This function erases the flash sector before writing new data.
// void flash_write_safe2(uint32_t offset, const uint8_t *data, size_t data_len) {

//     // Calculate absolute flash offset
//     uint32_t flash_offset =  offset;


//     ////////////////// EDITED BY ME //////////////////
 
//     ////////////////// EDITED BY ME //////////////////


//     printf("\n\n\nDebug section in flash_write_safe2\n");
//     printf("flash_offset: %d\n", flash_offset);
//     printf("offset: %d\n", offset);
//     printf("data_len: %d\n", data_len);
//     printf("FLASH_SECTOR_SIZE: %d\n", FLASH_SECTOR_SIZE);
//     printf("END Debug section in flash_write_safe2\n\n\n");
//     // Disable interrupts for a safe flash operation
//     uint32_t ints = save_and_disable_interrupts();

//     // Erase the flash sector before writing
//     flash_range_erase(flash_offset, data_len+1);

//     // Write data to flash
//     flash_range_program(flash_offset, data, data_len);

//     // Restore interrupts
//     restore_interrupts(ints);
//     printf("\n Debug flash_write_safe2 : Data written to flash memoryaaaaaaa.\n");
//     fflush(stdout);
    
// }
void flash_write_safe2(uint32_t offset, const uint8_t *data, size_t data_len) {

    // Calculate absolute flash offset
    uint32_t flash_offset =  offset;


    printf("\n\n\nDebug section in flash_write_safe2\n");
    printf("flash_offset: %d\n", flash_offset);
    printf("offset: %d\n", offset);
    printf("data_len: %d\n", data_len);
    printf("FLASH_SECTOR_SIZE: %d\n", FLASH_SECTOR_SIZE);
    printf("END Debug section in flash_write_safe2\n\n\n");
    // Disable interrupts for a safe flash operation
    uint32_t ints = save_and_disable_interrupts();

    // Erase the flash sector before writing
    flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);

    // Write data to flash
    flash_range_program(flash_offset, data, data_len);

    // Restore interrupts
    restore_interrupts(ints);
    printf("\n Debug flash_write_safe2 : Data written to flash memoryaaaaaaa.\n");
    fflush(stdout);
    
}



// void flash_write_safe2(uint32_t offset, const uint8_t *data, size_t data_len) {
//     printf("\nStarting flash_write_safe2\n");

//     // Ensure offset and length are sector aligned
//     uint32_t flash_offset = offset; // Assuming offset is already sector aligned
//     uint32_t aligned_len = ((data_len + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
//     printf("Data length: %u, Aligned length: %u\n", data_len, aligned_len);

//     printf("Debug section in flash_write_safe2\n");
//     printf("flash_offset: %u\n", flash_offset);
//     printf("data_len: %u, aligned_len: %u\n", data_len, aligned_len);
//     printf("FLASH_SECTOR_SIZE: %u\n", FLASH_SECTOR_SIZE);
//     printf("END Debug section in flash_write_safe2\n");

//     // Disable interrupts for a safe flash operation
//     uint32_t ints = save_and_disable_interrupts();

//     // Erase the flash sector before writing
//     flash_range_erase(flash_offset, aligned_len);
      

//     flash_range_program(flash_offset, data, data_len);
//   restore_interrupts(ints);
//      sleep_ms(300);
//     // Restore interrupts
 
    
//     printf("Data written to flash memory.\n");
// }

void flash_write_safe3(uint32_t offset, const uint8_t *data, size_t data_len) {
    printf("\nStarting flash_write_safe33333333333333333333333333\n");

 
    printf("Debug section in flash_write_safe2\n");
    printf("flash_offset: %u\n", offset);
 
    printf("FLASH_SECTOR_SIZE: %u\n", FLASH_SECTOR_SIZE);
    printf("END Debug section in flash_write_safe2\n");
    printf("Data length: %u\n", data_len);
    printf("Data: %s\n", data);


    // Disable interrupts for a safe flash operation
    uint32_t ints = save_and_disable_interrupts();

    // Erase the flash sector before writing
    flash_range_erase(offset, FLASH_SECTOR_SIZE);



    flash_range_program(offset, data, data_len);
       restore_interrupts(ints);

     sleep_ms(300);
    // Restore interrupts
 
    
    printf("Data written to flash memory.\n");
}

void flash_erase_safe2(uint32_t offset) {
    uint32_t ints = save_and_disable_interrupts();
    uint32_t sector_start = offset;;
    if (sector_start < FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
    	flash_range_erase(offset, FLASH_SECTOR_SIZE);
    } 
    else 
    {
        printf("Error: Sector start address is out of bounds.\n");
        fflush(stdout);
        // Restore interrupts before returning due to error
        restore_interrupts(ints);
        return;
    }
    restore_interrupts(ints);
}
/*
Function fs_write(file, buffer, size):
    // Check for invalid input parameters
    If file is NULL or buffer is NULL or size is less than or equal to 0:
        Return -1 // Error code for invalid input

    // Ensure the file is opened in either append ('a') or write ('w') mode
    If file's mode is not 'a' and file's mode is not 'w':
        Return -1 // Error code for incorrect file mode

    // Initialize local variables for tracking the current block and data pointer
    Initialize blockIndex to file's start_block
    Initialize dataPtr to point to the start of the buffer
    Initialize remainingSize to size
    Initialize totalWritten to 0

    // If appending, find the last block of the file
    If file's mode is 'a' and blockIndex is not NO_FREE_BLOCKS:
        While blockIndex is not FAT_ENTRY_END:
            Load block from blockIndex
            If the next_block of the loaded block is not FAT_ENTRY_END:
                Update blockIndex to the next_block of the loaded block
            Else:
                Break from the loop // Found the last block

    // If writing, potentially deallocate old blocks and allocate a new start block
    Else if file's mode is 'w':
        Optionally deallocate all blocks associated with this file
        Allocate a new block and set file's start_block to this new block
        Update blockIndex to this new start block

    // Read the last or new block into 'block' (initial or after finding/appending)
    Load block from blockIndex
    Initialize startOffset to block's size (amount of data already in the block)

    // Loop to write all data from buffer to storage
    While remainingSize > 0:
        Calculate spaceInCurrentBlock as BLOCK_DATA_SIZE - startOffset
        Calculate toWrite as the minimum of spaceInCurrentBlock and remainingSize

        // Check if current block has space to write
        If toWrite <= 0:
            Allocate a new block and store its index in newBlockIndex
            Set the next_block of the current block to newBlockIndex
            Write the current block back to storage to update its next_block link
            Update blockIndex to newBlockIndex
            Reset startOffset to 0 for the new block
            Set block's size to 0 and next_block to FAT_ENTRY_END
            Recalculate spaceInCurrentBlock and toWrite for the new block

        // Copy data from buffer to block's data starting at the current offset
        Copy data from dataPtr to block's data at startOffset for toWrite bytes
        Increment block's size by toWrite
        Increment startOffset by toWrite
        Update dataPtr to point to the next part of the input buffer
        Decrease remainingSize by toWrite
        Increment totalWritten by toWrite

        // Write the updated block back to storage
        Write block to storage at blockIndex

    // Return the total number of bytes successfully written
    Return totalWritten
*/