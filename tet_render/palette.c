#include "palette.h"
#include "st7735.h"     /* BLACK */

/* RGB565 values were derived from RGB888 Tetris Guideline colours by
 * keeping the top 5 / 6 / 5 bits of each channel (R<<11 | G<<5 | B).
 * See avr-c-port-design.mdc for the derivation table. */
const uint16_t palette[COLOR_COUNT] = {
    [COLOR_BG] = BLACK,
    [COLOR_T]  = 0xFFE0,    /* yellow */
    [COLOR_I]  = 0x07FF,    /* cyan   */
    [COLOR_O]  = 0xFD20,    /* orange */
    [COLOR_L]  = 0x001F,    /* blue   */
    [COLOR_J]  = 0xFB56,    /* pink   */
    [COLOR_S]  = 0x07E0,    /* green  */
    [COLOR_Z]  = 0xF800,    /* red    */
};
