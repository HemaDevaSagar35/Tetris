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

/* Next-piece preview (Step 6 phase 2):
 *   - 4 x 4 cell grid (NEXT_BOX_CELLS = 4); fits the widest piece (I = 4
 *     cells wide) and the tallest in canonical pose (L/J = 3 cells tall).
 *   - 4 px per mini-block (NEXT_PX = 4); total box = 16 x 16 px.
 *   - Centered in the right margin: usable area is x = 106..129 (24 px,
 *     since x = 105 is the playfield frame); box origin (110, 10) leaves
 *     4 px of breathing room on each side.
 *
 * Piece is drawn in canonical pose (rotation 0, anchor at origin), with
 * its bounding box centered in the 4x4 grid. We use `boundary.x_min` to
 * absorb the negative x-offsets in J and S create_shape (J has a block
 * at x = -1, S has one at x = -2 -- both spawn with x_min < 0).
 *
 * Update rule: fired once at boot (initial queued piece) and on every
 * promotion (when the queued piece becomes active). Erases the box to
 * BG then paints the 4 mini-blocks, ~5 ms SPI total. NOT in the C++ ref
 * (`testing_main.cpp` only renders the score); added here for parity
 * with classic Tetris (NES / Game Boy) which all show a next preview. */
#define NEXT_PX           4
#define NEXT_BOX_CELLS    4
#define NEXT_BOX_W        (NEXT_BOX_CELLS * NEXT_PX)
#define NEXT_BOX_H        (NEXT_BOX_CELLS * NEXT_PX)
#define NEXT_BOX_X        110
#define NEXT_BOX_Y        10

void render_hud_next(struct st7735 *lcd, uint8_t kind);

#endif
