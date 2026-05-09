#include "o_shape.h"

/* O tetromino: 2x2 square. No rotation -- all 4 quadrants look identical, so
 * we leave s->rotation at 0 forever. Mirrors C++ OShape::update_shape which
 * is just `update_boundary()`. */
static void o_create(Shape *s, int8_t x, int8_t y) {
    s->blocks[0].x = x;     s->blocks[0].y = y;
    s->blocks[1].x = x + 1; s->blocks[1].y = y;
    s->blocks[2].x = x;     s->blocks[2].y = y + 1;
    s->blocks[3].x = x + 1; s->blocks[3].y = y + 1;
}

static void o_update(Shape *s, int8_t rotate) {
    (void)rotate;                 /* deliberately ignored */
    shape_update_boundary(s);
}

static const ShapeVTable O_VT = {
    .update_shape = o_update,
    .create_shape = o_create,
};

void o_shape_init(Shape *s, int8_t x, int8_t y, int8_t rotation) {
    /* Pass rotation through; shape_init will spin update_shape that many times,
     * but each spin is a no-op so s->rotation stays 0. That's intentional. */
    shape_init(s, &O_VT, x, y, rotation);
}
