
#include "main.h" 

// -------- Global Variables --------- //

// -------- Functions --------- //

// Hello world here
//

void SPI_MasterInit(void) {
  // setting the AVR to master
  /* Set MOSI and SCK output, all others input */
  DDRB = (1<<DDB5)|(1<<DDB7);
  /* Enable SPI, Master, set clock rate fck/16 */
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void SPI_MasterTransmit(char cData){
  // doing the data transmission from the master (avr) to the display (any input)
  /* Start transmission */
  SPDR = cData;
  /* Wait for transmission complete */
  while(!(SPSR & (1<<SPIF))){
    
  };
}

int main(void) {
  // -------- Inits --------- //
  
  // clock_prescale_set(clock_div_1);                 /* CPU Clock: 8 MHz */
  // initUSART();
  // printString("OK");
  // put SS bit as output
  DDRB |= (1<<DDB4);
  DDRD = DDRD | (1<<DDD5) | (1<<DDD6);
  SPI_MasterInit();
  // ------ Event loop ------ //
  while (1) {



  }                                                  /* End event loop */
  return (0);                            /* This line is never reached */
}
