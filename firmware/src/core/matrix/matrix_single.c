// travmonkey
// Define functions to read keyboard matrix

#include <pico/time.h>
#include <pico/types.h>
#include <stddef.h>
#include <stdint.h>
#include "matrix.h"
#include <hardware/gpio.h>
#include <string.h>

void matrix_init_single(matrix_metadata_t *const metadata) {
  // Init the row pins; set them to output
  for (uint8_t pin_index = 0; pin_index < TOTAL_ROWS; pin_index++) {
    gpio_init(metadata->row_pins_full[pin_index]);
    gpio_set_dir(metadata->row_pins_full[pin_index], GPIO_OUT);
  }

  // Init the column pins; set them to input
  for (uint8_t pin_index = 0; pin_index < TOTAL_COLS; pin_index++) {
    gpio_init(metadata->col_pins_full[pin_index]);
    gpio_set_dir(metadata->col_pins_full[pin_index], GPIO_IN);
    gpio_pull_down(metadata->col_pins_full[pin_index]);
  }
};

void matrix_single_set_metadata(matrix_metadata_t *const metadata) {
  // Initialize the metadata for single matrix
  // TODO: this is broken due to metadata->row_pins_full and col_pins_full being half length
  matrix_initialize_metadata(metadata); // Clear the metadata struct
  metadata->half = 0; // Single
  memcpy(metadata->row_pins_full, row_pins, sizeof(metadata->row_pins_full));
  memcpy(metadata->col_pins_full, col_pins, sizeof(metadata->col_pins_full));
};

void matrix_read_single(matrix_state_t *const state, matrix_metadata_t *const metadata) {
  // Row doesn't matter, but setting column here instead of in the loop is ideal
  uint8_t row;
  uint8_t col;
  uint8_t destination_index = 0;
  // Perform a scan of the matrix
  // We will pull row pins high, then read 
  for (row = 0; row < TOTAL_ROWS; row++) {
    gpio_put(metadata->row_pins_full[row], 1);
    sleep_us(1); // Slight delay to increase accuracy
    
    // Read every column pin to check for button presses
    for (col = 0; col < TOTAL_COLS; col++) {
      state->state[destination_index++] = gpio_get(metadata->col_pins_full[col]);
    }
    // Stop scanning the row
    gpio_put(metadata->row_pins_full[row], 0);
  }
};
