#ifndef TET_RENDER_PALETTE_H
#define TET_RENDER_PALETTE_H

#include <stdint.h>
#include "factory.h"   /* COLOR_BG, COLOR_T, ..., COLOR_COUNT */

/* Render-side mapping from game-state color indices (stored in Board.cells
 * and active_color_idx) to the ST7735's RGB565 colour values. Designator
 * initialisation in the .c keeps the mapping order independent of the
 * COLOR_* enum order, so re-ordering factory.h won't silently re-skin
 * pieces. */
extern const uint16_t palette[COLOR_COUNT];

#endif
