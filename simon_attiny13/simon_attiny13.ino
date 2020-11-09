/*
   Copyright (c) 2016 Divadlo fyziky UDiF (www.udif.cz)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
*/

#include <avr/sleep.h>
#include <util/delay_basic.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#define pericia 4
#define combo 255


const uint8_t buttons[4] = {
  0b00010001, 0b00010010, 0b00010100, 0b00011000
};

const uint8_t tones[4] = {
  239 / 2, 179 / 2, 143 / 2, 119 / 2
};
uint8_t lastKey;
uint8_t lvl = 0;
uint8_t maxLvl;
uint16_t ctx;
uint8_t seed;
volatile uint8_t time;

#define DUMMY0 _SFR_IO8(0x2E) //#define DWDR _SFR_IO8(0x2E)

// bit accessible
//#define FLAGS _SFR_IO8(0x1D) //#define EEDR _SFR_IO8(0x1D)
#define FLAGS EEDR //#define EEDR _SFR_IO8(0x1D)
#define FLAG0 0
#define FLAG1 1
#define FLAG2 2
#define FLAG3 3
#define FLAG4 4
#define FLAG5 5
#define FLAG6 6
#define FLAG7 7




/*
  void sleepNow() {
  PORTB = 0b00000000; // disable all pull-up resistors
  cli(); // disable all interrupts
  WDTCR = 0; // turn off the Watchdog timer
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  }
*/

void play(uint8_t i, uint16_t t = 45000) {
  PORTB = 0b00000000;  // set all button pins low or disable pull-up resistors
  DDRB = buttons[i]; // set speaker and #i button pin as output
  OCR0A = tones[i];
  OCR0B = tones[i] >> 1;
  TCCR0B = (1 << WGM02) | (1 << CS01); // prescaler /8
  _delay_loop_2(t);
  TCCR0B = 0b00000000; // no clock source (Timer0 stopped)
  DDRB = 0b00000000;
  PORTB = 0b00001111;
}

/*
  #define lUp 0x10
  #define lDn 0x20
  #define lOk 0x30

  void leds(uint8_t ci)
  {
  uint8_t op1 = (ci & 0xF0);
  uint8_t op2 = (ci & 0x0F);

  //  for (uint8_t ii = (ci & 0x0F); ii == 0; ii--)
  {
    switch (op1) {
      case lUp:
        for (uint8_t i = op2; i == 0; i--)
        {
          for (uint8_t i = 0; i < 4; i++) {
            play(i, 25000);
          }
        }
        break;
      case lDn:
        for (uint8_t i = 0; i < 4; i++) {
          play(3 - i, 25000);
        }
        break;
      case lOk:
        for (uint8_t j = 0; j < 3; j++) {
          _delay_loop_2((uint16_t)10000);
          play(op2, 20000);
        }
        break;
    }
  }
  }
*/
/*
  void gameOver()
  {
  for (uint8_t i = 0; i < 4; i++)
    play(3 - i, 25000);

  if (lvl > maxLvl)
  {
    eeprom_write_byte((uint8_t*) 0, ~lvl); // write best score
    eeprom_write_byte((uint8_t*) 1, (seed)); // write seed
    for (uint8_t i = 0; i < 3; i++) { // play best score melody
      levelUp();
    }
  }
  sleepNow();
  }
*/


uint8_t simple_random4() {
  // use Linear-feedback shift register instead of Linear congruential generator
  // https://en.wikipedia.org/wiki/Linear_feedback_shift_register
  for (uint8_t i = 0; i < 2; i++)
  {
    uint8_t lsb = ctx & 1;
    ctx >>= 1;
    if (lsb || !ctx) ctx ^= 0b1011010000000000;
  }
  return ctx & 0b00000011;
}


ISR(TIM0_OVF_vect) {
  PORTB ^= 1 << PB4;
}

ISR(WDT_vect) {
  time++; // increase each 64 ms

  // random seed generation
  if (  TCCR0B & (1 << CS00)) // Timer0 in normal mode (no prescaler))
  {
    seed = (seed << 1) ^ TCNT0;
  }
}


//void resetCtx() {
//  ctx = (uint16_t)seed;
//}

//void(* resetFunc) (void) = 0;//declare reset function at address 0


void resetNow() {
  cli();
  WDTCR =  (1 << WDCE);
  WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);

  //  wdt_enable( WDTO_2S );
  while (1);
}


static inline void wdt_disable1 (void)
{
  uint8_t register temp_reg;
  asm volatile (
    "wdr"                        "\n\t"
    "in  %[TEMPREG],%[WDTREG]"   "\n\t"
    "ori %[TEMPREG],%[WDCE_WDE]" "\n\t"
    "out %[WDTREG],%[TEMPREG]"   "\n\t"
    "out %[WDTREG],__zero_reg__" "\n\t"
    : [TEMPREG] "=d" (temp_reg)
    : [WDTREG]  "I"  (_SFR_IO_ADDR(WDTCR)),
    [WDCE_WDE]  "n"  ((uint8_t)(_BV(WDCE) | _BV(WDE)))
    : "r0"
  );
}

static inline void wdt_disable2 (void)
{
  wdt_reset();
  WDTCR = (1 << WDCE) | (1 << WDE);
  asm volatile (
    "out %[WDTREG],__zero_reg__" "\n\t"
    :
    : [WDTREG]  "I"  (_SFR_IO_ADDR(WDTCR))
    :
  );
}



int main(void) {
  // watchdog reset
  MCUSR &= ~(1 << WDRF);
  wdt_disable2(); // wdt_disable() optimization

  // ports
  PORTB = 0b00001111; // enable pull-up resistors on 4 game buttons

  // flags
  FLAGS = 0b00000000;

  TCCR0B = (1 << CS00); // Timer0 in normal mode (no prescaler)

  // t0(seed generator and notes) and watchdog (basetime)
  WDTCR =   (1 << WDTIE) | (1 << WDP1); // start watchdog timer with 64ms prescaller (interrupt mode)
  TIMSK0 = 1 << TOIE0; // enable timer overflow interrupt

  sei(); // global interrupt enable
  // repeat for fist 8 WDT interrupts to shuffle the seed
  _delay_loop_2((uint16_t)0xFFFF);

  // set Timer0 in PWM Pase Correct: WGM02:00 = 0b101
  //                   prescaler /8: CS02:00  = 0b010
  //       pin B output disconneted: COM0B1:0 = 0b00
  TCCR0B = (0 << WGM02) | (1 << CS01);
  TCCR0A = (0 << COM0B1) | (0 << COM0B0) | (0 << WGM01)  | (1 << WGM00);

  // read best score and seed from eeprom
  maxLvl  = ~eeprom_read_byte((uint8_t*) 0);

  // secret codes
  switch (PINB & 0b00001111) {
    case 0b00000111:                                  // yellow button - reset score
      eeprom_write_byte((uint8_t*) 0, 255);
      maxLvl = 0;
      break;
    case 0b00001011:                                  // blue button - infinite loop
      lvl = combo;
      break;
    case 0b00001101:                                  // green button - extra coin
      lvl = maxLvl;
    case 0b00001110:                                  // red button - repeat
      seed = (((uint8_t) eeprom_read_byte((uint8_t*) 1)));
      break;
  }

  // wait to button release
  while ((PINB & 0b00001111) != 0b00001111) {};

  while (1) { // main loop
    ctx = seed;

    for (uint8_t cnt = 0; cnt <= lvl; cnt++) {      // play new sequence
      // never ends if lvl == 255
      _delay_loop_2(4400 + 489088 / (8 + lvl));
      play(simple_random4());
    }

    time = 0;
    lastKey = 5;
    ctx = seed;

    for (uint8_t cnt = 0; cnt <= lvl; cnt++)         // player sequence
    {
      bool next = false;
      while (!next) {                                // player iteraction
        for (uint8_t i = 0; i < 4; i++) {            // polling buttons
          if (!(PINB & buttons[i] & 0b00001111)) {
            if (time > 1 || i != lastKey) {          // key validation
              play(i);
              next = true;
              uint8_t correct = simple_random4();
              if (i != correct) {                    // you loss!
                for (uint8_t i = 0; i < 3; i++) {
                  _delay_loop_2(10000);
                  play(correct, 20000);
                }
                _delay_loop_2((uint16_t)0xFFFF);
                gameOver();
              }
              else {                                 // you win!
                time = 0;
                lastKey = i;
                break;
              }
            }
            time = 0;
          }
        }

        if (time > 64) {                             // timeout, you loss!
          //          FLAGS |= (1 << FLAG0);
          gameOver();
        }
      }
    }

    _delay_loop_2((uint16_t)0xFFFF);
    ledWin();

    if (lvl < pericia) {
      lvl++;
      _delay_loop_2((uint16_t)45000);
    }
    else
    {
      lvl = combo;
    }
  }
}

void gameOver() {
  ledLoss();
  if (lvl > maxLvl) {
    saveLevel();
    ledScore();
  }
  resetNow();
}


void ledLoss() {
  // game over chase
  for (uint8_t i = 0; i < 4; i++) play(3 - i, 25000);
}

void ledWin()
{
  for (uint8_t i = 0; i < 4; i++) play(i, 25000);
}

void ledScore() {
  // play best score melody
  for (uint8_t i = 0; i < 3; i++)
    ledWin();
}

void saveLevel() {
  // save max level and seed
  eeprom_write_byte((uint8_t*) 0, ~lvl);
  eeprom_write_byte((uint8_t*) 1, (seed));
}




