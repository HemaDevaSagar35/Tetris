#ifndef COMMON_TIMER_H
#define COMMON_TIMER_H

#include <stdint.h>

/* Configure Timer1 in CTC mode to fire a compare-match interrupt every 1 ms.
 * Must be called once at boot. Interrupts are NOT actually delivered until
 * you also enable them globally via sei() — call this BEFORE sei().
 */
void timer_init_1ms(void);

/* Read the millisecond counter atomically. Wraps every ~65.5 s, so always
 * use unsigned 16-bit subtraction for deltas:
 *
 *   uint16_t now = timer_now_ms();
 *   if ((uint16_t)(now - prev) >= GRAVITY_MS) { ... }
 */
uint16_t timer_now_ms(void);

#endif
