#ifndef TET_INPUT_BUTTONS_H
#define TET_INPUT_BUTTONS_H

#include <stdint.h>

/* Configure all button pins as inputs with internal pull-ups. Call once at boot. */
void buttons_init(void);

/* Returns 1 exactly once when the LEFT button has a debounced rising edge
 * (a real press, after mechanical bounce settles); returns 0 otherwise.
 *
 * Call at most once per main-loop iteration. Must be called continuously --
 * the function only detects edges by comparing successive calls.
 */
uint8_t button_left_just_pressed(void);

#endif
