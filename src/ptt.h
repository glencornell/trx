#ifndef PTT_H_INCLUDED
#define PTT_H_INCLUDED

#include <stdbool.h>
#include <linux/input-event-codes.h>

typedef enum {
  PTT_INPUT_SOURCE_DEV_INPUT,
  PTT_INPUT_SOURCE_GPIO
} ptt_input_source_t;

#define DEFAULT_PTT_ENABLED false

#define DEFAULT_PTT_INPUT_SOURCE PTT_INPUT_SOURCE_DEV_INPUT
#define DEFAULT_PTT_DEV_INPUT_KEYCODE KEY_LEFTCTRL
#define DEFAULT_PTT_DEV_INPUT_DEVICE "/dev/input/by-path/platform-i8042-serio-0-event-kbd"

#define DEFAULT_PTT_GPIO_DEVICE "/dev/gpiochip0"
#define DEFAULT_PTT_GPIO_PIN 1

struct ptt_s;
typedef struct ptt_s ptt_t;

// Callbacks that are invoked when the PTT button changes state
typedef void (*ptt_pressed_cb_t)(ptt_t *ptt, void *user_data);
typedef void (*ptt_released_cb_t)(ptt_t *ptt, void *user_data);

// Simple constructor, using defaults (/dev/input source, Left Ctrl Key as the PTT button):
ptt_t *ptt_create_simple();

// initialize the ptt object and start the task
ptt_t *ptt_create_dev_input(char const *device, int keycode);
ptt_t *ptt_create_gpio(char const *device, int pin_number);
// ptt_t *ptt_create_stdin(int keycode);

// Add button state change callbacks:
void ptt_add_pressed_cb(ptt_t *ptt, ptt_pressed_cb_t cb, void *user_data);
void ptt_add_released_cb(ptt_t *ptt, ptt_released_cb_t cb, void *user_data);

// terminate the ptt task
void ptt_destroy(ptt_t *ptt);

// OPTION 1: get the current state of the ptt button
bool ptt_is_pressed(ptt_t *ptt);

// OPTION 2: invoke this from your thread context's main loop to
// invoke the button state change callbacks
void ptt_loop(ptt_t *ptt);

#endif /* PTT_H_INCLUDED */
