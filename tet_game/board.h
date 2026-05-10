#ifndef TET_GAME_BOARD_H
#define TET_GAME_BOARD_H

#include <stdint.h>
#include "utils.h"   /* tet_shapes/utils.h: Shape, Pixel, SHAPE_BLOCKS */

/* Classic Tetris playfield. All coords are in board-cells, never pixels.
 * Renderer is the only thing that knows about pixels. */
#define BOARD_W   10
#define BOARD_H   20

/* Color-index grid. Mirrors the C++ `Board` (shapes/utils.h:136-326) but
 * stores `uint8_t` palette indices instead of full `Color` structs --
 * 200 B vs 800 B for an RGB565 grid. line_formed/deltas/total_lines
 * are populated by the line-clearing API in step 4 and are zero/unused
 * until then. */
typedef struct {
    uint8_t cells[BOARD_H][BOARD_W];   /* 0 = empty, otherwise palette index */
    uint8_t line_formed[BOARD_H];      /* 1 if row is full (set by check_lines) */
    uint8_t deltas[BOARD_H];           /* drop distance per row during collapse  */
    uint8_t lines_filled;              /* 1 if any line is currently full        */
    uint8_t total_lines;               /* count of full rows after check_lines   */
    int8_t  height_peak;               /* min y of any latched cell;
                                          BOARD_H == "nothing latched yet"        */
} Board;

/* Zero out everything. Call once at boot, plus on "play again". */
void board_init(Board *b);

/* Read a single cell. No bounds check on the hot path -- callers stay in
 * range. Returns the palette index (0 = empty). */
uint8_t board_get_cell(const Board *b, int8_t row, int8_t col);

/* Returns 1 if shifting `s` by (dx, dy) would put any of its 4 blocks:
 *   - off the left/right edge,
 *   - below the floor,
 *   - on top of an already-latched cell.
 * Above the visible board (y < 0) is NOT a collision -- pieces spawn from
 * y < 0 in some rotations and that has to be allowed.
 *
 * Usage:
 *   board_collides(b, s,  0,  0)  -- "is the piece overlapping right now?"
 *                                    (post-move/rotation validity check)
 *   board_collides(b, s,  0,  1)  -- "would gravity land it on something?"
 *                                    (lock test in the gravity tick)
 *   board_collides(b, s, -1,  0)  -- "could it move one cell left?"
 *
 * Replaces the C++ `onboard(Shape*, Board&, int overlap)` (game/utils.h:21-44),
 * generalised: C++ only checks (0, overlap); we take both dx and dy because
 * left/right collision needs (-1, 0) / (+1, 0). */
uint8_t board_collides(const Board *b, const Shape *s, int8_t dx, int8_t dy);

/* Stamp the shape's 4 blocks into the board with `color_idx`. Updates
 * height_peak. Does NOT touch the LCD -- the piece's pixels are already on
 * screen at this point, the board just takes ownership of them.
 *
 * Mirrors C++ `Board::latch_on` (shapes/utils.h:169-178) but defers
 * `line_formation()` to the caller -- the spawn / line-clear flow lives
 * in main.c, not inside latch. */
void board_latch(Board *b, const Shape *s, uint8_t color_idx);

/* Walk every row; mark line_formed[y] = 1 if row y is full, 0 otherwise.
 * Also writes lines_filled (1 if any) and total_lines (count).
 *
 * Call once after board_latch when you want to know if a line clear is due.
 * Cheap: 200 cell reads max. Faithful port of C++ Board::line_formation()
 * (shapes/utils.h:196-213). */
void board_check_lines(Board *b);

/* Instantly remove every row where line_formed[y] == 1 and pack the
 * non-full rows down so they stack against the floor. Updates height_peak
 * (new = old + total_lines, clamped). Does NOT touch line_formed[] /
 * lines_filled / total_lines -- caller reads them for scoring, then calls
 * board_reset_lines.
 *
 * Returns the number of cleared lines for convenience.
 *
 * Diverges from C++ Board::clear_lines (shapes/utils.h:216-236): that
 * version copies source rows to destinations but never clears the sources,
 * which double-stamps any piece that falls. The C++ avoids the bug by
 * using the animated clear_lines_selectively path in production; we'd
 * rather have a correct instant version. Two-pointer compaction: bottom-up
 * read pointer, bottom-up write pointer, skip full rows on read, copy
 * read->write on non-full, wipe everything above the final write index. */
uint8_t board_clear_lines(Board *b);

/* Zero line_formed[], deltas[], lines_filled, total_lines. Call after
 * consuming total_lines for scoring. Mirrors C++ Board::reset_lines_deltas
 * (shapes/utils.h:286-290). */
void board_reset_lines(Board *b);

/* --- step 5: animated clear support -------------------------------------- *
 * The width sweep wipes each full row column-by-column over time; the
 * height drop then shifts the surviving rows down. The three functions
 * below are the per-tick board mutations the animation state machine in
 * main.c calls. None of them touch the LCD -- the caller is responsible
 * for repainting the small set of cells that changed. */

/* Compute deltas[y] = number of full rows in rows [y+1, BOARD_H-1].
 * deltas[BOARD_H-1] is forced to 0 (nothing below the floor). Tells each
 * row how far down it will fall during the height animation. Call once
 * when the width sweep finishes. Faithful port of C++ calculate_delta
 * (shapes/utils.h:293-303). */
void board_calculate_deltas(Board *b);

/* One width-sweep tick: clear cells (left, y) and (right, y) for every
 * full row. Cheap: scans BOARD_H rows, writes 2 cells per marked row.
 * Faithful port of C++ clean_lines_selectively (shapes/utils.h:257-266). */
void board_clean_lines_selectively(Board *b, uint8_t left, uint8_t right);

/* One height-drop tick: shift row line_no down by deltas[line_no] cells.
 * If line_no is itself a full row (already wiped by the width sweep), or
 * if deltas[line_no] is 0 (nothing below to fall into), this is a no-op.
 * Otherwise: copy cells[line_no][*] -> cells[line_no + del_y][*], then
 * CLEAR cells[line_no][*] (so the row appears at its destination only,
 * not at both source and destination).
 *
 * The source-clear is our intentional fix over C++ clear_lines_selectively
 * (shapes/utils.h:268-284) which copies but never erases. The C++ version
 * leaves a "ghost" of the falling row at its old position -- the bug is
 * subtle enough to not be obvious in casual play but is a real visual
 * artefact when a piece sits a couple of rows above the cleared zone. */
void board_clear_lines_selectively(Board *b, int8_t line_no);

#endif
