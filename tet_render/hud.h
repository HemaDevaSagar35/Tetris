#ifndef TET_RENDER_HUD_H
#define TET_RENDER_HUD_H

#include <stdint.h>
#include "st7735.h"

/* ---- HUD layout --------------------------------------------------------- *
 * The playfield is centered in a 130 px screen with ~25 px on each side.
 *
 *   LEFT  margin (x = 0 .. 24): score readout (current + max).
 *   RIGHT margin (x = 105 .. 129): next-piece preview (added in Phase 2).
 *
 * Left HUD stack (top -> bottom):
 *
 *      y=  4..11   "SCR"    X1 label (5x8 font, 17 px wide)
 *      y= 14..23    digits  current score, 3 digits, pixel-doubled to 6x10
 *      y= 27..34   "MAX"    X1 label
 *      y= 37..46    digits  max score, same style as current
 *
 * Score digits use the same 3-col x 5-row bitmap as `testing_main.cpp`
 * `ScoreBoard::DIGITS`, pixel-doubled to 6 x 10 LCD pixels for legibility.
 * Inter-digit gap = 1 px. Both score blocks share the same x-origin
 * (HUD_LEFT_X) so they line up visually.
 *
 * Update rules:
 *   - Current score: ports testing_main.cpp:418-420 verbatim --
 *       score = score + board.total_lines
 *     fired when the line-clear animation finishes. Saturates at 999.
 *   - Max score: persisted to EEPROM. Read once at boot, only re-written
 *     at game-over when the current run beat the previous max. Player
 *     sees the new max paint to the HUD WHILE the GAME OVER overlay is
 *     up (the overlay is on the playfield; the left margin stays clear).
 *
 * Render cost per score: 15 fat-pixel rectangles (3*5 cells per digit) *
 * 3 digits at ~150 us per rectangle on a 4 MHz SPI = ~7 ms total. Labels
 * are drawn once per game (in reset_game), not per score change.        */

#define HUD_LEFT_X        2
#define HUD_SCR_LABEL_Y   4
#define HUD_SCORE_DIG_Y   14
#define HUD_MAX_LABEL_Y   27
#define HUD_MAX_DIG_Y     37

void render_hud_labels(struct st7735 *lcd);
void render_hud_score(struct st7735 *lcd, uint16_t score);
void render_hud_max_score(struct st7735 *lcd, uint16_t max_score);

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

/* Game-over overlay (Step 7):
 *   - Centered panel on the playfield, 76 x 45 px (x = 27..102, y = 58..102).
 *   - WHITE filled panel + BLACK X1 text (5x8 font). Three lines:
 *       line 1  "GAME OVER"      (9 chars,  53 px wide)
 *       line 2  "PLAY AGAIN?"    (11 chars, 65 px wide)
 *       line 3  "[YES]  NO" / "YES  [NO]"  (9 chars, 53 px wide -- cursor)
 *
 *   - Split into two render functions so cursor moves don't redraw the
 *     whole panel:
 *
 *       render_game_over_overlay(lcd)
 *           Paint panel + static lines 1 and 2. Called once on game-over
 *           entry.
 *
 *       render_game_over_selection(lcd, selection)
 *           Erase the line-3 strip back to WHITE, then paint the YES/NO
 *           line with brackets around the active selection. Called on
 *           entry and after every LEFT/RIGHT press. We have to erase the
 *           strip because the two strings have brackets at different
 *           x-offsets, so simply overdrawing the new string would leave
 *           old bracket pixels.
 *
 *   - Inputs while game_over == 1:
 *       LEFT  -> selection = 0 (YES)
 *       RIGHT -> selection = 1 (NO)
 *       DOWN  -> commit: YES = full reset, NO = no-op (wired later for
 *                power-off behaviour).
 *     The gameplay input block is short-circuited above by a `continue`,
 *     so the down-button just_pressed() callers are mutually exclusive --
 *     no edge can be consumed twice. */
#define GAME_OVER_SEL_YES   0
#define GAME_OVER_SEL_NO    1

void render_game_over_overlay(struct st7735 *lcd);
void render_game_over_selection(struct st7735 *lcd, uint8_t selection);

/* Start-screen overlay (boot):
 *   Same 76 x 45 px panel as game-over (visual consistency). Two lines:
 *       "TETRIS"   in X2 (5x16 font, splash feel), 35 px wide
 *       "[PLAY]"   in X1 (5x8 font), 35 px wide -- brackets read as button
 *
 *   Drawn once at boot before any game state is initialised. The main
 *   loop's start-screen short-circuit (game_started == 0) waits for a
 *   DOWN press and then calls reset_game() to flip into gameplay. */
void render_start_overlay(struct st7735 *lcd);

#endif
