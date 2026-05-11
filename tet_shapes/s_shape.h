#ifndef TET_SHAPES_S_SHAPE_H
#define TET_SHAPES_S_SHAPE_H

#include "utils.h"

/* NOTE: S's create_shape places a block at (x-2, y+1), so spawn x must be
 * >= 2 for the piece to fit on a board with x_min == 0. */
void s_shape_init(Shape *s, int8_t x, int8_t y, int8_t rotation);  /* rotation in 0..3 */

#endif
