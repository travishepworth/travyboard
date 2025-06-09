#include <pico/time.h>
#include <pico/types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "default.h"
#include "device/usbd.h"
#include "matrix.h"
#include <hardware/gpio.h>

// INIT: Split implementation
void matrix_init_split(matrix_metadata_t *const metadata) {
  matrix_split_set_metadata(metadata);
  for (uint8_t pin_index = 0; pin_index < TOTAL_ROWS_HALF; ++pin_index) {
    gpio_init(metadata->row_pins_half[pin_index]);
    gpio_set_dir(metadata->row_pins_half[pin_index], GPIO_OUT);
  }

  for (uint8_t pin_index = 0; pin_index < TOTAL_COLS_HALF; ++pin_index) {
    gpio_init(metadata->col_pins_half[pin_index]);
    gpio_set_dir(metadata->col_pins_half[pin_index], GPIO_IN);
    gpio_pull_down(metadata->col_pins_half[pin_index]);
  }
};

// INIT: set the metadata for the half
void matrix_split_set_metadata(matrix_metadata_t *const metadata) {
  matrix_initialize_metadata(metadata);
  matrix_detect_half(metadata);
  if (metadata->half == RIGHT_HALF) {
    memcpy(metadata->row_pins_half, &row_pins[TOTAL_ROWS_HALF], sizeof(metadata->row_pins_half));
    memcpy(metadata->col_pins_half, &col_pins[TOTAL_COLS_HALF], sizeof(metadata->col_pins_half));

  } else {
    memcpy(metadata->row_pins_half, row_pins, sizeof(metadata->row_pins_half));
    memcpy(metadata->col_pins_half, col_pins, sizeof(metadata->col_pins_half));
  }
}

// TODO: Make this work without usb detection
void matrix_detect_half(matrix_metadata_t *const metadata) {
  metadata->half = RIGHT_HALF;
  // if (tud_connected()) {
  //   metadata->half = RIGHT_HALF;
  // } else {
  //   metadata->half = LEFT_HALF;
  // }
}

// CORE: Read function for split backend
void matrix_read_split(matrix_state_t *const state, matrix_metadata_t *const metadata) {
  uint8_t row;
  uint8_t col;
  uint8_t destination_index = 0;

  // Perform a scan of the matrix
  // We will pull row pins high, then read 
  for (row = 0; row < TOTAL_ROWS_HALF; ++row) {
    gpio_put(metadata->row_pins_half[row], 1);
    sleep_us(1);
    
    // Read every column pin to check for button presses
    for (col = 0; col < TOTAL_COLS_HALF; ++col) {
      state->state[destination_index++] = gpio_get(metadata->col_pins_half[col]);
    }

    // Stop scanning the row
    gpio_put(metadata->row_pins_half[row], 0);
  }
};

// HELPER: Add one matrix to the other
void matrix_concatenate(matrix_state_t *const state, const matrix_state_t *right_state) {
  memcpy(&state->state[MATRIX_HALF_SIZE], right_state->state, MATRIX_HALF_SIZE);
}
