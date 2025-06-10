/*--Main--*/
#include "bsp/board_api.h"
#include <hardware/uart.h>

#include "debounce.h"
#include "tusb.h"

#include "keyboard.h"
#include "keymap.h"
#include "matrix.h"
#include "uart.h"

static keymap_t keymap;
static keycode_report_t report;
static matrix_metadata_t metadata;
static debounce_state_t debounce_state;
static matrix_state_t left_state;
static uart_report_t uart_report;

int main(void) {
  board_init();
  tusb_init();

  initialize_uart();
  initialize_uart_report(&uart_report);

  keyboard_init(&keymap, &metadata);
  debounce_init(&debounce_state);
  matrix_clear(&left_state);

  const uint32_t interval_ms = 1;
  static uint32_t start_ms = 0;

  while (true) {
    tud_task();
    uint32_t current_ms = board_millis();
    if (current_ms - start_ms >= interval_ms) {
      start_ms = current_ms;

      matrix_state_t true_state;
      matrix_state_t right_state;
      matrix_clear(&true_state);
      matrix_clear(&right_state);

      // TODO: move to function, a bit too much logic for main
      if (uart_is_readable(UART_ID)) {
        clear_uart_report(&uart_report);
        matrix_state_t incoming_state;
        matrix_clear(&incoming_state);

        if (receive_uart_report(&incoming_state, &uart_report)) {
          matrix_copy(&left_state, incoming_state);
        }
      }
      matrix_copy(&true_state, left_state);

      matrix_read(&right_state, &metadata);
      debounce_matrix(&right_state, &debounce_state);
      // TODO: make this take 3 arguments, instead of copying right to true
      matrix_concatenate(&true_state, &right_state);

      // Remote wakeup logic

      matrix_convert(&true_state); // Extract array of indices from scan

      set_layer(&keymap, get_active_layer(true_state, &keymap.layer_indices));

      set_keycodes(&keymap, &report, &true_state);
      send_keyboard_report(&report);
    }
  }
  return 0;
};
