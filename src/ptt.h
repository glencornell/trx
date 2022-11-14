#ifndef PTT_H_INCLUDED
#define PTT_H_INCLUDED

#include <stdbool.h>

typedef enum {
  PTT_INPUT_SOURCE_DEV_INPUT,
  PTT_INPUT_SOURCE_GPIO
} ptt_input_source_t;

struct ptt_s;
typedef struct ptt_s ptt_t;

ptt_t *ptt_create_simple();  // simple init: read from keyboard, ptt keycode is left control key
ptt_t *ptt_create(ptt_input_source_t input_source, int keycode);  // initialize the ptt object and start the task
void ptt_destroy(ptt_t *ptt); // destroy the ptt task
bool ptt_is_pressed(ptt_t *ptt); // get the current state of the ptt button

#endif /* PTT_H_INCLUDED */
