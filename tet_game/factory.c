#include "factory.h"
#include "i_shape.h"
#include "j_shape.h"
#include "l_shape.h"
#include "o_shape.h"
#include "s_shape.h"
#include "t_shape.h"
#include "z_shape.h"

/* Order is arbitrary; the RNG just indexes in. 7 entries x 4 bytes
 * (function pointer) + 1 byte (color) = ~40 B in the data section.
 * Could move to PROGMEM later if flash pressure shows up. */
const ShapeFactoryEntry shape_factory[SHAPE_KINDS] = {
    { i_shape_init, COLOR_I },
    { o_shape_init, COLOR_O },
    { t_shape_init, COLOR_T },
    { l_shape_init, COLOR_L },
    { j_shape_init, COLOR_J },
    { s_shape_init, COLOR_S },
    { z_shape_init, COLOR_Z },
};
