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

/* Animation tick rates. With BOARD_W=10, the width sweep takes 5 ticks
 * (left/right meet in 5 steps from either direction); the height drop
 * takes BOARD_H = 20 ticks. At 30 ms/tick: ~150 ms sweep + ~600 ms drop
 * = ~750 ms total clear animation, which feels right for arcade Tetris. */
#define WIDTH_TICK_MS   30U
#define HEIGHT_TICK_MS  30U

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

/* ---- per-cell / per-row paint primitives -------------------------------- *
 * paint_cell paints one 8x8 block at board cell (y, x) using the colour
 * currently in cells[y][x] (palette[0] for empty). paint_row is just 10
 * paint_cells across one row. Both are used by the animation tick to push
 * tiny dirty regions to the LCD without flicker -- each cell transitions
 * directly old-colour -> new-colour, never through a BG flash.
 *
 * Sizing: paint_cell is ~0.3 ms of SPI per call (8x8 px = 128 bytes data
 * + a handful of command bytes). paint_row is ~2.8 ms. The animation
 * touches at most 2 cells per width tick and 2 rows per height tick, so
 * each tick is ~6 ms worst case against a 30 ms tick budget -- plenty of
 * headroom.
 *
 * Both move to tet_render/ alongside render_shape in step 6.
 * --------------------------------------------------------------------------*/
static void paint_cell(struct st7735 *lcd, const Board *b, uint8_t y, uint8_t x) {
    uint16_t rgb = palette[b->cells[y][x]];
    uint8_t xs = (uint8_t)(BOARD_OFFSET_X + x * BLOCK_PIXELS);
    uint8_t ys = (uint8_t)(BOARD_OFFSET_Y + y * BLOCK_PIXELS);
    ST7735_DrawRectangle(lcd, xs, xs + BLOCK_PIXELS - 1,
                              ys, ys + BLOCK_PIXELS - 1, rgb);
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

/* ---- line-clear animation state machine --------------------------------- *
 * Two sequential timer-driven phases replicate testing_main.cpp:373-425:
 *
 *   phase 1 (width sweep, ~150 ms):
 *     Erase cells (left, y) and (right, y) for every full row y.
 *     Direction is randomised 50/50:
 *       width_dir = +1 (outside-in): left starts at 0, right at W-1,
 *                  each tick pulls them inward.
 *       width_dir = -1 (inside-out): left starts at W/2 + (W%2 - 1),
 *                  right at W/2, each tick pushes them outward.
 *     For W = 10 either direction takes exactly 5 ticks.
 *     End: left/right cross or leave [0, W-1].
 *
 *   phase 2 (height drop, ~600 ms):
 *     For each row y from H-1 down to 0, copy cells[y] to cells[y+deltas[y]]
 *     and clear cells[y]. Full rows (already wiped in phase 1) are no-ops.
 *     End: height_y reaches -1.
 *
 * On end, height_peak is bumped (old + total_lines, clamped), the line
 * bookkeeping is reset, and the caller spawns the next piece.
 * --------------------------------------------------------------------------*/
typedef struct {
    uint8_t  phase;          /* 0 = idle, 1 = width, 2 = height            */
    int8_t   width_left;
    int8_t   width_right;
    int8_t   width_dir;      /* +1 outside-in, -1 inside-out               */
    int8_t   height_y;       /* row currently being shifted; counts down   */
    uint16_t prev_ms;        /* wall-clock of the last tick                */
} AnimState;

static void anim_start(AnimState *a, uint16_t now) {
    /* 50/50 direction roll. Reuses the spawn RNG -- one extra byte from
     * the cycle, no separate stream needed. */
    if (rng_next8() & 1) {
        a->width_dir   = +1;
        a->width_left  = 0;
        a->width_right = (int8_t)(BOARD_W - 1);
    } else {
        a->width_dir   = -1;
        a->width_right = (int8_t)(BOARD_W / 2);
        a->width_left  = (int8_t)(a->width_right + (BOARD_W % 2 - 1));
    }
    a->height_y = (int8_t)(BOARD_H - 1);
    a->prev_ms  = now;
    a->phase    = 1;
}

static void anim_tick(struct st7735 *lcd, Board *b, AnimState *a, uint16_t now) {
    if (a->phase == 1) {
        /* Width sweep. */
        if ((uint16_t)(now - a->prev_ms) < WIDTH_TICK_MS) return;
        a->prev_ms = now;

        if (a->width_left >= 0 && a->width_right < (int8_t)BOARD_W &&
            a->width_left <= a->width_right) {
            uint8_t L = (uint8_t)a->width_left;
            uint8_t R = (uint8_t)a->width_right;
            board_clean_lines_selectively(b, L, R);
            for (uint8_t y = 0; y < BOARD_H; y++) {
                if (!b->line_formed[y]) continue;
                paint_cell(lcd, b, y, L);
                if (L != R) paint_cell(lcd, b, y, R);
            }
            a->width_left  = (int8_t)(a->width_left  + a->width_dir);
            a->width_right = (int8_t)(a->width_right - a->width_dir);
        } else {
            /* Width done -- compute fall distances and step into height. */
            board_calculate_deltas(b);
            a->phase = 2;
        }
        return;
    }

    if (a->phase == 2) {
        /* Height drop. */
        if ((uint16_t)(now - a->prev_ms) < HEIGHT_TICK_MS) return;
        a->prev_ms = now;

        if (a->height_y >= 0) {
            uint8_t y = (uint8_t)a->height_y;

            /* Skip-conditions match board_clear_lines_selectively itself:
             * full rows (wiped by width sweep) and rows with no fall.    */
            if (!b->line_formed[y] && b->deltas[y] > 0) {
                uint8_t dst = (uint8_t)(y + b->deltas[y]);

                /* Snapshot the source row BEFORE the mutation so we know
                 * which columns actually carry content. Empty columns at
                 * the source stay empty at both source AND dest after
                 * the copy (the dest was already empty -- either wiped by
                 * the width sweep, or source-cleared by a previous tick
                 * with higher height_y). So we can skip painting empty
                 * columns entirely; only ~2-4 cells per row typically
                 * need touching. Trade: 10 bytes of stack for ~60% less
                 * SPI traffic per tick + no BG-over-BG repaints. */
                uint8_t had_content[BOARD_W];
                for (uint8_t x = 0; x < BOARD_W; x++) {
                    had_content[x] = b->cells[y][x];
                }

                board_clear_lines_selectively(b, a->height_y);

                /* Dest-first ordering. The eye sees the piece appear at
                 * its new row while the old row is still showing, then
                 * the old row clears. Source-first would create a brief
                 * gap (~4 ms) where the piece is "in the air" -- right at
                 * the edge of perceptual flicker and compounds visibly
                 * across many shift ticks on a tall stack. */
                for (uint8_t x = 0; x < BOARD_W; x++) {
                    if (!had_content[x]) continue;
                    paint_cell(lcd, b, dst, x);
                    paint_cell(lcd, b, y,   x);
                }
            }
            a->height_y--;
        } else {
            /* Animation finished. Bump height_peak the same way instant
             * clear did (old + total_lines, clamped at BOARD_H sentinel),
             * then reset line bookkeeping. Spawning is the caller's job. */
            int16_t new_peak = (int16_t)b->height_peak + (int16_t)b->total_lines;
            if (new_peak > (int16_t)BOARD_H) new_peak = (int16_t)BOARD_H;
            b->height_peak = (int8_t)new_peak;
            board_reset_lines(b);
            a->phase = 0;
        }
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
    draw_board_frame(&lcd);

    timer_init_1ms();
    buttons_init();
    rng_seed_from_adc();            /* ADC poll is sync; safe with IRQs off */
    sei();                          /* now Timer1 compare-match ISR can fire */

    /* Game state. Color is a sibling local, not a Shape field
     * (see avr-c-port-design.mdc). */
    Board     board;
    Shape     active;
    uint8_t   active_color_idx;
    uint8_t   in_hard_drop = 0;     /* 1 = fast-falling; inputs locked    */
    AnimState anim = {0};           /* phase=0 (idle); other fields seeded
                                       by anim_start when a clear begins  */

    board_init(&board);
    spawn_next(&active, &active_color_idx);
    render_shape(&lcd, &active, active_color_idx);

    uint16_t prev_ms = timer_now_ms();

    while (1) {
        uint16_t now = timer_now_ms();

        /* ---- line-clear animation short-circuit ------------------------ *
         * While a clear is animating, gravity and inputs are frozen. The
         * tick advances state on the timer and, if the animation just
         * finished, spawns the next piece + resets the gravity clock so
         * the new piece doesn't insta-drop on the very next iteration.
         * Mirrors the C++ pattern of gating spawn / gravity on
         * !board.is_lines_formed(). */
        if (anim.phase != 0) {
            anim_tick(&lcd, &board, &anim, now);
            if (anim.phase == 0) {
                spawn_next(&active, &active_color_idx);
                render_shape(&lcd, &active, active_color_idx);
                prev_ms = now;
            }
            continue;
        }

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

                /* Line clear (animated -- step 5). If any lines formed,
                 * hand off to the animation state machine; the short-circuit
                 * at the top of the loop will tick the clear over ~750 ms
                 * and spawn the next piece when it's done. If no lines,
                 * spawn immediately as before. */
                board_check_lines(&board);
                if (board.lines_filled) {
                    anim_start(&anim, now);
                } else {
                    spawn_next(&active, &active_color_idx);
                    render_shape(&lcd, &active, active_color_idx);
                }
            }
        }
    }
    return 0;
}
