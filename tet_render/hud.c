#include "hud.h"
#include "palette.h"
#include "factory.h"     /* COLOR_BG, shape_factory, SHAPE_KINDS         */
#include "utils.h"       /* tet_shapes/utils.h: Shape, Pixel, Boundary,
                            SHAPE_BLOCKS, shape_get_boundary, shape_get_blocks */
#include "render.h"      /* BOARD_OFFSET_X, BOARD_PX_W, BOARD_PX_H        */

/* ---- score layout constants --------------------------------------------- *
 * Public x/y origins live in hud.h (HUD_LEFT_X, HUD_SCORE_DIG_Y,
 * HUD_MAX_DIG_Y, HUD_LVL_DIG_Y, HUD_*_LABEL_Y). The pixel-scale
 * tuning stays private here.
 *
 * SCORE_PX shrank from 2 -> 1 in step 9 so we can fit 6 digits in the
 * 25-px left margin (6 * 3 + 5 * 1 = 23 px). Trade: digits are half as
 * tall and half as wide. Inter-digit gap stays at 1 px for legibility.
 * Score readouts are 6 digits (cap 999,999 -- NES range), level readout
 * is 2 digits (cap 29 -- NES kill screen).                              */
#define SCORE_PX       1     /* 1:1 scale for the 3x5 bitmap                  */
#define SCORE_GAP_PX   1     /* inter-digit gap in screen pixels              */
#define SCORE_DIGITS   6     /* 6 digits -> caps at 999,999                   */
#define LVL_DIGITS     2     /* 2 digits -> caps at 99 (we clamp to 29)       */

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

/* Shared N-digit painter. Caller picks the y-origin and digit count so
 * the same routine paints all three readouts (6-digit score / max,
 * 2-digit level). All readouts share the x-origin HUD_LEFT_X to stay
 * visually column-aligned.
 *
 * `n_digits` is bounded at compile time by callers (max 6 today), but we
 * iterate up to a small fixed cap so the digits[] stack buffer stays
 * bounded -- no heap, no VLA, and the buffer cost is one byte per
 * possible digit. 8 is generous (allows up to 8-digit numbers if we
 * ever widen the field).
 *
 * Saturation: any value that won't fit in n_digits decimal digits gets
 * clamped to "all 9s" so the field never wraps to 0 silently.          */
static void draw_Ndigits(struct st7735 *lcd, uint32_t value,
                         uint8_t origin_y, uint8_t n_digits) {
    /* Compute 10^n_digits to derive the saturation cap. n_digits is
     * small (<=8); this loop is cheap and avoids pulling in a generic
     * pow(). */
    uint32_t cap = 1;
    for (uint8_t i = 0; i < n_digits; i++) cap *= 10u;
    if (value >= cap) value = cap - 1u;

    /* Extract digits from least-significant to most-significant, then
     * paint left-to-right. Stack buffer sized for the largest n_digits
     * we expect (6 for score, 2 for level). */
    uint8_t digits[8];
    for (uint8_t i = 0; i < n_digits; i++) {
        digits[n_digits - 1 - i] = (uint8_t)(value % 10u);
        value /= 10u;
    }

    uint8_t cursor_x  = HUD_LEFT_X;
    uint8_t advance_x = (uint8_t)(3 * SCORE_PX + SCORE_GAP_PX);
    for (uint8_t i = 0; i < n_digits; i++) {
        draw_digit(lcd, digits[i], cursor_x, origin_y);
        cursor_x = (uint8_t)(cursor_x + advance_x);
    }
}

void render_hud_labels(struct st7735 *lcd) {
    /* "SCR" / "MAX" / "LVL" never change once painted; called once per
     * game (in reset_game) and not on every value update. WHITE text on
     * BLACK background -- BG was just cleared by ST7735_ClearScreen, so
     * no separate erase is needed. */
    ST7735_SetPosition(HUD_LEFT_X, HUD_SCR_LABEL_Y);
    ST7735_DrawString(lcd, "SCR", WHITE, X1);
    ST7735_SetPosition(HUD_LEFT_X, HUD_MAX_LABEL_Y);
    ST7735_DrawString(lcd, "MAX", WHITE, X1);
    ST7735_SetPosition(HUD_LEFT_X, HUD_LVL_LABEL_Y);
    ST7735_DrawString(lcd, "LVL", WHITE, X1);
}

void render_hud_score(struct st7735 *lcd, uint32_t score) {
    draw_Ndigits(lcd, score, HUD_SCORE_DIG_Y, SCORE_DIGITS);
}

void render_hud_max_score(struct st7735 *lcd, uint32_t max_score) {
    draw_Ndigits(lcd, max_score, HUD_MAX_DIG_Y, SCORE_DIGITS);
}

void render_hud_level(struct st7735 *lcd, uint8_t level) {
    /* level is 0..29 in practice (gravity table clamps); widened to
     * uint32_t here just to fit draw_Ndigits' signature. */
    draw_Ndigits(lcd, (uint32_t)level, HUD_LVL_DIG_Y, LVL_DIGITS);
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

/* ---- start-screen overlay ---------------------------------------------- *
 * Reuses the same 76 x 45 panel rectangle as game-over for visual
 * consistency. Two lines, vertically centered:
 *
 *     "TETRIS"   X2 font -- 16 px tall, 6 px advance per char (X2 only
 *                doubles HEIGHT, not width; see ST7735_DrawChar). So
 *                "TETRIS" is 6 chars * 5 + 5 gaps = 35 px wide.
 *
 *     "[PLAY]"   X1 font -- 8 px tall, also 35 px wide for 6 chars.
 *
 * Both lines are the same width by happy coincidence, so they stack as
 * a clean column centered in the panel. Y offsets from OVL_Y0 leave a
 * 6 px top pad, 6 px between lines, 5 px bottom pad. */
#define START_TITLE_Y_OFF   6     /* title -> uses y = 64..79  */
#define START_PLAY_Y_OFF    28    /* button -> uses y = 86..93 */

#define START_TITLE_W       35    /* X2 "TETRIS"  visible width */
#define START_PLAY_W        35    /* X1 "[PLAY]"  visible width */

void render_start_overlay(struct st7735 *lcd) {
    /* WHITE panel. Reuses OVL_X0/Y0/Y1/W from the game-over overlay --
     * the panel geometry is shared by design so the two screens look
     * like the same dialog box. */
    ST7735_DrawRectangle(lcd, OVL_X0, OVL_X1, OVL_Y0, OVL_Y1, WHITE);

    /* Title: "TETRIS" X2, centered horizontally. */
    uint8_t t_x = (uint8_t)(OVL_X0 + (OVL_W - START_TITLE_W) / 2);
    ST7735_SetPosition(t_x, OVL_Y0 + START_TITLE_Y_OFF);
    ST7735_DrawString(lcd, "TETRIS", BLACK, X2);

    /* "[PLAY]" button, centered. The brackets are intentional -- they
     * read as a button affordance even without colour cues. */
    uint8_t p_x = (uint8_t)(OVL_X0 + (OVL_W - START_PLAY_W) / 2);
    ST7735_SetPosition(p_x, OVL_Y0 + START_PLAY_Y_OFF);
    ST7735_DrawString(lcd, "[PLAY]", BLACK, X1);
}
