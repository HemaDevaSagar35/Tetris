#include "board.h"

void board_init(Board *b) {
    for (uint8_t r = 0; r < BOARD_H; r++) {
        for (uint8_t c = 0; c < BOARD_W; c++) {
            b->cells[r][c] = 0;
        }
        b->line_formed[r] = 0;
        b->deltas[r]      = 0;
    }
    b->lines_filled = 0;
    b->total_lines  = 0;
    b->height_peak  = (int8_t)BOARD_H;   /* sentinel: nothing latched yet */
}

uint8_t board_get_cell(const Board *b, int8_t row, int8_t col) {
    return b->cells[row][col];
}

uint8_t board_collides(const Board *b, const Shape *s, int8_t dx, int8_t dy) {
    const Pixel *blocks = shape_get_blocks(s);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        int8_t nx = (int8_t)(blocks[i].x + dx);
        int8_t ny = (int8_t)(blocks[i].y + dy);

        /* Walls and floor. Top is intentionally unbounded so spawn rotations
         * with negative y are legal. */
        if (nx < 0 || nx >= (int8_t)BOARD_W) return 1;
        if (ny >= (int8_t)BOARD_H)           return 1;
        if (ny < 0)                          continue;

        if (b->cells[ny][nx] != 0) return 1;
    }
    return 0;
}

void board_latch(Board *b, const Shape *s, uint8_t color_idx) {
    const Pixel *blocks = shape_get_blocks(s);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        int8_t x = blocks[i].x;
        int8_t y = blocks[i].y;

        /* Defensive: a block above the visible board (y < 0) would be the
         * "spawned and locked instantly" game-over case. Skip writing
         * out-of-range cells; game-over detection lives in step 7. */
        if (y < 0 || y >= (int8_t)BOARD_H) continue;
        if (x < 0 || x >= (int8_t)BOARD_W) continue;

        b->cells[y][x] = color_idx;
        if (y < b->height_peak) b->height_peak = y;
    }
}
