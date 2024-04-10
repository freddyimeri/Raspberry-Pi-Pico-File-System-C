#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "flash_config.h"




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



void flash_read_safe_struct(uint32_t offset, flash_data_t *data) {
    if (offset + sizeof(flash_data_t) > FLASH_TARGET_OFFSET + FLASH_MEMORY_SIZE_BYTES) {
        printf("Error: Attempt to read beyond flash memory limits.\n");
        fflush(stdout);
        return;
    }

    // Ensure we're reading from a valid area within the flash memory designated for user data
    uint32_t absoluteAddress = XIP_BASE + offset;

    // Reading the entire flash_data_t struct from flash into the provided data pointer
    memcpy(data, (const void *)absoluteAddress, sizeof(flash_data_t));
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


    // printf("\n\nDebug section in flash_read_safe\n");
  
    // printf("Offset: %d\n", offset);
    // printf("readLen %zu\n", readLen);
  
    
    // printf("END Debug section in flash_read_safe\n\n");

    // printf("Data read: ");
    // for(size_t i = 0; i < readLen; i++) {
    //     printf("%c", buffer[i]);
    // }
    // printf("\n");
    // fflush(stdout);

    // Copy the data from the struct to the provided buffer
    memcpy(buffer, flashData.data, readLen);
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



