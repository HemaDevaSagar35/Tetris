#include "rng.h"
#include <avr/io.h>

/* Must be nonzero -- xorshift gets stuck at 0 (x ^ (x<<n) == 0 → stays 0). */
static uint8_t rng_state = 1;

uint8_t rng_next8(void) {
    uint8_t x = rng_state;
    x ^= (uint8_t)(x << 7);
    x ^= (uint8_t)(x >> 5);
    x ^= (uint8_t)(x << 3);
    rng_state = x;
    return x;
}

void rng_seed_from_adc(void) {
    /* ADC0 (PA0), AVcc reference, prescaler /128.
     * /128 at F_CPU=8 MHz → 62.5 kHz ADC clock, within the 50-200 kHz
     * datasheet-recommended range. Accuracy doesn't matter here -- we
     * specifically want the noisy LSB. */
    ADMUX  = (uint8_t)(1 << REFS0);
    ADCSRA = (uint8_t)((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));

    /* Datasheet: the first conversion after ADC enable is slow and less
     * stable. Throw it away. */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) { }

    /* Shift-and-fold the LSB of 8 conversions into one seed byte. */
    uint8_t seed = 0;
    for (uint8_t i = 0; i < 8; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)) { }
        seed = (uint8_t)((seed << 1) | (uint8_t)(ADC & 1));
    }

    /* Power down the ADC -- not used during play. */
    ADCSRA &= (uint8_t)~(1 << ADEN);

    /* If ADC produced an all-zero seed (unlikely but possible -- e.g. PA0
     * pulled to GND somewhere), fall back to a constant. xorshift dies at
     * 0 so any nonzero value works. */
    if (seed == 0) seed = 1;
    rng_state = seed;
}
