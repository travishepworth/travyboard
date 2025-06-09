#ifndef DEFAULT_H
#define DEFAULT_H

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_MODE_SPLIT true // Set to true for split keyboard mode, false for single half

enum KEYMAP_TOTALS {
  TOTAL_ROWS = 8, // Total rows for the entire keyboard (double for split keyboards)
  TOTAL_COLS = 14, // Total columns for the entire keyboard
  TOTAL_LAYERS = 4, // Total layers on this keymap
  TOTAL_ROWS_HALF = (TOTAL_ROWS / 2), // Half rows for split keyboards
  TOTAL_COLS_HALF = (TOTAL_COLS / 2), // Half columns for split keyboards
  LEFT_HALF = 0, // Left half of the keyboard
  RIGHT_HALF = 1, // Right half of the keyboard
  TOTAL_KEYS = (TOTAL_COLS * TOTAL_ROWS),
  TOTAL_KEYS_HALF = (TOTAL_KEYS / 2)
};

extern const uint8_t row_pins[TOTAL_ROWS];
extern const uint8_t col_pins[TOTAL_COLS];
extern const uint8_t keymap_default[TOTAL_LAYERS][TOTAL_ROWS_HALF][TOTAL_COLS];

#endif // DEFAULT_H
