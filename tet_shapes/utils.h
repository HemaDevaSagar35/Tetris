#ifndef TET_SHAPES_UTILS_H
#define TET_SHAPES_UTILS_H

#include <stdint.h>

#define SHAPE_BLOCKS 4

typedef struct {
    int8_t x;
    int8_t y;
} Pixel;

typedef struct {
    int8_t x_min;
    int8_t x_max;
    int8_t y_max;
} Boundary;

struct Shape;

typedef struct {
    void (*update_shape)(struct Shape *self, int8_t rotate);
    void (*create_shape)(struct Shape *self, int8_t x, int8_t y);
} ShapeVTable;

typedef struct Shape {
    const ShapeVTable *vt;
    Pixel    blocks[SHAPE_BLOCKS];
    int8_t   rotation;             /* quadrant index 0..3 (0=0deg, 1=90, 2=180, 3=270) */
    Boundary limits;
} Shape;

void         shape_update_position(Shape *s, int8_t dx, int8_t dy);
void         shape_update_boundary(Shape *s);
Boundary     shape_get_boundary(const Shape *s);
const Pixel *shape_get_blocks(const Shape *s);

static inline void shape_update_shape(Shape *s, int8_t rotate) {
    s->vt->update_shape(s, rotate);
}

void shape_init(Shape *s, const ShapeVTable *vt,
                int8_t x, int8_t y, int8_t rotation);  /* rotation in 0..3 */

#endif
