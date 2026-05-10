#include "main.h"
#include "st7735.h"
#include "timer.h"
#include "buttons.h"
#include "board.h"
#include "rules.h"
#include "factory.h"
#include "rng.h"

/* ---- tunable knobs ------------------------------------------------------- *
 * Classic Tetris playfield is 10 wide x 20 tall (BOARD_W/BOARD_H now live in
 * tet_game/board.h since they describe game state, not rendering). ST7735 is
 * 130 x 161 px.
 *   board pixel size = 10*8 x 20*8 = 80 x 160
 *   offsets center the board horizontally (vertically it's already flush).
 * --------------------------------------------------------------------------*/
#define BLOCK_PIXELS    8
#define GRAVITY_MS      1000U
#define HARD_DROP_MS    62U      /* matches downward_ghost_speed in C++ ref  */

#define SCREEN_W        130            /* matches MAX_X in common/st7735.h   */
#define SCREEN_H        161            /* matches MAX_Y in common/st7735.h   */
#define BOARD_PX_W      (BOARD_W * BLOCK_PIXELS)
#define BOARD_PX_H      (BOARD_H * BLOCK_PIXELS)
#define BOARD_OFFSET_X  ((SCREEN_W - BOARD_PX_W) / 2)   /* = 25 */
#define BOARD_OFFSET_Y  ((SCREEN_H - BOARD_PX_H) / 2)   /* =  0 */

/* ---- palette ------------------------------------------------------------- *
 * COLOR_* indices live in tet_game/factory.h (they're game-state values,
 * stored in the Board). This array is the renderer-side mapping from
 * those indices to RGB565 for the ST7735. Both move to
 * tet_render/palette.{h,c} in step 6.
 * --------------------------------------------------------------------------*/
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

/* ---- spawn --------------------------------------------------------------- *
 * Roll a random piece, x, and rotation; init it in `s`; write its color
 * index to `*color_idx`; and kick it back on board if the random x left
 * the rotated shape poking off a side edge.
 *
 * Ports the C++ spawn block at testing_main.cpp:228-245 minus the
 * "is_lines_formed()" gate (step 4) and the height_peak <= y_max game-over
 * check (step 7). Game-over: if the new piece overlaps the stack at spawn,
 * the next gravity tick latches it in place; the board will visibly fill
 * up. We'll add the proper game-over check in step 7.
 *
 * Bit-level choices:
 *   - `% SHAPE_KINDS`  - 7 isn't a power of 2, so ~3% bias toward shapes
 *                        0..2. Acceptable; switching to rejection sampling
 *                        costs a loop and we don't care for a hobby game.
 *   - `% BOARD_W`      - 10 isn't a power of 2 either; same ~4% bias.
 *                        xaxis_correction handles the off-board case after.
 *   - `& 0x03`         - rotation is 0..3, power of 2, no bias.
 * --------------------------------------------------------------------------*/
static void spawn_next(Shape *s, uint8_t *color_idx) {
    uint8_t shape_idx = (uint8_t)(rng_next8() % SHAPE_KINDS);
    int8_t  spawn_x   = (int8_t)(rng_next8() % BOARD_W);
    int8_t  rotation  = (int8_t)(rng_next8() & 0x03);

    shape_factory[shape_idx].init(s, spawn_x, 0, rotation);
    *color_idx = shape_factory[shape_idx].color_idx;

    /* Random x + rotation can put blocks at x < 0 or x >= BOARD_W. Same
     * wall-kick we use during gameplay rotations. */
    int8_t corr = xaxis_correction(shape_get_boundary(s));
    if (corr) shape_update_position(s, corr, 0);
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
    rng_seed_from_adc();            /* ADC poll is sync; safe with IRQs off */
    sei();                          /* now Timer1 compare-match ISR can fire */

    /* Game state. Color is a sibling local, not a Shape field
     * (see avr-c-port-design.mdc). */
    Board   board;
    Shape   active;
    uint8_t active_color_idx;
    uint8_t in_hard_drop = 0;       /* 1 = fast-falling; inputs locked */

    board_init(&board);
    spawn_next(&active, &active_color_idx);
    render_shape(&lcd, &active, active_color_idx);

    uint16_t prev_ms = timer_now_ms();

    while (1) {
        uint16_t now = timer_now_ms();

        /* ---- inputs (gated entirely while hard-drop is in progress) ---- *
         * Mirrors the C++ pattern of guarding every input with
         * `(downward_ghost == -1)` -- collapsed into one outer `if` here.
         * The hard-drop button lives inside this block too, so re-pressing
         * it during a hard-drop is a no-op for free. */
        if (!in_hard_drop) {
            /* All four movement handlers share the same erase-mutate-repaint
             * pattern: do_valid_move/rotation either commits the change or
             * reverts internally, so on failure we erase and repaint the
             * exact same pixels (one frame of no-op). The simplicity beats
             * saving a Shape backup; see avr-c-port-design.mdc "Known
             * caveat -- unconditional repaint on failed rotation". */
            if (button_left_just_pressed()) {
                render_shape(&lcd, &active, COLOR_BG);
                do_valid_move(&active, &board, -1);
                render_shape(&lcd, &active, active_color_idx);
            }
            if (button_right_just_pressed()) {
                render_shape(&lcd, &active, COLOR_BG);
                do_valid_move(&active, &board, +1);
                render_shape(&lcd, &active, active_color_idx);
            }
            if (button_rotate_cw_just_pressed()) {
                render_shape(&lcd, &active, COLOR_BG);
                do_valid_rotation(&active, &board, +1);
                render_shape(&lcd, &active, active_color_idx);
            }
            if (button_rotate_ccw_just_pressed()) {
                render_shape(&lcd, &active, COLOR_BG);
                do_valid_rotation(&active, &board, -1);
                render_shape(&lcd, &active, active_color_idx);
            }

            /* Down -> enter hard drop. Inputs lock until the piece can no
             * longer fall (handled in the gravity branch below). */
            if (button_down_just_pressed()) {
                in_hard_drop = 1;
            }
        }

        /* ---- gravity: GRAVITY_MS normally, HARD_DROP_MS while fast-falling. *
         * Collision is now board-aware (board_collides catches both the floor
         * and any latched cell underneath). When the piece can't fall further
         * we latch it into the board and spawn the next one. */
        uint16_t step_ms = in_hard_drop ? HARD_DROP_MS : GRAVITY_MS;
        if ((uint16_t)(now - prev_ms) >= step_ms) {
            prev_ms = now;

            if (!board_collides(&board, &active, 0, 1)) {
                render_shape(&lcd, &active, COLOR_BG);
                shape_update_position(&active, 0, 1);
                render_shape(&lcd, &active, active_color_idx);
            } else {
                /* Lock: cells take ownership; pixels are already correct so
                 * no extra paint needed for the latched piece. Exit hard-drop
                 * so inputs unlock for the next piece. Game-over (spawn into
                 * an occupied cell) is deferred to step 7. */
                board_latch(&board, &active, active_color_idx);
                in_hard_drop = 0;

                spawn_next(&active, &active_color_idx);
                render_shape(&lcd, &active, active_color_idx);
            }
        }
    }
    return 0;
}
