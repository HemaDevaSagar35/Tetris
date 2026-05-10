#include "hud.h"
#include "palette.h"
#include "factory.h"     /* COLOR_BG */

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
