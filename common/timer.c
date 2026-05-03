#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/* OCR1A derived from F_CPU at compile time, so changing the Makefile clock
 * (e.g. 8 MHz -> 16 MHz) automatically reconfigures the tick.
 *   tick_period = (OCR1A + 1) * prescaler / F_CPU
 * For prescaler /64 and a 1 ms tick:
 *   OCR1A = F_CPU / 64 / 1000 - 1
 *     F_CPU =  8 MHz -> OCR1A = 124
 *     F_CPU = 16 MHz -> OCR1A = 249
 */
#define TIMER1_OCRA_FOR_1MS  ((F_CPU) / 64UL / 1000UL - 1UL)

static volatile uint16_t g_ms = 0;

ISR(TIMER1_COMPA_vect) {
    g_ms++;
}

void timer_init_1ms(void) {
    TCCR1A = 0;                                  /* no pin output behavior */
    TCCR1B = (1 << WGM12)                        /* CTC, TOP = OCR1A       */
           | (1 << CS11) | (1 << CS10);          /* prescaler /64          */
    OCR1A  = (uint16_t) TIMER1_OCRA_FOR_1MS;
    TIMSK1 = (1 << OCIE1A);                      /* enable compare-A int   */
}

uint16_t timer_now_ms(void) {
    uint16_t v;
    /* 16-bit reads on this 8-bit MCU aren't atomic; ISR could fire between
     * the two byte loads. ATOMIC_BLOCK briefly disables interrupts. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        v = g_ms;
    }
    return v;
}
