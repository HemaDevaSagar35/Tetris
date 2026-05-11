#ifndef TET_RENDER_RENDER_H
#define TET_RENDER_RENDER_H

#include <stdint.h>
#include "st7735.h"
#include "utils.h"      /* tet_shapes/utils.h: Shape, Pixel, SHAPE_BLOCKS */
#include "board.h"

/* ---- playfield geometry -------------------------------------------------- *
 * Classic Tetris is 10 cols x 20 rows. We render each cell as an 8x8 px
 * square -> board pixel size 80 x 160. The ST7735 is 130 x 161, so we
 * have ~25 px left and right of the playfield for the score (left) and
 * next-piece preview (right). Top/bottom are flush.
 *
 * Constants live here (not in board.h) because they're a rendering
 * concern -- the game state (Board) doesn't know or care about pixels.
 * --------------------------------------------------------------------------*/
#define BLOCK_PIXELS    8
#define SCREEN_W        130            /* matches MAX_X in common/st7735.h   */
#define SCREEN_H        161            /* matches MAX_Y in common/st7735.h   */
#define BOARD_PX_W      (BOARD_W * BLOCK_PIXELS)
#define BOARD_PX_H      (BOARD_H * BLOCK_PIXELS)
#define BOARD_OFFSET_X  ((SCREEN_W - BOARD_PX_W) / 2)   /* = 25 */
#define BOARD_OFFSET_Y  ((SCREEN_H - BOARD_PX_H) / 2)   /* =  0 */

/* Paint the active piece's 4 blocks at colour palette[color_idx]. Pass
 * COLOR_BG to erase. */
void render_shape(struct st7735 *lcd, const Shape *s, uint8_t color_idx);

/* Paint a single 8x8 board cell using whatever colour index is currently
 * stored in cells[y][x]. The animation tick uses this to push tiny dirty
 * regions to the LCD without a full repaint. */
void paint_cell(struct st7735 *lcd, const Board *b, uint8_t y, uint8_t x);

/* Draw a 1-px white "U" frame around the playfield (left + right +
 * bottom). Top is intentionally open so pieces visibly enter from above.
 * Called once at boot. */
void draw_board_frame(struct st7735 *lcd);

#endif
