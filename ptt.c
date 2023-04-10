#include "ptt.h"
#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KEY_RELEASED 0
#define KEY_PRESSED  1
#define KEY_REPEATED 2

static const char *const evval[3] = {
    "RELEASED",
    "PRESSED ",
    "REPEATED"
};

typedef enum {
  PTT_KEY_STATE_RELEASED,
  PTT_KEY_STATE_PRESSED,
  PTT_KEY_STATE_UNKNOWN
} ptt_key_state_t;

typedef void (*destroy_proc_t)(ptt_t *);
typedef bool (*is_pressed_proc_t)(ptt_t *);

extern unsigned int verbose;

static void *dev_input_thread_main(void *arg);
static bool dev_input_is_pressed(ptt_t *ptt);
static void dev_input_destroy(ptt_t *ptt);
static void gpio_destroy(ptt_t *ptt);
static bool gpio_is_pressed(ptt_t *ptt);

typedef struct ptt_s {
  // Common state variables:
  ptt_input_source_t input_source;
  ptt_key_state_t prev_key_state;
  ptt_key_state_t key_state;  // protected by a mutex
  char const *device;
  destroy_proc_t destroy_proc;
  is_pressed_proc_t is_pressed_proc;
  ptt_pressed_cb_t pressed_cb;
  void *pressed_user_data;
  ptt_released_cb_t released_cb;
  void *released_user_data;

  // state variables when input source is /dev/input:
  int keycode;
  bool is_active; // variable to tell thread when to exit from main loop
  pthread_t pid; // ptt gpio thread id
  pthread_mutex_t mutex; // mutex for shared state

  // state variables when the input source is gpio.  Note that we use
  // the linux gpiod interface:
  struct gpiod_chip *chip;
  struct gpiod_line *line;
} ptt_t;


//////////////////////////////////////////////////////////////////////
// INPUT SOURCE: /DEV/INPUT

ptt_t *ptt_dev_input_create(char *input_device, int keycode) {
  ptt_t *ptt = (ptt_t *)malloc(sizeof(ptt_t));
  ptt->input_source = PTT_INPUT_SOURCE_DEV_INPUT;
  ptt->device = input_device;
  ptt->keycode = keycode;
  ptt->prev_key_state = PTT_KEY_STATE_UNKNOWN;
  ptt->key_state = PTT_KEY_STATE_UNKNOWN;
  ptt->is_active = true;
  ptt->destroy_proc = &dev_input_destroy;
  ptt->is_pressed_proc = &dev_input_is_pressed;
  ptt->pressed_cb = NULL;
  ptt->pressed_user_data = NULL;
  ptt->released_cb = NULL;
  ptt->released_user_data = NULL;
  pthread_mutex_init(&ptt->mutex, NULL);
  pthread_create(&ptt->pid, NULL, &dev_input_thread_main, (void *)ptt);
  return ptt;
}

static void *dev_input_thread_main(void *arg) {
  ptt_t *ptt = (ptt_t *)arg;
  struct input_event ev;
  ssize_t n;
  int fd;

  if ((fd = open(ptt->device, O_RDONLY)) == -1) {
    fprintf(stderr, "Cannot open %s: %s.\n", ptt->device, strerror(errno));
    return NULL;
  }

  while (ptt->is_active) {
    // blocking read to prevent over-utilizaton of CPU
    if ((n = read(fd, &ev, sizeof ev)) == (ssize_t)-1) {
      if (errno == EINTR)
        continue;
      else
        break;
    } else {
      if (n != sizeof ev) {
        errno = EIO;
        break;
      }
    }
    if (ev.type == EV_KEY && ev.code == ptt->keycode) {
      switch(ev.value) {
      case KEY_PRESSED:
        printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
        pthread_mutex_lock(&ptt->mutex);
        ptt->key_state = PTT_KEY_STATE_PRESSED;
        pthread_mutex_unlock(&ptt->mutex);
        break;
      case KEY_RELEASED:
        printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
        pthread_mutex_lock(&ptt->mutex);
        ptt->key_state = PTT_KEY_STATE_RELEASED;
        pthread_mutex_unlock(&ptt->mutex);
        break;
      case KEY_REPEATED:
        break;
      }
    }
  }

  close(fd);

  return NULL;
}

static void dev_input_destroy(ptt_t *ptt) {
  ptt->is_active = false; // tell the thread to exit its main loop
  pthread_join(ptt->pid, NULL);
  pthread_mutex_destroy(&ptt->mutex);
}

static bool dev_input_is_pressed(ptt_t *ptt) {
  bool rval = false;
  pthread_mutex_lock(&ptt->mutex);
  rval = ptt->key_state == PTT_KEY_STATE_PRESSED;
  pthread_mutex_unlock(&ptt->mutex);
  return rval;
}

//////////////////////////////////////////////////////////////////////
// INPUT SOURCE: GPIO

ptt_t *ptt_create_gpio(char const *device_name, int pin_number) {
  ptt_t *ptt = (ptt_t *)malloc(sizeof(ptt_t));
  ptt->input_source = PTT_INPUT_SOURCE_GPIO;
  ptt->prev_key_state = PTT_KEY_STATE_UNKNOWN;
  ptt->key_state = PTT_KEY_STATE_UNKNOWN;
  ptt->is_active = true;
  ptt->destroy_proc = &gpio_destroy;
  ptt->is_pressed_proc = &gpio_is_pressed;
  ptt->pressed_cb = NULL;
  ptt->pressed_user_data = NULL;
  ptt->released_cb = NULL;
  ptt->released_user_data = NULL;
  ptt->chip = gpiod_chip_open(device_name);
  ptt->line = gpiod_chip_get_line(ptt->chip, pin_number);
  gpiod_line_request_input(ptt->line, "PTT");
  return ptt;
}

static void gpio_destroy(ptt_t *ptt) {
  gpiod_line_release(ptt->line);
  gpiod_chip_close(ptt->chip);
}

static bool gpio_is_pressed(ptt_t *ptt) {
  int val = gpiod_line_get_value(ptt->line);
  return (val == 0 || val == -1) ? false : true;
}

//////////////////////////////////////////////////////////////////////
// PTT API

ptt_t *ptt_create_simple() {
  return ptt_dev_input_create(DEFAULT_PTT_DEV_INPUT_DEVICE, KEY_LEFTCTRL);
}

void ptt_destroy(ptt_t *ptt) {
  ptt->destroy_proc(ptt);
  free(ptt);
}

bool ptt_is_pressed(ptt_t *ptt) {
  return ptt->is_pressed_proc(ptt);
}

void ptt_add_pressed_cb(ptt_t *ptt, ptt_pressed_cb_t cb, void *user_data) {
  ptt->pressed_cb = cb;
  ptt->pressed_user_data = user_data;
}

void ptt_add_released_cb(ptt_t *ptt, ptt_released_cb_t cb, void *user_data) {
  ptt->released_cb = cb;
  ptt->released_user_data = user_data;
}

void ptt_loop_iter(ptt_t *ptt) {
  const bool is_pressed = ptt_is_pressed(ptt);
  const bool was_pressed = ptt->prev_key_state == PTT_KEY_STATE_PRESSED;

  if (is_pressed && !was_pressed) {
    ptt->prev_key_state = PTT_KEY_STATE_PRESSED;
    if (ptt->pressed_cb) ptt->pressed_cb(ptt, ptt->pressed_user_data);
  } else if (!is_pressed && was_pressed) {
    ptt->prev_key_state = PTT_KEY_STATE_RELEASED;
    if (ptt->released_cb) ptt->released_cb(ptt, ptt->released_user_data);
  }
}
