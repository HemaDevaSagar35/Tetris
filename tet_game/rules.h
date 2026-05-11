#ifndef TET_GAME_RULES_H
#define TET_GAME_RULES_H

#include <stdint.h>
#include "utils.h"   /* tet_shapes/utils.h: Shape, Boundary */
#include "board.h"

/* Returns the x-axis shift needed to bring `limits` back inside
 * [0, BOARD_W-1]:
 *   - 0  if already in bounds,
 *   - >0 if the shape pokes off the LEFT  edge (shift right by this much),
 *   - <0 if the shape pokes off the RIGHT edge (shift left  by this much).
 * Used as a wall-kick by do_valid_rotation and (step 3) as a spawn-time
 * correction by the factory after a random x.
 * Direct port of C++ `xaxis_correction` (game/utils.h:9-19). */
int8_t xaxis_correction(Boundary limits);

/* Try moving the active piece by `dx` columns. The move is committed if the
 * new position is valid (no wall, floor, or latched-cell overlap);
 * otherwise it is reverted. Returns 1 on commit, 0 on revert.
 *
 * Replaces the wall-only `b.x_min > 0` / `b.x_max < BOARD_W - 1` checks in
 * Step 1's main.c -- this version is stack-aware via board_collides.
 * Direct port of C++ `do_valid_move` (game/utils.h:83-92). */
uint8_t do_valid_move(Shape *s, const Board *b, int8_t dx);

/* Try rotating the active piece by `rotate` (+1 = CW, -1 = CCW). Applies a
 * horizontal wall-kick correction first, then validates against the board.
 * Reverts BOTH rotation and correction on failure. Returns 1 on commit,
 * 0 on revert.
 *
 * Replaces Step 1's stub `try_rotation` in main.c, which only checked the
 * bottom edge. This version checks every wall + every latched cell.
 * Direct port of C++ `do_valid_rotation` (game/utils.h:66-80). */
uint8_t do_valid_rotation(Shape *s, const Board *b, int8_t rotate);

#endif
