#include "rules.h"

int8_t xaxis_correction(Boundary limits) {
    if (limits.x_min < 0) {
        return (int8_t)(-limits.x_min);
    }
    if (limits.x_max >= (int8_t)BOARD_W) {
        return (int8_t)((BOARD_W - 1) - limits.x_max);
    }
    return 0;
}

uint8_t do_valid_move(Shape *s, const Board *b, int8_t dx) {
    shape_update_position(s, dx, 0);

    if (board_collides(b, s, 0, 0)) {
        shape_update_position(s, (int8_t)(-dx), 0);
        return 0;
    }
    return 1;
}

uint8_t do_valid_rotation(Shape *s, const Board *b, int8_t rotate) {
    shape_update_shape(s, rotate);

    /* Wall-kick: shift the rotated shape back inside the playfield if the
     * rotation pushed any block off the left/right edge. The vertical kick
     * (i.e. SRS-style "wall kicks" off the floor) is intentionally not
     * implemented -- a rotation that would put y_max past the floor just
     * fails, which matches the C++ ref. */
    int8_t corr = xaxis_correction(shape_get_boundary(s));
    if (corr) shape_update_position(s, corr, 0);

    if (board_collides(b, s, 0, 0)) {
        if (corr) shape_update_position(s, (int8_t)(-corr), 0);
        shape_update_shape(s, (int8_t)(-rotate));
        return 0;
    }
    return 1;
}
