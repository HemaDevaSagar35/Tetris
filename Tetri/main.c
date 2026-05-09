#include "main.h"
#include "st7735.h"
#include "timer.h"
#include "buttons.h"
#include "t_shape.h"

/* ---- tunable knobs ------------------------------------------------------- *
 * Classic Tetris playfield is 10 wide x 20 tall. ST7735 is 130 x 161 px.
 *   board pixel size = 10*8 x 20*8 = 80 x 160
 *   offsets center the board horizontally (vertically it's already flush).
 * --------------------------------------------------------------------------*/
#define BLOCK_PIXELS    8
#define BOARD_W         10
#define BOARD_H         20
#define GRAVITY_MS      1000U

#define SCREEN_W        130            /* matches MAX_X in common/st7735.h   */
#define SCREEN_H        161            /* matches MAX_Y in common/st7735.h   */
#define BOARD_PX_W      (BOARD_W * BLOCK_PIXELS)
#define BOARD_PX_H      (BOARD_H * BLOCK_PIXELS)
#define BOARD_OFFSET_X  ((SCREEN_W - BOARD_PX_W) / 2)   /* = 25 */
#define BOARD_OFFSET_Y  ((SCREEN_H - BOARD_PX_H) / 2)   /* =  0 */

/* ---- palette ------------------------------------------------------------- *
 * Will move to tet_render/palette.{h,c} when that module appears.
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
 * coords, offset into the centered playfield. Pass COLOR_BG to erase.
 * ST7735_DrawRectangle uses INCLUSIVE endpoints, so xe = xs + BLOCK_PIXELS-1
 * covers exactly BLOCK_PIXELS px.
 * --------------------------------------------------------------------------*/
static void render_shape(struct st7735 *lcd, const Shape *s, uint8_t color_idx) {
    uint16_t rgb = palette[color_idx];
    const Pixel *blocks = shape_get_blocks(s);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        uint8_t xs = (uint8_t)(BOARD_OFFSET_X + blocks[i].x * BLOCK_PIXELS);
        uint8_t ys = (uint8_t)(BOARD_OFFSET_Y + blocks[i].y * BLOCK_PIXELS);
        ST7735_DrawRectangle(lcd, xs, xs + BLOCK_PIXELS - 1,
                                  ys, ys + BLOCK_PIXELS - 1, rgb);
    }
}

/* ---- playfield frame ---------------------------------------------------- *
 * Draw a 1-px white "U" border around the playfield (left + right + bottom).
 * The top is intentionally open so pieces visibly enter from above.
 * Called once at boot. Move to tet_render/ alongside palette later.
 * --------------------------------------------------------------------------*/
static void draw_board_frame(struct st7735 *lcd) {
    uint8_t left   = BOARD_OFFSET_X - 1;
    uint8_t right  = BOARD_OFFSET_X + BOARD_PX_W;
    uint8_t top    = BOARD_OFFSET_Y;
    uint8_t bottom = BOARD_OFFSET_Y + BOARD_PX_H;        /* one px below playfield */

    ST7735_DrawRectangle(lcd, left,  left,  top, bottom, WHITE);   /* left bar   */
    ST7735_DrawRectangle(lcd, right, right, top, bottom, WHITE);   /* right bar  */
    ST7735_DrawRectangle(lcd, left,  right, bottom, bottom, WHITE);/* bottom bar */
}

/* ---- rotation helper ---------------------------------------------------- *
 * Stripped-down port of game/utils.h::do_valid_rotation, minus the board
 * collision check (no Board yet). Rotates in place, applies horizontal
 * wall-kick, and undoes the whole thing if the result still pokes off the
 * bottom. Will move to tet_game/rules.{h,c} when that module exists.
 *   `rotate` is +1 (clockwise) or -1 (anti-clockwise).
 * Returns 1 if the rotation took effect, 0 if it was reverted.
 * --------------------------------------------------------------------------*/
static uint8_t try_rotation(Shape *s, int8_t rotate) {
    shape_update_shape(s, rotate);

    Boundary b = shape_get_boundary(s);
    int8_t corr = 0;
    if (b.x_min < 0)                     corr = (int8_t)(-b.x_min);
    else if (b.x_max >= (int8_t)BOARD_W) corr = (int8_t)((BOARD_W - 1) - b.x_max);
    if (corr) shape_update_position(s, corr, 0);

    b = shape_get_boundary(s);
    if (b.y_max >= (int8_t)BOARD_H) {
        if (corr) shape_update_position(s, (int8_t)(-corr), 0);
        shape_update_shape(s, (int8_t)(-rotate));
        return 0;
    }
    return 1;
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
    draw_board_frame(&lcd);

    timer_init_1ms();
    buttons_init();
    sei();   /* now Timer1 compare-match ISR can actually fire */

    /* Game state: active piece + its color (sibling locals, color NOT on Shape) */
    Shape   active;
    uint8_t active_color_idx = COLOR_T;

    t_shape_init(&active, 3, 0, 0);                   /* spawn at top: x=3, y=0, rot 0 */
    render_shape(&lcd, &active, active_color_idx);

    uint16_t prev_ms = timer_now_ms();

    while (1) {
        uint16_t now = timer_now_ms();

        /* ---- input: left button -- move one column left ---------------- */
        if (button_left_just_pressed()) {
            Boundary b = shape_get_boundary(&active);
            if (b.x_min > 0) {
                render_shape(&lcd, &active, COLOR_BG);
                shape_update_position(&active, -1, 0);
                render_shape(&lcd, &active, active_color_idx);
            }
        }

        /* ---- input: right button -- move one column right -------------- */
        if (button_right_just_pressed()) {
            Boundary b = shape_get_boundary(&active);
            if (b.x_max < (int8_t)(BOARD_W - 1)) {
                render_shape(&lcd, &active, COLOR_BG);
                shape_update_position(&active, +1, 0);
                render_shape(&lcd, &active, active_color_idx);
            }
        }

        /* ---- input: rotate clockwise / anti-clockwise ------------------ *
         * try_rotation does the wall-kick and may revert. We erase, mutate,
         * repaint regardless -- on failed rotation the piece is back at its
         * original blocks, so the repaint is a no-op visually. */
        if (button_rotate_cw_just_pressed()) {
            render_shape(&lcd, &active, COLOR_BG);
            try_rotation(&active, +1);
            render_shape(&lcd, &active, active_color_idx);
        }
        if (button_rotate_ccw_just_pressed()) {
            render_shape(&lcd, &active, COLOR_BG);
            try_rotation(&active, -1);
            render_shape(&lcd, &active, active_color_idx);
        }

        /* ---- gravity: drop one row every GRAVITY_MS -------------------- */
        if ((uint16_t)(now - prev_ms) >= GRAVITY_MS) {
            prev_ms = now;

            Boundary b = shape_get_boundary(&active);
            if (b.y_max + 1 < BOARD_H) {
                render_shape(&lcd, &active, COLOR_BG);
                shape_update_position(&active, 0, 1);
                render_shape(&lcd, &active, active_color_idx);
            }
            /* else: piece is on the floor, stays put forever */
        }
    }
    return 0;
}
