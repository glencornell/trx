#include "ptt.h"
#include <errno.h>
#include <fcntl.h>
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

typedef void (*create_proc_t)(ptt_t *);
typedef void (*destroy_proc_t)(ptt_t *);
typedef bool (*is_pressed_proc_t)(ptt_t *);

typedef struct ptt_s {
  ptt_input_source_t input_source;
  bool is_pressed; // ptt button state

  int keycode;
  bool is_active; // variable to tell thread when to exit from main loop
  pthread_t pid; // ptt gpio thread id
  pthread_mutex_t mutex; // mutex for shared state

  create_proc_t create_proc;
  destroy_proc_t destroy_proc;
  is_pressed_proc_t is_pressed_proc;
} ptt_t;


static void *dev_input_main(void *arg) {
  ptt_t *ptt = (ptt_t *)arg;
  const char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
  struct input_event ev;
  ssize_t n;
  int fd;
  
  if ((fd = open(dev, O_RDONLY)) == -1) {
    fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
    return NULL;
  }

  while (ptt->is_active) {
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
      case KEY_RELEASED:
        printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
        pthread_mutex_lock(&ptt->mutex);
        ptt->is_pressed = ev.value == KEY_PRESSED;
        pthread_mutex_unlock(&ptt->mutex);
        break;
      case KEY_REPEATED:
        break;
      }
    }
  }

  return NULL;
}

static void dev_input_create(ptt_t *ptt) {
  pthread_mutex_init(&ptt->mutex, NULL);
  pthread_create(&ptt->pid, NULL, &dev_input_main, (void *)ptt);
}

static void dev_input_destroy(ptt_t *ptt) {
  ptt->is_active = false; // tell the thread to exit its main loop
  pthread_join(ptt->pid, NULL);
  pthread_mutex_destroy(&ptt->mutex);
}

static bool dev_input_is_pressed(ptt_t *ptt) {
  bool rval = false;
  pthread_mutex_lock(&ptt->mutex);
  rval = ptt->is_pressed;
  pthread_mutex_unlock(&ptt->mutex);
  return rval;
}

static void gpio_create(ptt_t *ptt) {
  // TODO
}

static void gpio_destroy(ptt_t *ptt) {
  // TODO
}

static bool gpio_is_pressed(ptt_t *ptt) {
  bool rval = false;
  // TODO: read gpio pin
  return rval;
}

ptt_t *ptt_create_simple() {
  return ptt_create(PTT_INPUT_SOURCE_DEV_INPUT, KEY_LEFTCTRL);
}

ptt_t *ptt_create(ptt_input_source_t input_source, int keycode) {
  ptt_t *ptt = (ptt_t *)malloc(sizeof(ptt_t));
  ptt->input_source = input_source;
  ptt->keycode = keycode;
  ptt->is_pressed = false;
  ptt->is_active = true;
  
  switch (ptt->input_source) {
  case PTT_INPUT_SOURCE_DEV_INPUT:
    ptt->create_proc = &dev_input_create;
    ptt->destroy_proc = &dev_input_destroy;
    ptt->is_pressed_proc = &dev_input_is_pressed;
    break;
  case PTT_INPUT_SOURCE_GPIO:
    ptt->create_proc = &gpio_create;
    ptt->destroy_proc = &gpio_destroy;
    ptt->is_pressed_proc = &gpio_is_pressed;
    break;
  }
  ptt->create_proc(ptt);
  return ptt;
}

void ptt_destroy(ptt_t *ptt) {
  ptt->destroy_proc(ptt);
  free(ptt);
}

bool ptt_is_pressed(ptt_t *ptt) {
  return ptt->is_pressed_proc(ptt);
}
