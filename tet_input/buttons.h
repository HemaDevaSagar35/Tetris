#ifndef TET_INPUT_BUTTONS_H
#define TET_INPUT_BUTTONS_H

#include <stdint.h>

/* Configure all button pins as inputs with internal pull-ups. Call once at boot. */
void buttons_init(void);

/* Each getter returns 1 exactly once when its button has a debounced rising
 * edge (a real press, after mechanical bounce settles); 0 otherwise.
 *
 * Call each one at most once per main-loop iteration. They must be called
 * continuously -- edges are detected by comparing successive calls.
 */
uint8_t button_left_just_pressed(void);
uint8_t button_right_just_pressed(void);
uint8_t button_rotate_cw_just_pressed(void);
uint8_t button_rotate_ccw_just_pressed(void);
uint8_t button_down_just_pressed(void);

#endif
