#ifndef TET_SHAPES_J_SHAPE_H
#define TET_SHAPES_J_SHAPE_H

#include "utils.h"

/* NOTE: J's create_shape places a block at (x-1, y+2), so spawn x must be
 * >= 1 for the piece to fit on a board with x_min == 0. */
void j_shape_init(Shape *s, int8_t x, int8_t y, int8_t rotation);  /* rotation in 0..3 */

#endif
