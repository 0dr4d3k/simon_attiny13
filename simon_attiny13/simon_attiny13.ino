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

void sleepNow() {
  PORTB = 0b00000000; // disable all pull-up resistors
  cli(); // disable all interrupts
  WDTCR = 0; // turn off the Watchdog timer
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}

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
/*
  void gameOver() {
  for (uint8_t i = 0; i < 4; i++) {
    play(3 - i, 25000);
  }
  if (lvl > maxLvl) {
    eeprom_write_byte((uint8_t*) 0, ~lvl); // write best score
    eeprom_write_byte((uint8_t*) 1, (seed)); // write seed
    for (uint8_t i = 0; i < 3; i++) { // play best score melody
      levelUp();
    }
  }
  //  sleepNow();
  }

  void levelUp() {
  for (uint8_t i = 0; i < 4; i++) {
    play(i, 25000);
  }
  }
*/

uint8_t simple_random4() {
  /*
    // https://en.wikipedia.org/wiki/Linear_congruential_generator
    // ctx = ctx * 1103515245 + 12345; // too big for ATtiny13
    ctx = 2053 * ctx + 13849;
    uint8_t temp = ctx ^ (ctx >> 8); // XOR two bytes
    temp ^= (temp >> 4); // XOR two nibbles
    return (temp ^ (temp >> 2)) & 0b00000011; // XOR two pairs of bits and return remainder after division by 4
  */
  // use Linear-feedback shift register instead of Linear congruential generator
  // https://en.wikipedia.org/wiki/Linear_feedback_shift_register
  for (uint8_t i = 0; i < 2; i++) { // we need two random bits
    uint8_t lsb = ctx & 1; // Get LSB (i.e., the output bit)
    ctx >>= 1; // Shift register
    if (lsb || !ctx) { // output bit is 1 or ctx = 0
      ctx ^= 0xB400; // apply toggle mask
    }
  }
  return ctx & 0b00000011; // remainder after division by 4

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

void resetCtx() {
  ctx = (uint16_t)seed;
}

//void(* resetFunc) (void) = 0;//declare reset function at address 0

int main(void) {
  // ports
  PORTB = 0b00001111; // enable pull-up resistors on 4 game buttons

  TCCR0B = (1 << CS00); // Timer0 in normal mode (no prescaler)

  // start T0 to get aleatory seed in the first push button
  WDTCR =   (1 << WDTIE) | (1 << WDP1); // start watchdog timer with 64ms prescaller (interrupt mode)
  TIMSK0 |= 1 << TOIE0; // enable timer overflow interrupt

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

  // initialization sequence
  switch (PINB & 0b00001111) {
    //  switch (PINB) {

    case 0b00000111: // yellow button
      eeprom_write_byte((uint8_t*) 0, 255); // reset best score
      maxLvl = 0;
      break;
    case 0b00001011: // blue button
      lvl = 255; // play random tones in an infinite loop
      break;
    case 0b00001101: // green button
      lvl = maxLvl; // start from max level and load seed from eeprom (no break here)
    case 0b00001110: // red button
      seed = (((uint8_t) eeprom_read_byte((uint8_t*) 1)));
      break;
  }

  // wait to button release
  while ((PINB & 0b00001111) != 0b00001111) {};

  while (1) { // main loop
    //    for (;;) {
    resetCtx();
    for (uint8_t cnt = 0; cnt <= lvl; cnt++) { // never ends if lvl == 255
      _delay_loop_2(4400 + 489088 / (8 + lvl));
      play(simple_random4());
    }
    time = 0;
    lastKey = 5;
    resetCtx();
    for (uint8_t cnt = 0; cnt <= lvl; cnt++) {
      bool pressed = false;
      while (!pressed) {
        for (uint8_t i = 0; i < 4; i++) {
          if (!(PINB & buttons[i] & 0b00001111)) {
            //         if (!(PINB & buttons[i])) {
            if (time > 1 || i != lastKey) {
              play(i);
              pressed = true;
              uint8_t correct = simple_random4();
              if (i != correct) {
                leds(lOk | correct);
                _delay_loop_2((uint16_t)0xFFFF);
                //                gameOver();
                //                leds((uint8_t)lDn|0x01);
                leds(lDn | 1);
                if (lvl > maxLvl) {
                  eeprom_write_byte((uint8_t*) 0, ~lvl); // write best score
                  eeprom_write_byte((uint8_t*) 1, (seed)); // write seed
                  //                  leds((int8_t)lUp|0x03);
                  leds(lUp | 7);
                }
                sleepNow();

              }
              time = 0;
              lastKey = i;
              break;
            }
            time = 0;
          }

        }

        // thinking time 16s (250)
        // thinking time 4s (62)
        if (time > 62) {
          //~          sleepNow();
          //          gameOver();
          //          leds((uint8_t)lDn+1);
          leds(lDn | 1);
          sleepNow();
        }
      }
    }
    _delay_loop_2((uint16_t)0xFFFF);
    if (lvl < pericia) {
      lvl++;
      //      levelUp(); // animation for completed level
      leds(lUp | 3);
      _delay_loop_2((uint16_t)45000);
    }
    else { // special animation for highest allowable pericia level
      //      levelUp();
      //      gameOver(); // then turn off
      lvl = combo;
    }
  }
}
