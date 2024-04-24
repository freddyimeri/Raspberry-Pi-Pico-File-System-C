#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdlib.h>
#include <stdio.h>

static int random_initialized = 0;  // Flag to check if random generator has been initialized

// Function to initialize random number generation
void init_random() {
    if (!random_initialized) {
        adc_init();
        adc_select_input(0);  // Select an unconnected ADC input for noise

        // Read an analog value for seeding the random number generator
        uint16_t noise = adc_read();
        srand(noise);

        random_initialized = 1;  // Set the initialized flag
    }
}

// Function to generate a unique number between 1000 and 4,294,967,294 (one less than uint32_t max)
uint32_t generateUniqueId() {
    if (!random_initialized) {
        init_random();  // Ensure random number generator is initialized
    }
    // Calculate a range from 1000 to 4,294,967,294
    uint32_t range = 4294967294u - 1000 + 1;
    return rand() % range + 1000;
}

int main() {
    stdio_init_all();


    sleep_ms(2000);
    
        while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    
    for (int i = 15; i >= 1; i--) {
        printf("Operation starts in: %d Seconds\n", i);
        sleep_ms(1000);
    }


    // Loop to generate and print 10 unique IDs
    for (int i = 0; i < 10; i++) {
        uint32_t uniqueId = generateUniqueId();
        printf("Unique ID %d: %u\n", i + 1, uniqueId);
    }

    return 0;
}



