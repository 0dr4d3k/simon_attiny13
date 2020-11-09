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

#define pericia 32
#define combo 255

#define SET(x,y) x |= (1 << y)
#define CLEAR(x,y) x &= ~(1<< y)
#define READ(x,y) ((0u == (x & (1<<y)))?0u:1u)
#define TOGGLE(x,y) (x ^= (1<<y))

const uint8_t buttons[4] = {
  0b00010001, 0b00010010, 0b00010100, 0b00011000
};

const uint8_t tones[4] = {
  239 / 2, 179 / 2, 143 / 2, 119 / 2
};
uint8_t lastK;
//uint8_t presK;
uint8_t lvl = 0;  // oJo, mirar el reseteo
uint8_t cnt;
uint8_t maxLvl;
uint16_t ctx;
uint8_t seed;
volatile uint8_t time;
bool p2p = false;
bool easer = false;


#define DUMMY0 _SFR_IO8(0x2E) //#define DWDR _SFR_IO8(0x2E)

// bit accessible
//#define FLAGS _SFR_IO8(0x1D) //#define EEDR _SFR_IO8(0x1D)
#define FLAGS EEDR // #define EEDR _SFR_IO8(0x1D)
//#define FLAGS DWDR
#define FLAG0 0    // P2P, Player to Player flag
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


uint8_t s[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // 32 levels

uint8_t p2p_sequence2(uint8_t i_key) {
  //  uint8_t p_byte = (cnt >> 2);
  //  uint8_t p_key  = (cnt % 4) << 1;
  uint8_t p_byte = (cnt / 4);
  uint8_t p_key  = (cnt % 4) * 2;
  uint8_t d_key, o_key;

  if (cnt == lvl) {  // save key
    d_key = (i_key & 0b00000011) << p_key;
    s[p_byte] |= d_key;
    return i_key;
  }
  else if (cnt < lvl) {  //get key
    d_key = s[p_byte];
    o_key = d_key >> p_key;
    return o_key & 0b00000011;
  }
  else {
  }
};

const uint8_t s_easer[2] = {0b01000010, 0b00010011};
uint8_t p2p_easer(uint8_t i_key) {
  //  uint8_t p_byte = (cnt >> 2);
  //  uint8_t p_key  = (cnt % 4) << 1;
  uint8_t p_byte = (cnt / 4);
  uint8_t p_key  = (cnt % 4) * 2;
  uint8_t d_key, o_key;



  d_key = s_easer[p_byte];
  o_key = d_key >> p_key;
  return o_key & 0b00000011;


};


uint8_t p2p_sequence() {
  uint8_t led_s = s[cnt >> 2];
  led_s >>= (cnt % 4);
  return led_s & 0b00000011;
};


void p2p_up(uint8_t key) {
  key <<= (cnt % 4);
  s[cnt >> 2] = key;
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
  // Seed expansion 0 padding
  ctx = (uint16_t)seed;
}

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

/*
  uint8_t eeprom_read_byte2 (const uint8_t *p)
  {
  uint8_t t = FLAGS;
  uint8_t b = eeprom_read_byte ((uint8_t *)p);
  FLAGS = t;
  return b;
  }

  void eeprom_write_byte2 (uint8_t *p, uint8_t value)
  {
  uint8_t t = FLAGS;
  eeprom_write_byte ((uint8_t *)p, value);
  FLAGS = t;
  }
*/


int main(void) {
  //lvl = 0x00;
  //cnt = 0x00;

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
    case 0b00001011:                                  // blue button - p2p game
      p2p = true;
      easer = true;

      //      FLAGS |= (1 << FLAG0);
      //      SET(FLAGS, FLAG0);

      //      lvl = combo;
      break;
    case 0b00001101:                                  // green button - extra coin
      lvl = maxLvl;
    case 0b00001110:                                  // red button - repeat
      seed = (((uint8_t) eeprom_read_byte((uint8_t*) 1)));
      break;
  }

  // wait to button release
  while ((PINB & 0b00001111) != 0b00001111) {};

  // p2p led animation
  if (p2p) {
    _delay_loop_2((uint16_t)45000);
    //    resetCtx();
    //    uint8_t led_init = simple_random4();
    //    p2p_up(led_init);
    //    play(led_init);
    ledWin();
    ledLoss();
  }
  //  if (READ(FLAGS,FLAG0) == 1u) {ledWin(); ledLoss();}


  while (1)                                     {   // main loop
    uint8_t presK;
    resetCtx();
    if (!p2p)
    {
      //    if (!READ(FLAGS,FLAG0))
      for (cnt = 0; cnt <= lvl; cnt++)  {   // play new sequence
        // never ends if lvl == 255
        _delay_loop_2((uint16_t)45000);
        //~        _delay_loop_2(4400 + 489088 / (8 + lvl));
        play(simple_random4());
      }
    }

    time = 0;
    lastK = 5;
    resetCtx();
    /*
        for (cnt = 0; cnt <= 6; cnt++)             {   // player sequence
          bool next = false;
          while (!next)                              {   // player iteraction
            for (presK = 0; presK < 4; presK++)      {   // polling buttons
              if (!(PINB & buttons[presK] & 0x0F))   {
                if (time > 1 || presK != lastK)      {   // key validation
                  play(presK);
                  next = true;
                  uint8_t correct;
                  if (p2p){
                    correct = p2p_easer(presK);
                  }

                  if (presK != correct)              {   // you loss!
                    easer = false;

                  }
                  else                               {   // you win!
                    time = 0;
                    lastK = presK;
                    break;
                  }
                }
                time = 0;
              }
            }
            if (time > 64)                           {   // timeout, you loss!
              gameOver();
            }
          }
          if (!easer)
            break;
        }
    */
    uint8_t lvl = easer ? 6 : lvl;
    for (cnt = 0; cnt <= lvl; cnt++)             {   // player sequence
      bool next = false;
      while (!next)                              {   // player iteraction
        for (presK = 0; presK < 4; presK++)      {   // polling buttons
          if (!(PINB & buttons[presK] & 0x0F))   {
            if (time > 1 || presK != lastK)      {   // key validation
              play(presK);
              next = true;
              uint8_t correct;
              if (p2p)
              {
                if (easer)
                  correct = p2p_easer(presK);
                else
                  correct = p2p_sequence2(presK);
              }
              else
                correct = simple_random4();

              if (presK != correct)              {   // you loss!

                if (easer)
                {
                  easer = false;
                  break;
                }
                else
                {
                  for (uint8_t i = 0; i < 3; i++)  {
                    _delay_loop_2(10000);
                    play(correct, 20000);
                  }
//~                  _delay_loop_2((uint16_t)0xFFFF);
                  gameOver();
                }
              }
              else                               {   // you win!
                time = 0;
                lastK = presK;
                break;
              }
            }
            time = 0;
          }
        }
        if (time > 64)                           {   // timeout, you loss!
          //          FLAGS |= (1 << FLAG0);
          if (easer)
            easer = false;
          else
            gameOver();
        }
      }

      if (!easer)
        break;
    }



//~    _delay_loop_2((uint16_t)0xFFFF);
    ledWin();




    if (lvl < pericia) {
      lvl++;
      //      p2p_up(presK);
      _delay_loop_2((uint16_t)45000);
    }
    else
      lvl = combo;

    if (easer)
      lvl = combo;
    


    /*
        resetCtx();
        //    if (!p2p)

        {
          //    if (!READ(FLAGS,FLAG0))
      //      for (cnt = 0; cnt <= lvl; cnt++)  {   // play new sequence
          for (cnt = 0; cnt < lvl; cnt++)  {   // play new sequence
            // never ends if lvl == 255
            _delay_loop_2((uint16_t)45000);
            //~        _delay_loop_2(4400 + 489088 / (8 + lvl));
            if (!p2p)
              play(simple_random4());
            else
              play(p2p_sequence2(0));
          }
        }
    */
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




