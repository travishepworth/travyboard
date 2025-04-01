// travmonkey

#include "keyboard.hpp"
#include "bsp/board.h"
#include "class/hid/hid.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "keymap.hpp"
#include "pico/time.h"
#include "usb_descriptors.h"

#define LEFT 0
#define RIGHT 1

KeyMap left_keymap(LEFT);
KeyMap right_keymap(RIGHT);

// Variable to hold keys ready for keypress send
static uint8_t staged_keys[6];
// Variable to hold pressed keys before layer is applied
static uint8_t held_keys[6][3]; // 2nd array hold row, col, half
static int total_keys;          // Store total amount of keys presses

static bool left_layer = false;  // Store if left layer key is pressed
static bool right_layer = false; // Store if right layer key is pressed

KeyBoard::KeyBoard(const std::int8_t &name) {
  if (name == RIGHT) {
    // Initialize right half of keyboard
    // Initialize the row pins as the output
    for (uint8_t PIN : RIGHT_ROW_PINS) {
      gpio_init(PIN);
      gpio_set_dir(PIN, GPIO_OUT);
    }
    // Initialize the column pins as the input, and pull them down
    for (uint8_t PIN : RIGHT_COLUMN_PINS) {
      gpio_init(PIN);
      gpio_set_dir(PIN, GPIO_IN);
      gpio_pull_down(PIN);
    }
  } else if (name == LEFT) {
    // Initialize left half of keyboard
    // Initialize the row pins as the output
    for (uint8_t PIN : LEFT_ROW_PINS) {
      gpio_init(PIN);
      gpio_set_dir(PIN, GPIO_OUT);
    }
    // Initialize the column pins as the input, and pull them down
    for (uint8_t PIN : LEFT_COLUMN_PINS) {
      gpio_init(PIN);
      gpio_set_dir(PIN, GPIO_IN);
      gpio_pull_down(PIN);
    }
  }
}

void KeyBoard::scan_buttons() {
  // Poll every 1ms
  const uint32_t interval_ms = 1;
  static uint32_t start_ms = 0;

  // Check for time since last poll
  if (board_millis() - start_ms < interval_ms) {
    return; // not enough time
  }
  start_ms += interval_ms;

  refresh(); // Refresh all static variables

  handle_uart(); // Recieve and handle UART buffer

  // Loop over every row
  for (uint8_t row = 0; row < sizeof(KeyBoard::RIGHT_ROW_PINS); row++) {
    // Start scanning current row
    gpio_put(KeyBoard::RIGHT_ROW_PINS[row], 1);
    sleep_us(1); // Small delay for accuracy

    // Loop over every column
    for (uint8_t col = 0; col < sizeof(KeyBoard::RIGHT_COLUMN_PINS); col++) {
      if (gpio_get(KeyBoard::RIGHT_COLUMN_PINS[col]) && total_keys < 6) {
        button_handler(row, col, 'R'); // Handle the button press
      }
    }
    // Stop scanning current row
    gpio_put(KeyBoard::RIGHT_ROW_PINS[row], 0);
  }

  send_keyboard_report(check_layer()); // Send the keypress with current layer
}

void KeyBoard::button_handler(uint8_t row, uint8_t col, char half) {
  // Function to handle button presses and check for layer keys
  // TODO - Implement macro detection and functionality
  uint8_t keycode;
  uint8_t keyboard_half;
  // Temp variable to store the keycode
  if (half == 'R') {
    keycode = right_keymap.return_keycode(row, col, 0);
    keyboard_half = RIGHT_HALF;
  } else { // Half will be left if not right
    keycode = left_keymap.return_keycode(row, col, 0);
    keyboard_half = LEFT_HALF;
  }

  // Check if keycode is a layer key and set that key as held
  if (keycode == 1) {
    left_layer = true;
    return; // Leave to prevent adding key to held keys
  } else if (keycode == 2) {
    right_layer = true;
    return; // Leave to prevent adding key to held keys
  }

  // Add the key to the current keys if not layer key
  held_keys[total_keys][0] = row;
  held_keys[total_keys][1] = col;
  held_keys[total_keys][2] = keyboard_half;
  total_keys += 1;
}

void KeyBoard::handle_uart(void) {
  // While loop to recieve everything in buffer, or until max keystrokes is hit
  // Remaining in buffer will be handled in next send of keys
  while (uart_is_readable(UART_ID) && total_keys < 6) {
    // Recieve next packet in the buffer from uart
    uint8_t packet = uart_getc(UART_ID);
    uint8_t row = (packet >> 4);   // Bitshift to get row value
    uint8_t col = (packet & 0x0f); // & 0x0f returns the col value

    button_handler(row, col, 'L'); // Handle the button press
  }
}

uint8_t KeyBoard::check_layer(void) {
  // Return value of the layer based off of static booleans
  if (left_layer && right_layer) {
    return 3;
  } else if (left_layer) {
    return 1;
  } else if (right_layer) {
    return 2;
  }

  return 0; // Return default layer if both false
}

void KeyBoard::send_keyboard_report(uint8_t layer) {
  // Send keypresses for held keys
  uint8_t keycode;       // Initialize keycode variable
  uint8_t modifier = 0;  // Initialize modifier variable
  static uint16_t media_key = 0; // Initialize media key variable
  static bool media_pressed = false; // Variable to check if media key is pressed
  bool media_key_active = false;

  // Loop through all of the held keys
  for (uint8_t keys = 0; keys < total_keys; keys++) {
    uint8_t row = held_keys[keys][0];    // Set row
    uint8_t column = held_keys[keys][1]; // Set column
    uint8_t half = held_keys[keys][2];

    // Get Keycode from keymap depending on which half it is
    if (half == LEFT_HALF) {
      keycode = left_keymap.return_keycode(row, column, layer);
    } else if (half == RIGHT_HALF) {
      keycode = right_keymap.return_keycode(row, column, layer);
    } else {
      keycode = NULL_VALUE; // Return null if anything else
    }

    // Skip Invalid Keycodes | Layers shouldn't make it here but just in case
    if (row == NULL_VALUE || keycode == 1 || keycode == 2) {
      continue;
    }

    switch (keycode) {
      // Check for modifier keys
    case 0xE8:
      modifier |= KEYBOARD_MODIFIER_LEFTGUI;
      break;
    case 0xE9:
      modifier |= KEYBOARD_MODIFIER_LEFTALT;
      break;

      // Check for media keys
    case HID_USAGE_CONSUMER_PLAY_PAUSE:
      media_key = keycode;
      media_key_active = true; // Set media key active to true
      break;
    case HID_USAGE_CONSUMER_SCAN_PREVIOUS:
      media_key = keycode;
      media_key_active = true; // Set media key active to true
      break;
    case HID_USAGE_CONSUMER_SCAN_NEXT:
      media_key = keycode;
      media_key_active = true; // Set media key active to true
      break;
    case HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT:
      media_key = keycode;
      media_key_active = true; // Set media key active to true
      break;
    case HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT:
      media_key = keycode;
      media_key_active = true; // Set media key active to true
      break;

      // Check for other keys

    default:
      // Ensure the same keycode doesn't get added twice
      bool found = false;
      for (uint8_t code : staged_keys) {
        if (code == keycode) {
          found = true;
          break;
        }
      }

      // Add the key to the staged_keys array
      if (!found) {
        staged_keys[total_keys] = keycode;
        total_keys++;
      }
    }
  }

  // Send keypress if HID is ready
  if (tud_hid_ready()) {

    if (media_key_active && !media_pressed) {
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &media_key, sizeof(media_key));
      media_pressed = true; // Reset media key pressed to false
    } else if (!media_key_active && media_pressed) {
      // Send empty report to clear the media key
      media_key = 0;
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &media_key, sizeof(media_key));
      media_pressed = false; // Reset media key pressed to false
    }

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, staged_keys);
    // Make sure memory is completely clear just in case
    memset(staged_keys, 0, sizeof(staged_keys));
  }
}

Keys KeyBoard::scan_pins(void) {
  // Poll every 1ms
  const uint32_t interval_ms = 1;
  static uint32_t start_ms = 0;

  Keys keys;

  // Check for time since last poll
  if (board_millis() - start_ms < interval_ms) {
    return keys; // not enough time
  }
  start_ms += interval_ms;

  // Define the total amount of keys pressed. This is a max of 6
  uint total_keys = 0;

  // Loop over every row
  for (uint8_t row = 0; row < sizeof(KeyBoard::LEFT_ROW_PINS); row++) {
    // Start scanning current row
    gpio_put(KeyBoard::LEFT_ROW_PINS[row], 1);
    sleep_us(1); // Small delay for accuracy

    // Loop over every column
    for (uint8_t col = 0; col < sizeof(KeyBoard::LEFT_COLUMN_PINS); col++) {
      if (gpio_get(KeyBoard::LEFT_COLUMN_PINS[col]) && total_keys < 6) {
        // Scan columns and add the key to the current keys
        keys[total_keys].row = row;
        keys[total_keys].col = col;
        total_keys += 1;
      }
    }

    // Stop scanning current row
    gpio_put(KeyBoard::LEFT_ROW_PINS[row], 0);
  }

  return keys;
}

void KeyBoard::refresh(void) {
  // Reset the staged_keys array
  for (size_t i = 0; i < 6; i++) {
    staged_keys[i] = 0;
  }

  // Reset the held_keys array
  for (size_t i = 0; i < 6; i++) {
    held_keys[i][0] = NULL_VALUE;
    held_keys[i][1] = NULL_VALUE;
    held_keys[i][2] = NULL_VALUE;
  }

  // Reset the total amount of keys pressed. This is a max of 6
  total_keys = 0;

  // Reset layer booleans to false
  left_layer = false;
  right_layer = false;
}
