#include "main.h"
#include <avr/eeprom.h>
#include "st7735.h"
#include "timer.h"
#include "buttons.h"
#include "board.h"
#include "rules.h"
#include "factory.h"
#include "rng.h"
#include "render.h"      /* render_shape, paint_cell, draw_board_frame,
                            BLOCK_PIXELS, BOARD_OFFSET_*                  */
#include "hud.h"         /* render_hud_score, _max_score, _labels,
                            _start_overlay, game-over overlay helpers     */

/* ---- persistent max score (EEPROM) -------------------------------------- *
 * Single uint16_t stored in EEPROM byte address 0..1 (the linker assigns
 * `EEMEM` variables in declaration order; this is the only one we use).
 *
 * Lifecycle:
 *   - Boot: eeprom_read_word() -> max_score. A freshly-flashed chip reads
 *     0xFFFF for un-touched bytes; we treat that as "no max yet" and
 *     display 000. The HEX flasher can also explicitly zero the EEPROM
 *     section, but we don't depend on that.
 *   - Game-over: if score > max_score, update RAM copy + eeprom_update_word()
 *     (which skips the write when bytes are already correct, so no
 *     unnecessary wear). EEPROM is rated ~100k writes; a serious player
 *     setting a new high score every minute would still take ~70 days
 *     of continuous play to wear out the cell.
 *   - Reset (YES + DOWN): max_score in RAM is intentionally NOT cleared.
 *     The only way to clear is to reflash with -e (chip erase).
 *
 * `EEMEM` is an avr-gcc attribute that places the variable in the .eeprom
 * section -- compiled to a separate .eep file by the Makefile and burned
 * with `avrdude -U eeprom:w:...`. The variable's *flash* footprint is 0
 * because EEPROM lives in a different memory.                            */
static uint16_t EEMEM ee_max_score = 0;

/* ---- tunable knobs ------------------------------------------------------- *
 * Rendering geometry (BLOCK_PIXELS, BOARD_OFFSET_*, palette) moved to
 * tet_render/. main.c only holds game-loop tuning now.
 * --------------------------------------------------------------------------*/
#define GRAVITY_MS      1000U
#define HARD_DROP_MS    62U      /* matches downward_ghost_speed in C++ ref  */

/* Animation tick rates. With BOARD_W=10, the width sweep takes 5 ticks
 * (left/right meet in 5 steps from either direction); the height drop
 * takes BOARD_H = 20 ticks. At 30 ms/tick: ~150 ms sweep + ~600 ms drop
 * = ~750 ms total clear animation, which feels right for arcade Tetris. */
#define WIDTH_TICK_MS   30U
#define HEIGHT_TICK_MS  30U

/* Score cap matches the 3-digit HUD field. Score is a running total of
 * lines cleared, ported verbatim from testing_main.cpp:418-420. */
#define SCORE_MAX       999U

/* ---- spawn / queue ------------------------------------------------------- *
 * Step 6 phase 2 introduces a 1-deep queue so the right-margin preview can
 * show "what's coming next". The active piece is the piece you're playing
 * with; `next_kind` is the kind that will be promoted on the next spawn.
 *
 * Flow per spawn (boot, post-clear, post-lock):
 *   1. spawn_with_kind(active, next_kind, color)  -- use queued kind for
 *                                                    active piece
 *   2. next_kind = roll_kind()                    -- queue a new kind
 *   3. render_shape(active)                       -- paint active on board
 *   4. render_hud_next(next_kind)                 -- paint preview
 *
 * `promote_and_queue` bundles those four steps. Boot also needs a one-time
 * `next_kind = roll_kind()` before the first call (otherwise the first
 * active piece would be undefined).
 *
 * Ports the C++ spawn block at testing_main.cpp:228-245 with two
 * divergences:
 *   - C++ rolls a fresh kind inline; we promote a queued kind so the
 *     preview can show one piece ahead.
 *   - Game-over check (step 7): we use `board_collides(b, active, 0, 0)`
 *     after building the piece, whereas C++ uses `height_peak <= y_max`
 *     (testing_main.cpp:258). Collision is slightly more "classic" --
 *     it only triggers when the spawned blocks ACTUALLY overlap the
 *     stack, so a line-clear that opens a slot can rescue the spawn.
 *     C++ is more pessimistic and ends the game even if the piece would
 *     have fit.
 *
 * Bit-level choices:
 *   - `% SHAPE_KINDS`  - 7 isn't a power of 2, so ~3% bias toward kinds
 *                        0..2. Acceptable; rejection sampling costs a
 *                        loop and the visible bias is tiny for a hobby
 *                        game.
 *   - `% BOARD_W`      - 10 isn't a power of 2 either; same ~4% bias.
 *                        xaxis_correction handles the off-board case after.
 *   - `& 0x03`         - rotation is 0..3, power of 2, no bias.
 * --------------------------------------------------------------------------*/
static uint8_t roll_kind(void) {
    return (uint8_t)(rng_next8() % SHAPE_KINDS);
}

static void spawn_with_kind(Shape *s, uint8_t kind, uint8_t *color_idx) {
    int8_t spawn_x  = (int8_t)(rng_next8() % BOARD_W);
    int8_t rotation = (int8_t)(rng_next8() & 0x03);

    shape_factory[kind].init(s, spawn_x, 0, rotation);
    *color_idx = shape_factory[kind].color_idx;

    /* Random x + rotation can put blocks at x < 0 or x >= BOARD_W. Same
     * wall-kick we use during gameplay rotations. */
    int8_t corr = xaxis_correction(shape_get_boundary(s));
    if (corr) shape_update_position(s, corr, 0);
}

/* Returns 1 iff the new active piece overlaps the latched stack at its
 * spawn position -- the game-over trigger. On game over we deliberately
 * SKIP rendering the piece and updating the preview: the next thing the
 * caller does is paint the GAME OVER overlay over the playfield, and
 * showing a piece "stuck in" the stack for one frame before the overlay
 * lands would just look like a glitch. */
static uint8_t promote_and_queue(struct st7735 *lcd, Shape *active,
                                 uint8_t *color_idx, uint8_t *next_kind,
                                 const Board *board) {
    spawn_with_kind(active, *next_kind, color_idx);
    if (board_collides(board, active, 0, 0)) {
        return 1;
    }
    *next_kind = roll_kind();
    render_shape(lcd, active, *color_idx);
    render_hud_next(lcd, *next_kind);
    return 0;
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

/* ---- reset --------------------------------------------------------------- *
 * Tear everything back to a clean playable state. Called from two places:
 *   - First entry into gameplay (DOWN press on the start screen).
 *   - YES + DOWN on the game-over overlay.
 *
 * Order matters:
 *   1. Clear screen + redraw frame BEFORE any HUD calls -- ST7735_ClearScreen
 *      wipes the frame too, and the HUD paint funcs assume the frame is
 *      already there (so they don't paint over it).
 *   2. board_init zeros cells + bookkeeping + sets height_peak sentinel.
 *   3. Zero score / hard-drop / anim. anim = {0} sets phase = 0 so the
 *      anim short-circuit doesn't fire on the next iteration.
 *   4. HUD paints, in order: labels ("SCR" / "MAX"), current score (0),
 *      max score (whatever the player has on this chip). Labels and max
 *      score are static for the lifetime of one game -- they're painted
 *      here once and never touched until the next reset.
 *   5. promote_and_queue: roll a kind, queue another, paint active + preview.
 *      Can't game-over here (board just got cleared), so we ignore the
 *      return value with (void).
 *   6. Reset prev_ms so gravity doesn't fire immediately on the next loop.
 *
 * `max_score` is read-only here: the caller (main) owns the RAM copy and
 * updates it on game-over before calling reset. We pass by value, not
 * pointer, to make that contract clear at the call site.
 *
 * The RNG state intentionally is NOT re-seeded: each spawn has already
 * advanced the xorshift8 stream, so we get fresh-looking pieces from
 * wherever the stream happened to be when the game ended. Re-seeding from
 * ADC would also be fine but is unnecessary churn.
 * --------------------------------------------------------------------------*/
/* ---- game-over entry helper --------------------------------------------- *
 * Two call sites (after-clear and after-lock-no-clear) need the exact same
 * sequence: maybe promote new max -> paint overlay -> paint cursor. Folded
 * here so the two sites can't diverge.
 *
 * Max-score update sequence (only when score beat the previous max):
 *   1. Bump RAM copy first -- if a power glitch interrupts the EEPROM
 *      write below, the RAM copy still reflects the in-game state for
 *      the (vanishingly unlikely) case the player keeps playing. The
 *      next clean game-over will retry the EEPROM write.
 *   2. eeprom_update_word: writes byte-by-byte, skipping unchanged bytes.
 *      A first-ever max takes ~7 ms (2 bytes * ~3.3 ms each); a same-value
 *      "write" is essentially free. The CPU is halted by the EEPROM
 *      hardware during the write, so timing-sensitive code is gated
 *      anyway -- and game-over freezes gameplay, so the halt is invisible.
 *   3. render_hud_max_score: repaints the MAX digits while the game-over
 *      overlay is up. The overlay covers the playfield, not the left
 *      margin, so the new max paints in real time -- the player sees
 *      their new record celebrated.
 *
 * Order matters: paint the new MAX before the overlay so the overlay's
 * panel doesn't briefly mask the MAX repaint (they don't overlap, but
 * keeping a strict left-to-right order makes the visual chain easy to
 * reason about).                                                          */
static void enter_game_over(struct st7735 *lcd, uint16_t score,
                            uint16_t *max_score, uint8_t *game_over,
                            uint8_t *go_selection) {
    if (score > *max_score) {
        *max_score = score;
        eeprom_update_word(&ee_max_score, *max_score);
        render_hud_max_score(lcd, *max_score);
    }
    *game_over    = 1;
    *go_selection = GAME_OVER_SEL_YES;
    render_game_over_overlay(lcd);
    render_game_over_selection(lcd, *go_selection);
}

static void reset_game(struct st7735 *lcd, Board *board, Shape *active,
                       uint8_t *color_idx, uint8_t *next_kind,
                       uint16_t *score, AnimState *anim,
                       uint8_t *in_hard_drop, uint16_t *prev_ms,
                       uint16_t max_score) {
    ST7735_ClearScreen(lcd, BLACK);
    draw_board_frame(lcd);

    board_init(board);
    *score        = 0;
    *in_hard_drop = 0;
    *anim         = (AnimState){0};

    render_hud_labels(lcd);
    render_hud_score(lcd, *score);
    render_hud_max_score(lcd, max_score);

    *next_kind = roll_kind();
    (void)promote_and_queue(lcd, active, color_idx, next_kind, board);

    *prev_ms = timer_now_ms();
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
    /* Frame is part of the gameplay screen; we draw it AFTER the start
     * overlay is dismissed (reset_game redraws it). The start screen sits
     * on a black background with no frame -- a cleaner splash look. */

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
    uint16_t  score = 0;            /* running total of lines cleared     */
    uint8_t   next_kind = 0;        /* queued piece kind; rolled on entry
                                       into gameplay, promoted on each spawn */
    uint8_t   game_over    = 0;     /* 1 when GAME OVER overlay is showing */
    uint8_t   go_selection = GAME_OVER_SEL_YES;   /* cursor on the overlay */

    /* Persistent max score. Read once at boot. 0xFFFF is the value an
     * un-touched EEPROM cell returns on a freshly-flashed chip; treat
     * that as "no max yet" -> display 000. Any value below the cap is
     * trusted as-is. */
    uint16_t  max_score = eeprom_read_word(&ee_max_score);
    if (max_score == 0xFFFFu) max_score = 0;
    if (max_score > SCORE_MAX) max_score = SCORE_MAX;

    board_init(&board);

    /* ---- start screen -------------------------------------------------- *
     * Paint the splash + [PLAY] panel and spin until the player hits DOWN.
     * No gameplay state has been spawned yet; the board is initialised but
     * not painted. Buttons are already armed (buttons_init() above) so
     * the debounced edge-detector works here too.
     *
     * Why a spin loop instead of folding the start branch into the main
     * while(1)? Two reasons:
     *   1. Symmetry: gameplay assumes promote_and_queue has been called
     *      so `active` is a valid Shape. Letting the main loop see an
     *      un-initialised `active` would need extra guards everywhere.
     *   2. Cost: gameplay's loop spends most of its time in the gravity
     *      timer check; on the start screen there's nothing else to do,
     *      so a plain spin is simplest and uses zero ROM. */
    render_start_overlay(&lcd);
    while (!button_down_just_pressed()) {
        /* Drain other button edges so they don't carry into the game.
         * `*_just_pressed()` returns the latched press flag and clears
         * it, so calling them here pre-clears any spurious press the
         * player might have done during boot. */
        (void)button_left_just_pressed();
        (void)button_right_just_pressed();
        (void)button_rotate_cw_just_pressed();
        (void)button_rotate_ccw_just_pressed();
    }

    /* DOWN was just consumed by the loop condition. Flip into gameplay
     * via reset_game (which clears the start overlay, paints the frame,
     * HUD, and spawns the first piece). */
    uint16_t prev_ms;
    reset_game(&lcd, &board, &active, &active_color_idx, &next_kind,
               &score, &anim, &in_hard_drop, &prev_ms, max_score);

    while (1) {
        uint16_t now = timer_now_ms();

        /* ---- game-over short-circuit ----------------------------------- *
         * Sits above the anim short-circuit so that even if a clear was
         * mid-animation when the game ended (it can't be -- game-over only
         * fires on spawn AFTER the anim finishes -- but defensively), we'd
         * still freeze. While `game_over` is set, gameplay (gravity, anim,
         * movement inputs) is fully gated; only the cursor + commit buttons
         * are live.
         *
         * Button semantics in game-over mode:
         *   LEFT  -> cursor = YES (no-op if already there)
         *   RIGHT -> cursor = NO
         *   DOWN  -> commit:
         *             YES = reset_game()  (full re-init)
         *             NO  = nothing for now -- placeholder for future
         *                   power-off behaviour. We still consume the edge
         *                   so the gameplay loop doesn't see it if the
         *                   player navigates back to YES later.
         *
         * Each press only repaints line 3 of the overlay (cheap), not the
         * whole panel. Mutual exclusion with the gameplay input block is
         * automatic -- the `continue` below skips the gameplay block.   */
        if (game_over) {
            if (button_left_just_pressed() &&
                go_selection != GAME_OVER_SEL_YES) {
                go_selection = GAME_OVER_SEL_YES;
                render_game_over_selection(&lcd, go_selection);
            }
            if (button_right_just_pressed() &&
                go_selection != GAME_OVER_SEL_NO) {
                go_selection = GAME_OVER_SEL_NO;
                render_game_over_selection(&lcd, go_selection);
            }
            if (button_down_just_pressed()) {
                if (go_selection == GAME_OVER_SEL_YES) {
                    game_over    = 0;
                    go_selection = GAME_OVER_SEL_YES;
                    reset_game(&lcd, &board, &active, &active_color_idx,
                               &next_kind, &score, &anim, &in_hard_drop,
                               &prev_ms, max_score);
                } else {
                    /* NO + DOWN -- "switch off" simulation. Paint the
                     * whole screen WHITE then break out of the main loop.
                     * Falling out of main() lands in avr-libc's _exit,
                     * which does `cli; rjmp .` -- interrupts off, chip
                     * halted, no further response. The LCD controller
                     * keeps its own framebuffer so the WHITE stays on
                     * screen indefinitely. Only a hardware RESET (or
                     * power cycle) brings the chip back.
                     *
                     * Note: this is "halted" not "asleep" -- chip still
                     * draws ~10 mA. For a USB-powered hobby setup that's
                     * fine; swap `break` for `sleep_cpu()` with
                     * SLEEP_MODE_PWR_DOWN if true low-power is ever
                     * needed. */
                    ST7735_ClearScreen(&lcd, WHITE);
                    break;
                }
            }
            continue;
        }

        /* ---- line-clear animation short-circuit ------------------------ *
         * While a clear is animating, gravity and inputs are frozen. The
         * tick advances state on the timer and, if the animation just
         * finished, spawns the next piece + resets the gravity clock so
         * the new piece doesn't insta-drop on the very next iteration.
         * Mirrors the C++ pattern of gating spawn / gravity on
         * !board.is_lines_formed(). */
        if (anim.phase != 0) {
            /* Snapshot total_lines before the tick. anim_tick calls
             * board_reset_lines() on the final phase-2 tick, which zeros
             * total_lines -- so reading after the tick would always give 0
             * when the animation just finished. Mirrors the C++ pattern
             * of grabbing get_total_lines() inside the "is_lines_formed()"
             * block before reset_lines_deltas() runs. */
            uint8_t cleared = board.total_lines;
            anim_tick(&lcd, &board, &anim, now);
            if (anim.phase == 0) {
                /* Animation done: apply scoring rule from
                 * testing_main.cpp:418-420 (score += total_lines), then
                 * saturate at SCORE_MAX so the 3-digit HUD stays in range. */
                uint16_t new_score = (uint16_t)(score + cleared);
                if (new_score > SCORE_MAX || new_score < score) {
                    new_score = SCORE_MAX;        /* overflow guard too */
                }
                if (new_score != score) {
                    score = new_score;
                    render_hud_score(&lcd, score);
                }
                if (promote_and_queue(&lcd, &active, &active_color_idx,
                                      &next_kind, &board)) {
                    /* Spawn collided with the stack -- step 7 trigger. */
                    enter_game_over(&lcd, score, &max_score, &game_over,
                                    &go_selection);
                }
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
                    if (promote_and_queue(&lcd, &active, &active_color_idx,
                                          &next_kind, &board)) {
                        /* Spawn collided with the stack -- step 7 trigger. */
                        enter_game_over(&lcd, score, &max_score, &game_over,
                                        &go_selection);
                    }
                }
            }
        }
    }
    return 0;
}
