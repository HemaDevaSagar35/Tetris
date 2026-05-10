#include "hud.h"
#include "palette.h"
#include "factory.h"     /* COLOR_BG, shape_factory, SHAPE_KINDS         */
#include "utils.h"       /* tet_shapes/utils.h: Shape, Pixel, Boundary,
                            SHAPE_BLOCKS, shape_get_boundary, shape_get_blocks */
#include "render.h"      /* BOARD_OFFSET_X, BOARD_PX_W, BOARD_PX_H        */

/* ---- score layout constants --------------------------------------------- */
#define SCORE_X        2     /* origin x in the left margin (1 px from edge) */
#define SCORE_Y        4     /* origin y from screen top                     */
#define SCORE_PX       2     /* pixel-doubling scale for the 3x5 bitmap      */
#define SCORE_GAP_PX   1     /* inter-digit gap in screen pixels             */
#define SCORE_DIGITS   3     /* 3 digits -> caps at 999                      */

/* ---- 3x5 digit bitmap --------------------------------------------------- *
 * Byte-for-byte the same table as testing_main.cpp ScoreBoard::DIGITS
 * (game/utils.h:127-145). 5 rows per digit, each row's low 3 bits encode
 * one row of the glyph: bit-2 = col 0 (leftmost), bit-1 = col 1, bit-0 = col 2.
 *
 * Sits in flash (avr-gcc puts static const tables in .rodata which is
 * read directly without pgm_read_byte on AVR with -O*; it's only 50 bytes
 * so we don't bother with PROGMEM). */
static const uint8_t digit_bitmap[10][5] = {
    /* 0 */ { 0b111, 0b101, 0b101, 0b101, 0b111 },
    /* 1 */ { 0b010, 0b110, 0b010, 0b010, 0b111 },
    /* 2 */ { 0b111, 0b001, 0b111, 0b100, 0b111 },
    /* 3 */ { 0b111, 0b001, 0b111, 0b001, 0b111 },
    /* 4 */ { 0b101, 0b101, 0b111, 0b001, 0b001 },
    /* 5 */ { 0b111, 0b100, 0b111, 0b001, 0b111 },
    /* 6 */ { 0b111, 0b100, 0b111, 0b101, 0b111 },
    /* 7 */ { 0b111, 0b001, 0b001, 0b001, 0b001 },
    /* 8 */ { 0b111, 0b101, 0b111, 0b101, 0b111 },
    /* 9 */ { 0b111, 0b101, 0b111, 0b001, 0b111 },
};

/* Paint one digit at (origin_x, origin_y). Each glyph cell becomes a
 * SCORE_PX x SCORE_PX rectangle -- on-bit = WHITE, off-bit = COLOR_BG.
 * We paint every cell (not just on-bits) so the previous digit at the
 * same coords is implicitly erased; no separate background pass needed. */
static void draw_digit(struct st7735 *lcd, uint8_t digit,
                       uint8_t origin_x, uint8_t origin_y) {
    uint16_t fg = WHITE;
    uint16_t bg = palette[COLOR_BG];
    for (uint8_t r = 0; r < 5; r++) {
        uint8_t mask = digit_bitmap[digit][r];
        for (uint8_t c = 0; c < 3; c++) {
            uint16_t color = ((mask >> (2 - c)) & 1u) ? fg : bg;
            uint8_t xs = (uint8_t)(origin_x + c * SCORE_PX);
            uint8_t ys = (uint8_t)(origin_y + r * SCORE_PX);
            ST7735_DrawRectangle(lcd, xs, xs + SCORE_PX - 1,
                                      ys, ys + SCORE_PX - 1, color);
        }
    }
}

void render_hud_score(struct st7735 *lcd, uint16_t score) {
    if (score > 999) score = 999;        /* saturate -- 3-digit field */

    uint8_t digits[SCORE_DIGITS];
    digits[0] = (uint8_t)(score / 100);
    digits[1] = (uint8_t)((score / 10) % 10);
    digits[2] = (uint8_t)(score % 10);

    uint8_t cursor_x  = SCORE_X;
    uint8_t advance_x = (uint8_t)(3 * SCORE_PX + SCORE_GAP_PX);
    for (uint8_t i = 0; i < SCORE_DIGITS; i++) {
        draw_digit(lcd, digits[i], cursor_x, SCORE_Y);
        cursor_x = (uint8_t)(cursor_x + advance_x);
    }
}

/* ---- next-piece preview ------------------------------------------------- */
void render_hud_next(struct st7735 *lcd, uint8_t kind) {
    /* Wipe the preview box to BG. Single window+stream call -- ~770 us
     * for 16*16 = 256 pixels at our 4 MHz SPI -- much faster than per-cell
     * painting. The block then writes ~64 px (4 cells * 16 px) on top, so
     * total render cost is ~1 ms; fine for a non-hot-path call. */
    ST7735_DrawRectangle(lcd, NEXT_BOX_X, NEXT_BOX_X + NEXT_BOX_W - 1,
                              NEXT_BOX_Y, NEXT_BOX_Y + NEXT_BOX_H - 1,
                              palette[COLOR_BG]);

    if (kind >= SHAPE_KINDS) return;     /* safety: caller-side bug -> no-op */

    /* Build the piece in canonical pose (rotation 0, anchor at origin).
     * shape_factory[kind].init calls create_shape then shape_update_boundary,
     * so tmp.limits is filled even though we never place this Shape on the
     * actual board. */
    Shape tmp;
    shape_factory[kind].init(&tmp, 0, 0, 0);
    uint16_t rgb = palette[shape_factory[kind].color_idx];

    /* Center the bounding box inside the 4x4 grid.
     *   x_min:  J places a block at (x-1, y+2) and S at (x-2, y+1) in
     *           canonical pose, so x_min < 0 for those two; subtracting
     *           x_min normalises the leftmost cell to column 0.
     *   y_min:  always 0 in canonical pose -- every create_shape places
     *           its anchor at (x, y) and only adds non-negative dy to
     *           the other blocks. So height = y_max + 1. */
    Boundary b = shape_get_boundary(&tmp);
    int8_t width  = (int8_t)(b.x_max - b.x_min + 1);
    int8_t height = (int8_t)(b.y_max + 1);
    int8_t cx     = (int8_t)((NEXT_BOX_CELLS - width)  / 2);
    int8_t cy     = (int8_t)((NEXT_BOX_CELLS - height) / 2);

    /* Paint each block as a NEXT_PX x NEXT_PX mini-rectangle. */
    const Pixel *blocks = shape_get_blocks(&tmp);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        int8_t bx = (int8_t)(blocks[i].x - b.x_min + cx);
        int8_t by = (int8_t)(blocks[i].y + cy);
        uint8_t xs = (uint8_t)(NEXT_BOX_X + bx * NEXT_PX);
        uint8_t ys = (uint8_t)(NEXT_BOX_Y + by * NEXT_PX);
        ST7735_DrawRectangle(lcd, xs, xs + NEXT_PX - 1,
                                  ys, ys + NEXT_PX - 1, rgb);
    }
}

/* ---- game-over overlay -------------------------------------------------- *
 * Geometry derivation:
 *   - X1 font glyph: 5 cols x 8 rows, advances 6 px per char (5 + 1 space).
 *   - Line widths (chars * 6 - 1):
 *       "GAME OVER"   ->  9 *6 - 1 = 53 px
 *       "PLAY AGAIN?" -> 11 *6 - 1 = 65 px
 *       "[YES]  NO"   ->  9 *6 - 1 = 53 px   (same width as "YES  [NO]")
 *   - Playfield: x = 25..104 (80 px), y = 0..159.
 *
 * Panel: inset 2 px from playfield edges -> x = 27..102 (76 wide). Three
 * 8-px text lines + padding need ~38 px tall; we round up to y = 58..102
 * (45 tall) so the panel reads as deliberate spacing, not cramped.
 *
 * Vertical layout (y offsets from OVL_Y0):
 *   +4   line 1  "GAME OVER"
 *   +18  line 2  "PLAY AGAIN?"
 *   +32  line 3  "[YES]  NO"  /  "YES  [NO]"
 * Each line is 8 px tall; gaps are 6 px; bottom padding = 102 - (58+32+8) = 4.
 * --------------------------------------------------------------------------*/
#define OVL_X0          (BOARD_OFFSET_X + 2)                /* 27 */
#define OVL_X1          (BOARD_OFFSET_X + BOARD_PX_W - 3)   /* 102 */
#define OVL_Y0          58
#define OVL_Y1          102
#define OVL_W           (OVL_X1 - OVL_X0 + 1)               /* 76 */

#define OVL_LINE1_Y_OFF  4
#define OVL_LINE2_Y_OFF  18
#define OVL_LINE3_Y_OFF  32

#define OVL_LINE1_W      53   /* "GAME OVER"   */
#define OVL_LINE2_W      65   /* "PLAY AGAIN?" */
#define OVL_LINE3_W      53   /* "[YES]  NO" or "YES  [NO]" -- same width */

void render_game_over_overlay(struct st7735 *lcd) {
    /* WHITE filled panel. Single DrawRectangle, ~10 ms SPI for 76*45 px. */
    ST7735_DrawRectangle(lcd, OVL_X0, OVL_X1, OVL_Y0, OVL_Y1, WHITE);

    /* Line 1: "GAME OVER", centered. */
    uint8_t l1_x = (uint8_t)(OVL_X0 + (OVL_W - OVL_LINE1_W) / 2);
    ST7735_SetPosition(l1_x, OVL_Y0 + OVL_LINE1_Y_OFF);
    ST7735_DrawString(lcd, "GAME OVER", BLACK, X1);

    /* Line 2: "PLAY AGAIN?", centered. */
    uint8_t l2_x = (uint8_t)(OVL_X0 + (OVL_W - OVL_LINE2_W) / 2);
    ST7735_SetPosition(l2_x, OVL_Y0 + OVL_LINE2_Y_OFF);
    ST7735_DrawString(lcd, "PLAY AGAIN?", BLACK, X1);

    /* Line 3 is the dynamic selection line -- caller paints it via
     * render_game_over_selection() once the initial cursor state is
     * decided. Keeping that out of here lets the caller redraw just
     * line 3 on every LEFT/RIGHT press without touching lines 1 / 2. */
}

void render_game_over_selection(struct st7735 *lcd, uint8_t selection) {
    /* Erase the line-3 strip back to WHITE first. The two strings have
     * brackets at different x-offsets ("[YES]  NO" puts brackets at
     * cols 0 and 4; "YES  [NO]" at cols 5 and 8), so overdrawing the
     * new string would leave stray bracket pixels from the old one.
     * Strip spans the full panel interior horizontally with a 1-px
     * vertical margin around the 8-px-tall glyphs.                    */
    uint8_t strip_y0 = (uint8_t)(OVL_Y0 + OVL_LINE3_Y_OFF - 1);
    uint8_t strip_y1 = (uint8_t)(OVL_Y0 + OVL_LINE3_Y_OFF + 8);
    ST7735_DrawRectangle(lcd, OVL_X0 + 1, OVL_X1 - 1,
                              strip_y0, strip_y1, WHITE);

    /* Draw the selection line, centered. Both strings are 9 chars wide
     * so the start_x is the same regardless of which one is active. */
    uint8_t l3_x = (uint8_t)(OVL_X0 + (OVL_W - OVL_LINE3_W) / 2);
    ST7735_SetPosition(l3_x, OVL_Y0 + OVL_LINE3_Y_OFF);
    ST7735_DrawString(lcd,
                      (selection == GAME_OVER_SEL_YES) ? "[YES]  NO"
                                                      : "YES  [NO]",
                      BLACK, X1);
}
