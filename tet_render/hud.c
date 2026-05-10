#include "hud.h"
#include "palette.h"
#include "factory.h"     /* COLOR_BG, shape_factory, SHAPE_KINDS         */
#include "utils.h"       /* tet_shapes/utils.h: Shape, Pixel, Boundary,
                            SHAPE_BLOCKS, shape_get_boundary, shape_get_blocks */

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
