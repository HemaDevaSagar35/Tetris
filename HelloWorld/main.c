
#include "main.h" 
#include "st7735.h"

// -------- Global Variables --------- //

// -------- Functions --------- //

// Hello world here
//

// void SPI_MasterInit(void) {
//   // setting the AVR to master
//   /* Set MOSI and SCK output, all others input */
//   DDRB = (1<<DDB5)|(1<<DDB7);
//   /* Enable SPI, Master, set clock rate fck/16 */
//   SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
// }

// void SPI_MasterTransmit(char cData){
//   // doing the data transmission from the master (avr) to the display (any input)
//   /* Start transmission */
//   SPDR = cData;
//   /* Wait for transmission complete */
//   while(!(SPSR & (1<<SPIF))){
    
//   };
// }

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
  ST7735_SetPosition(10, 10);
  ST7735_DrawString(&lcd, "Hello World!", WHITE, X1);

  while (1) { }
  return 0;
 
}
