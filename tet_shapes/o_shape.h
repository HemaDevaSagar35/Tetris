#ifndef TET_SHAPES_O_SHAPE_H
#define TET_SHAPES_O_SHAPE_H

#include "utils.h"

/* O is rotationally symmetric -- the `rotation` argument is accepted for API
 * consistency but has no visual effect; the piece always reads as rotation 0. */
void o_shape_init(Shape *s, int8_t x, int8_t y, int8_t rotation);

#endif
