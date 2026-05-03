#include "main.h"
#include "st7735.h"
#include "timer.h"
#include "t_shape.h"

/* ---- tunable knobs ------------------------------------------------------- */
#define BLOCK_PIXELS 10        /* size of one tetromino block, in display px */
#define BOARD_W      10
#define BOARD_H      16        /* fits in 161-px-tall ST7735 at 10 px/block  */
#define GRAVITY_MS   1000U     /* drop interval                              */

/* ---- palette ------------------------------------------------------------- *
 * Will move to tet_render/palette.{h,c} when that module appears. Keeping
 * it here for now so the gravity test is self-contained.
 * --------------------------------------------------------------------------*/
#define COLOR_BG     0
#define COLOR_T      1
#define COLOR_I      2
#define COLOR_O      3
#define COLOR_L      4
#define COLOR_J      5
#define COLOR_S      6
#define COLOR_Z      7
#define COLOR_COUNT  8

static const uint16_t palette[COLOR_COUNT] = {
    [COLOR_BG] = BLACK,
    [COLOR_T]  = 0xFFE0,    /* yellow */
    [COLOR_I]  = 0x07FF,    /* cyan   */
    [COLOR_O]  = 0xFD20,    /* orange */
    [COLOR_L]  = 0x001F,    /* blue   */
    [COLOR_J]  = 0xFB56,    /* pink   */
    [COLOR_S]  = 0x07E0,    /* green  */
    [COLOR_Z]  = 0xF800,    /* red    */
};

/* ---- rendering ----------------------------------------------------------- *
 * Renders 4 filled rectangles for the tetromino at its current logical
 * coords. Pass COLOR_BG to erase. ST7735_DrawRectangle uses INCLUSIVE
 * endpoints, so xe = xs + BLOCK_PIXELS - 1 (covers exactly BLOCK_PIXELS px).
 * --------------------------------------------------------------------------*/
static void render_shape(struct st7735 *lcd, const Shape *s, uint8_t color_idx) {
    uint16_t rgb = palette[color_idx];
    const Pixel *blocks = shape_get_blocks(s);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        uint8_t xs = (uint8_t)(blocks[i].x * BLOCK_PIXELS);
        uint8_t ys = (uint8_t)(blocks[i].y * BLOCK_PIXELS);
        ST7735_DrawRectangle(lcd, xs, xs + BLOCK_PIXELS - 1,
                                  ys, ys + BLOCK_PIXELS - 1, rgb);
    }
}

/* ---- main ---------------------------------------------------------------- */
int main(void) {
    struct signal cs = { .ddr = &DDRB, .port = &PORTB, .pin = 4 };  /* SS  on PB4 */
    struct signal bl = { .ddr = &DDRD, .port = &PORTD, .pin = 7 };  /* backlight  */
    struct signal dc = { .ddr = &DDRD, .port = &PORTD, .pin = 5 };  /* RS  on TFT */
    struct signal rs = { .ddr = &DDRD, .port = &PORTD, .pin = 6 };  /* RST on TFT */
    struct st7735 lcd = { .cs = &cs, .bl = &bl, .dc = &dc, .rs = &rs };

    ST7735_Init(&lcd);
    ST7735_ClearScreen(&lcd, BLACK);

    timer_init_1ms();
    sei();   /* now Timer1 compare-match ISR can actually fire */

    /* Game state: active piece + its color (sibling locals, color NOT on Shape) */
    Shape   active;
    uint8_t active_color_idx = COLOR_T;

    t_shape_init(&active, 3, 0, 0);              /* spawn at top-ish: x=3, y=0, rot 0 */
    render_shape(&lcd, &active, active_color_idx);

    uint16_t prev_ms = timer_now_ms();

    while (1) {
        uint16_t now = timer_now_ms();

        /* Unsigned subtraction handles g_ms wraparound (every ~65.5 s). */
        if ((uint16_t)(now - prev_ms) >= GRAVITY_MS) {
            prev_ms = now;

            Boundary b = shape_get_boundary(&active);
            if (b.y_max + 1 < BOARD_H) {
                render_shape(&lcd, &active, COLOR_BG);     /* erase */
                shape_update_position(&active, 0, 1);      /* drop one row */
                render_shape(&lcd, &active, active_color_idx); /* repaint */
            }
            /* else: piece is on the floor, stays put forever */
        }
    }
    return 0;
}
