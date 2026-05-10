#ifndef TET_RENDER_HUD_H
#define TET_RENDER_HUD_H

#include <stdint.h>
#include "st7735.h"

/* ---- HUD layout --------------------------------------------------------- *
 * The playfield is centered in a 130 px screen with ~25 px on each side.
 *
 *   LEFT  margin (x = 0 .. 24): score readout.
 *   RIGHT margin (x = 105 .. 129): next-piece preview (added in Phase 2).
 *
 * Score:
 *   Three digits, leading zeros, capped at 999 (saturating).
 *   Each digit is a 3-col x 5-row bitmap (the same bitmap as
 *   testing_main.cpp ScoreBoard::DIGITS), pixel-doubled to 6 x 10 LCD
 *   pixels for legibility. Inter-digit gap = 1 px.
 *   Layout: origin (SCORE_X, SCORE_Y) = (2, 4). Total footprint:
 *     width  = 3 * 6 + 2 * 1 = 20 px (fits within 25 px left margin)
 *     height = 5 * 2         = 10 px
 *
 * Update rule: ports testing_main.cpp:418-420 verbatim --
 *   score = score + board.total_lines
 * fired when the line-clear animation finishes. Score is saturated at
 * 999 to keep the display three digits.
 *
 * Render cost: each call redraws all 3 digits as 15 fat-pixel rectangles
 * (3*5 cells per digit). At ~150 us per rectangle on a 4 MHz SPI, ~7 ms
 * total. Only fires when a line clears, so it's not in the hot path. */
void render_hud_score(struct st7735 *lcd, uint16_t score);

#endif
