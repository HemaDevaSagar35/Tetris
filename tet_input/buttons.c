#include "buttons.h"

#include <avr/io.h>
#include "timer.h"

/* ---- pin map ------------------------------------------------------------ *
 * Active-low: button shorts the pin to GND, internal pull-up holds it HIGH
 * when not pressed.
 *
 * Avoid pins already in use:
 *   PB4..PB7 = SPI to ST7735, PD5/PD6/PD7 = LCD DC/RST/BL,
 *   PC2..PC5 = JTAG (only available as GPIO if JTAGEN fuse is disabled).
 *
 * Buttons live on two ports:
 *   PORTD: BTN_LEFT, BTN_ROT_CCW
 *   PORTC: BTN_RIGHT, BTN_ROT_CW
 *
 * PD0/PD1 (USART RXD/TXD) are intentionally NOT used so USART stays free
 * for printf-style debugging. PC0/PC1 are the I2C/TWI pins but we don't
 * use I2C anywhere, so they're safe as plain GPIO.
 * --------------------------------------------------------------------------*/
#define BTN_LEFT_BIT      PD4   /* move left           */
#define BTN_ROT_CCW_BIT   PD3   /* rotate anti-clock   */
#define BTN_RIGHT_BIT     PC0   /* move right          */
#define BTN_ROT_CW_BIT    PC1   /* rotate clockwise    */

#define BTN_MASK_PORTD  ((1 << BTN_LEFT_BIT) | (1 << BTN_ROT_CCW_BIT))
#define BTN_MASK_PORTC  ((1 << BTN_RIGHT_BIT) | (1 << BTN_ROT_CW_BIT))

/* ---- debounce ----------------------------------------------------------- *
 * Wait-for-stable: a raw edge isn't accepted until the new state has held
 * steady for DEBOUNCE_MS. Bigger than the typical 5-20 ms bounce window,
 * smaller than the human max click rate (~10 Hz = 100 ms apart).
 * --------------------------------------------------------------------------*/
#define DEBOUNCE_MS   50U

typedef struct {
    uint8_t  debounced;   /* last accepted (debounced) state    */
    uint8_t  last_raw;    /* last raw read from the pin         */
    uint16_t change_ms;   /* timer_now_ms() at last raw change  */
} button_state_t;

void buttons_init(void) {
    /* One masked write per port: configure as inputs, then enable pull-ups. */
    DDRD  &= ~BTN_MASK_PORTD;
    PORTD |=  BTN_MASK_PORTD;

    DDRC  &= ~BTN_MASK_PORTC;
    PORTC |=  BTN_MASK_PORTC;
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
    uint8_t raw = !(PIND & (1 << BTN_LEFT_BIT));
    return button_step(&st, raw);
}

uint8_t button_right_just_pressed(void) {
    static button_state_t st = {0};
    uint8_t raw = !(PINC & (1 << BTN_RIGHT_BIT));
    return button_step(&st, raw);
}

uint8_t button_rotate_cw_just_pressed(void) {
    static button_state_t st = {0};
    uint8_t raw = !(PINC & (1 << BTN_ROT_CW_BIT));
    return button_step(&st, raw);
}

uint8_t button_rotate_ccw_just_pressed(void) {
    static button_state_t st = {0};
    uint8_t raw = !(PIND & (1 << BTN_ROT_CCW_BIT));
    return button_step(&st, raw);
}
