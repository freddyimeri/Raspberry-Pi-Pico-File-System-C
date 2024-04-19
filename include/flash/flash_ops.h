#ifndef FLASH_OPS_H
#define FLASH_OPS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t write_count;
    uint8_t data[256];
    size_t data_len;
} flash_data_t;

// Original function signatures expected by cli.c
void flash_write_safe(uint32_t offset, const uint8_t *data, size_t data_len);
void flash_read_safe(uint32_t offset, uint8_t *buffer, size_t buffer_len);
void flash_erase_safe(uint32_t offset);




void flash_read_safe2(uint32_t offset, uint8_t *buffer, size_t buffer_len);
void flash_write_safe2(uint32_t offset, const uint8_t *data, size_t data_len);
// Actual implementations that work with flash_data_t
void flash_write_safe_struct(uint32_t offset, flash_data_t *data);
void flash_read_safe_struct(uint32_t offset, flash_data_t *data);

void flash_erase_safe2(uint32_t offset);





#endif // FLASH_OPS_H
