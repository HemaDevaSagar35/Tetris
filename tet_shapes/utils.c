#include "utils.h"

void shape_update_position(Shape *s, int8_t dx, int8_t dy) {
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        s->blocks[i].x += dx;
        s->blocks[i].y += dy;
    }
    shape_update_boundary(s);
}

void shape_update_boundary(Shape *s) {
    int8_t x_min = INT8_MAX;
    int8_t x_max = 0;
    int8_t y_max = 0;
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        int8_t bx = s->blocks[i].x;
        int8_t by = s->blocks[i].y;
        if (bx < x_min) x_min = bx;
        if (bx > x_max) x_max = bx;
        if (by > y_max) y_max = by;
    }
    s->limits.x_min = x_min;
    s->limits.x_max = x_max;
    s->limits.y_max = y_max;
}

Boundary shape_get_boundary(const Shape *s) {
    return s->limits;
}

const Pixel *shape_get_blocks(const Shape *s) {
    return s->blocks;
}

void shape_init(Shape *s, const ShapeVTable *vt,
                int8_t x, int8_t y, int8_t rotation) {
    s->vt = vt;
    s->rotation = 0;
    s->limits.x_min = INT8_MAX;
    s->limits.x_max = 0;
    s->limits.y_max = 0;

    vt->create_shape(s, x, y);
    for (int8_t i = 0; i < rotation; i++) {
        vt->update_shape(s, 1);
    }
    shape_update_boundary(s);
}
