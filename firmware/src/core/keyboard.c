// travmonkey
// Process array into keycodes and store in struct
// Access them later with the memory address
#include "class/hid/hid_device.h"
#include <pico/types.h>
#include <stdint.h>
#include <string.h>

#include "keyboard.h"
#include "keymap.h"
#include "keymap_shorthands.h"
#include "layer_processor.h"
#include "matrix.h"
#include "usb_descriptors.h"

void keyboard_init(keymap_t *const keymap, matrix_metadata_t *const metadata) {
  // Initialize the keyboard
  initialize_keymaps(keymap);
  store_layer_indices(keymap);
  select_matrix_backend();
  matrix_init(metadata); // Init gpio pins for reading and writing
}

void initialize_report_t(keycode_report_t *const report) {
  memset(report, 0, sizeof(keycode_report_t)); // Clear the report struct
}

void clear_current_report(keycode_report_t *const report) {
  memset(report->keycodes, 0, MAX_KEYCODES);
  report->count = 0;
  report->modifier = 0;
  report->media_key = 0;
  // media_key_active is static and should not be cleared
}

uint8_t check_locked_index(locked_keycodes_t *const previous_report,
                           uint8_t current_index) {
  for (uint8_t index = 0; index < MAX_KEYCODES; ++index) {
    if (current_index == previous_report->indices[index]) {
      return previous_report->keycodes[index];
    }
  }
  return 0;
}

void lock_keycode(locked_keycodes_t *const previous_report, uint8_t keycode,
                  uint8_t current_index) {
  previous_report->keycodes[previous_report->count] = keycode;
  previous_report->indices[previous_report->count++] = current_index;
}

void set_keycodes(keymap_t const *const keymap, keycode_report_t *const report,
                  matrix_state_t const *const state) {

  for (uint8_t index = 0; index < state->total_activated_keys; ++index) {
    if (state->activated_keys[index] < MATRIX_SIZE) {
      // Check if this key was already pressed in the previous scan
      uint8_t locked_keycode = check_locked_index(&report->previous_keycodes, 
                                                  state->activated_keys[index]);
      
      if (locked_keycode != 0) {
        // Key was already pressed - use the locked keycode
        lock_keycode(&report->current_keycodes, locked_keycode, 
                     state->activated_keys[index]);
        key_handler(report, locked_keycode);
      } else {
        // New key press - use current layer's keycode
        uint8_t current_keycode = keymap->active_keymap[state->activated_keys[index]];
        lock_keycode(&report->current_keycodes, current_keycode, 
                     state->activated_keys[index]);
        key_handler(report, current_keycode);
      }
    }
  }
}

void reset_locked_keycodes(keycode_report_t *const report) {
  memset(&report->previous_keycodes, 0, sizeof(locked_keycodes_t));
  memcpy(&report->previous_keycodes, &report->current_keycodes,
         sizeof(locked_keycodes_t));
  memset(&report->current_keycodes, 0, sizeof(locked_keycodes_t));
}

uint8_t set_modifier_bit(uint8_t keycode) {
  switch (keycode) {
  case KC_LCTL:
    return 0;
  case KC_LSFT:
    return 1;
  case KC_LALT:
    return 2;
  case KC_LGUI:
    return 3;
  case KC_RCTL:
    return 4;
  case KC_RSFT:
    return 5;
  case KC_RALT:
    return 6;
  case KC_RGUI:
    return 7;
  default:
    return 0xFF;
  }
}

// TODO: make this dynamic
// TODO: Support modes
bool is_layer_key(uint8_t keycode) {
  switch (keycode) {
  case LAYER_KEY_M01:
  case LAYER_KEY_M02:
  case LAYER_KEY_M03:
  case LAYER_KEY_M04:
  case LAYER_KEY_M05:
  case LAYER_KEY_M06:
  case LAYER_KEY_M07:
    return true;
  default:
    return false;
  }
}

void key_handler(keycode_report_t *const report, uint8_t keycode) {
  uint8_t modifier_bit = set_modifier_bit(keycode);
  if (modifier_bit != 0xFF) {
    report->modifier |= (1 << modifier_bit);
    return;
  }
  if (is_layer_key(keycode)) {
    return;
  }
  // TODO: make this dynamic
  switch (keycode) {
  case KC_BRID:
  case KC_BRIP:
  case KC_MPRV:
  case KC_MNXT:
  case KC_MPLY:
    report->media_key = keycode;
    return;
  default:
    if (report->count < MAX_KEYCODES) {
      report->keycodes[report->count++] = keycode;
      return;
    }
  }
}

void send_keyboard_report(keycode_report_t *const report) {
  if (tud_hid_ready()) {
    if (!report->media_key_active && report->media_key != 0) {
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &report->media_key,
                     sizeof(report->media_key));
      report->media_key_active = true;
    } else if (report->media_key_active && report->media_key == 0) {
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &report->media_key,
                     sizeof(report->media_key));
      report->media_key_active = false;
    }
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report->modifier,
                            report->keycodes);
  }
  reset_locked_keycodes(report);
  clear_current_report(report);
}
