
#include "main.h" 
#include "st7735.h"
#include "t_shape.h"

// -------- Global Variables --------- //
#define BLOCK_PIXELS 10
/* Render a single tetromino as 4 filled rectangles. Lives here for now;
 * will move into tet_render/ once we add the Board too. */
static void render_shape(struct st7735 *lcd, const Shape *s, uint16_t color) {
    const Pixel *blocks = shape_get_blocks(s);
    for (uint8_t i = 0; i < SHAPE_BLOCKS; i++) {
        int8_t bx = blocks[i].x;
        int8_t by = blocks[i].y;
        uint8_t xs = (uint8_t)(bx * BLOCK_PIXELS);
        uint8_t ys = (uint8_t)(by * BLOCK_PIXELS);
        uint8_t xe = (uint8_t)(xs + BLOCK_PIXELS - 1);
        uint8_t ye = (uint8_t)(ys + BLOCK_PIXELS - 1);
        /* ST7735_DrawRectangle uses INCLUSIVE endpoints: (xe-xs+1)*(ye-ys+1) px */
        ST7735_DrawRectangle(lcd, xs, xe, ys, ye, color);
    }
}


int main(void) {
  // -------- Inits --------- //
  
  
  // DDRB |= (1<<DDB4);
  // DDRD = DDRD | (1<<DDD5) | (1<<DDD6);
  // SPI_MasterInit();

  struct signal cs = { .ddr = &DDRB, .port = &PORTB, .pin = 4 };  // SS on PB4
  struct signal bl = { .ddr = &DDRD, .port = &PORTD, .pin = 7 };  // this is wasting of pin. TODO: let's think of fixing it later
  struct signal dc = { .ddr = &DDRD, .port = &PORTD, .pin = 5 };  // this is rs on TFT
  struct signal rs = { .ddr = &DDRD, .port = &PORTD, .pin = 6 }; // this is rst on tft
  struct st7735 lcd = { .cs = &cs, .bl = &bl, .dc = &dc, .rs = &rs };

  ST7735_Init(&lcd);
  ST7735_ClearScreen(&lcd, BLACK);
  // ST7735_SetPosition(10, 10);
  // ST7735_DrawString(&lcd, "Hello World!", WHITE, X1);
  // ST7735_DrawRectangle (&lcd, 10, 40, 10, 90, WHITE);

  Shape t;
  t_shape_init(&t, 3, 3, 0);     /* x=3, y=3, rotation quadrant 0 (= 0deg) */
  render_shape(&lcd, &t, WHITE);

  while (1) { }
  return 0;
 
}
