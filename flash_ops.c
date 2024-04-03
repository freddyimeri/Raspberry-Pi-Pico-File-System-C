#include "flash_ops.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#define FLASH_TARGET_OFFSET (256 * 1024) // Offset where user data starts (256KB into flash)
#define FLASH_SIZE PICO_FLASH_SIZE_BYTES // Total flash size available


void flash_write_safe_struct(uint32_t offset, flash_data_t *new_data) {
    uint32_t flash_offset = FLASH_TARGET_OFFSET + offset;
    // Ensure we don't exceed flash memory limits
    if (flash_offset + sizeof(flash_data_t) > FLASH_TARGET_OFFSET + FLASH_SIZE) {
        printf("Error: Attempt to write beyond flash memory limits.\n");
        return;
    }
    uint32_t ints = save_and_disable_interrupts();
    // Temporary structure to hold the current data
    flash_data_t current_data;
    memset(&current_data, 0, sizeof(flash_data_t));
    // Read the existing data (if any)
    memcpy(&current_data, (const void *)(XIP_BASE + flash_offset), sizeof(flash_data_t));
    // Increment the write count based on existing data
    new_data->write_count = current_data.write_count + 1;
    // Erase the sector before writing new data
    flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);
    // Write the new data, including the updated write count
    flash_range_program(flash_offset, (const uint8_t *)new_data, sizeof(flash_data_t));
    restore_interrupts(ints);
}

void flash_read_safe_struct(uint32_t offset, flash_data_t *data) {
    uint32_t flash_offset = FLASH_TARGET_OFFSET + offset;

    // Bounds checking
    if (flash_offset + sizeof(flash_data_t) > FLASH_TARGET_OFFSET + FLASH_SIZE) {
        printf("Error: Attempt to read beyond flash memory limits.\n");
        return;
    }
    // Reading Data
    memcpy(data, (const void *)(XIP_BASE + flash_offset), sizeof(flash_data_t));
}



void flash_write_safe(uint32_t offset, const uint8_t *data, size_t data_len) {
    flash_data_t flashData;
    flashData.write_count = 0; 
    memcpy(flashData.data, data, data_len);
    flashData.data_len = data_len;
    flash_write_safe_struct(offset, &flashData);
}

// Adapter for flash_read_safe to work with cli.c without modifications
void flash_read_safe(uint32_t offset, uint8_t *buffer, size_t buffer_len) {
    flash_data_t flashData;
    flash_read_safe_struct(offset, &flashData);

    if (buffer_len > flashData.data_len) buffer_len = flashData.data_len;
    memcpy(buffer, flashData.data, buffer_len);
}

void flash_erase_safe(uint32_t offset) {
    uint32_t flash_offset = FLASH_TARGET_OFFSET + offset;
    uint32_t ints = save_and_disable_interrupts();
    uint32_t sector_start = flash_offset & ~(FLASH_SECTOR_SIZE - 1);
    if (sector_start < FLASH_TARGET_OFFSET + FLASH_SIZE) {
    	flash_range_erase(sector_start, FLASH_SECTOR_SIZE);
    } 
    else 
    {
        printf("Error: Sector start address is out of bounds.\n");
        // Restore interrupts before returning due to error
        restore_interrupts(ints);
        return;
    }
    restore_interrupts(ints);
}