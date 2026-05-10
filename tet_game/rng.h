#ifndef TET_GAME_RNG_H
#define TET_GAME_RNG_H

#include <stdint.h>

/* Seed the global xorshift8 state from ADC noise on ADC0 (PA0).
 *
 * Reads 8 ADC conversions and folds each LSB into the seed byte. PA0 should
 * be left floating (not wired to anything) -- the AVR ADC's least
 * significant bit picks up enough thermal/power noise on an unconnected
 * pin to produce different seeds each power-on.
 *
 * Call once at boot, before the first rng_next8() call. Safe to call with
 * interrupts enabled -- ADC reads are polled, no ISR involved.
 *
 * Replaces C++ `random_device` + `mt19937` setup (testing_main.cpp:143-160). */
void rng_seed_from_adc(void);

/* Advance the xorshift8 state and return the new value. Period 255: cycles
 * through every non-zero 8-bit value before repeating. ~85 spawns of 3
 * calls each before the cycle repeats; fine for piece selection.
 *
 * The (7, 5, 3) shift triple is the canonical known-good 8-bit xorshift
 * (Wikipedia "Xorshift"). If we ever need a longer period, swap in
 * xorshift16 for ~5 more bytes of state. */
uint8_t rng_next8(void);

#endif
