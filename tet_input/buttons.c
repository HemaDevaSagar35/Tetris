#include "buttons.h"

#include <avr/io.h>
#include "timer.h"

/* ---- pin map ------------------------------------------------------------ *
 * Add new buttons here. Each button gets a bit on PORTD (or another port).
 * Avoid pins already in use: PB4..PB7 (SPI), PD0/PD1 (USART),
 * PD5/PD6/PD7 (LCD DC/RST/BL).
 * --------------------------------------------------------------------------*/
#define BTN_LEFT_BIT  PD4

/* ---- debounce ----------------------------------------------------------- *
 * Wait-for-stable: a raw edge isn't accepted until the new state has held
 * steady for DEBOUNCE_MS. Bigger than the typical 5-20ms bounce window,
 * smaller than the human max click rate (~10 Hz = 100 ms apart).
 * --------------------------------------------------------------------------*/
#define DEBOUNCE_MS   50U

typedef struct {
    uint8_t  debounced;   /* last accepted (debounced) state */
    uint8_t  last_raw;    /* last raw read from the pin       */
    uint16_t change_ms;   /* timer_now_ms() at last raw change */
} button_state_t;

void buttons_init(void) {
    DDRD  &= ~(1 << BTN_LEFT_BIT);   /* PD4 as input            */
    PORTD |=  (1 << BTN_LEFT_BIT);   /* enable internal pull-up */
}

/* Generic per-button stepper.
 * Returns 1 the first poll after `raw` has been stably HIGH for DEBOUNCE_MS
 * following a previously-stable LOW (i.e. a real press). 0 otherwise. */
static uint8_t button_step(button_state_t *st, uint8_t raw) {
    uint16_t now = timer_now_ms();

    if (raw != st->last_raw) {
        st->change_ms = now;
        st->last_raw  = raw;
    }

    if ((uint16_t)(now - st->change_ms) >= DEBOUNCE_MS &&
        st->debounced != st->last_raw)
    {
        uint8_t prev = st->debounced;
        st->debounced = st->last_raw;
        return st->debounced && !prev;   /* rising edge of debounced signal */
    }
    return 0;
}

uint8_t button_left_just_pressed(void) {
    static button_state_t st = {0};
    /* Pressed = pin LOW (active-low with internal pull-up). */
    uint8_t raw = !(PIND & (1 << BTN_LEFT_BIT));
    return button_step(&st, raw);
}
