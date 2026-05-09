#include "z_shape.h"

/* Z tetromino. C++ reference: shapes/z_shape.h ZShape::create_shape /
 * update_shape. */
static void z_create(Shape *s, int8_t x, int8_t y) {
    s->blocks[0].x = x;     s->blocks[0].y = y;
    s->blocks[1].x = x + 1; s->blocks[1].y = y;
    s->blocks[2].x = x + 1; s->blocks[2].y = y + 1;
    s->blocks[3].x = x + 2; s->blocks[3].y = y + 1;
}

static void z_update(Shape *s, int8_t rotate) {
    if (s->rotation == 0) {
        if (rotate == 1) {
            /* 0 -> 1 */
            s->blocks[0].x += 2;
            s->blocks[1].x += 1; s->blocks[1].y += 1;
            s->blocks[3].x -= 1; s->blocks[3].y += 1;
        } else {
            /* 0 -> 3 */
            s->blocks[0].y += 2;
            s->blocks[1].x -= 1; s->blocks[1].y += 1;
            s->blocks[3].x -= 1; s->blocks[3].y -= 1;
        }
    } else if (s->rotation == 1) {
        if (rotate == 1) {
            /* 1 -> 2 */
            s->blocks[0].y += 1;
            s->blocks[1].x -= 1;
            s->blocks[2].y -= 1;
            s->blocks[3].x -= 1; s->blocks[3].y -= 2;
        } else {
            /* 1 -> 0 */
            s->blocks[0].x -= 2;
            s->blocks[1].x -= 1; s->blocks[1].y -= 1;
            s->blocks[3].x += 1; s->blocks[3].y -= 1;
        }
    } else if (s->rotation == 2) {
        if (rotate == 1) {
            /* 2 -> 3 */
            s->blocks[0].x -= 2; s->blocks[0].y += 1;
            s->blocks[1].x -= 1;
            s->blocks[2].y += 1;
            s->blocks[3].x += 1;
        } else {
            /* 2 -> 1 */
            s->blocks[0].y -= 1;
            s->blocks[1].x += 1;
            s->blocks[2].y += 1;
            s->blocks[3].x += 1; s->blocks[3].y += 2;
        }
    } else {
        /* s->rotation == 3 */
        if (rotate == 1) {
            /* 3 -> 0 */
            s->blocks[0].y -= 2;
            s->blocks[1].x += 1; s->blocks[1].y -= 1;
            s->blocks[3].x += 1; s->blocks[3].y += 1;
        } else {
            /* 3 -> 2 */
            s->blocks[0].x += 2; s->blocks[0].y -= 1;
            s->blocks[1].x += 1;
            s->blocks[2].y -= 1;
            s->blocks[3].x -= 1;
        }
    }

    s->rotation = (int8_t)((s->rotation + rotate + 4) & 0x03);
    shape_update_boundary(s);
}

static const ShapeVTable Z_VT = {
    .update_shape = z_update,
    .create_shape = z_create,
};

void z_shape_init(Shape *s, int8_t x, int8_t y, int8_t rotation) {
    shape_init(s, &Z_VT, x, y, rotation);
}
