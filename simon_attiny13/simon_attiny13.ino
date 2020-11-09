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

#define pericia 0x1F // 32 levels
#define combo   0xFF // combo

#define e0 0x42
#define e1 0x13

#define BT(t) (uint8_t)((t*1000)/16)

const uint8_t buttons[4] = {
  0b00010001,
  0b00010010,
  0b00010100,
  0b00011000
  //  0b00010110,
  //  0b00010011,
  //  0b00010101,
  //  0b00011010
};

const uint8_t bTones[8] = {
  7, 4, 2, 0, 1, 3, 5, 6
};

const uint8_t tones[8] = {
  119 / 2, // 0 -> 3
  133 / 2, // 1
  141 / 2, // 2 -> 2
  158 / 2, // 3
  178 / 2, // 4 -> 1
  200 / 2, // 5
  212 / 2, // 6
  238 / 2  // 7 -> 0
};

//uint8_t lastK;
uint8_t lvl = 0;
//uint8_t cnt;
uint8_t maxLvl;
uint16_t ctx;
uint8_t seed;
volatile uint8_t time;


/* no used
  void sleepNow() {
  PORTB = 0b00000000;                            // disable all pull-up resistors
  cli();                                         // disable all interrupts
  WDTCR = 0;                                     // turn off the Watchdog timer
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  }
*/

void play(uint8_t i, uint8_t t = BT(0.176))   {  // play tones
  PORTB = 0b00000000;                            // disable pull-up
  DDRB = buttons[i % 4];                         // set speaker and #i button pin as output
  OCR0A = tones[bTones[i]];
  OCR0B = tones[bTones[i]] >> 1;
  TCCR0B = (1 << WGM02) | (1 << CS01);           // prescaler /8
  delay_wdt(t);
  TCCR0B = 0b00000000;                           // no clock source (Timer0 stopped)
  DDRB = 0b00000000;
  PORTB = 0b00001111;
}


uint8_t simple_random4()                      {  // linear-feedback shift register
  for (uint8_t i = 0; i < 2; i++)             {
    uint8_t lsb = ctx & 1;
    ctx >>= 1;
    if (lsb || !ctx) ctx ^= 0b1011010000000000;
  }
  return ctx & 0b00000011;
}


uint8_t s[8] = {0, 0, 0, 0, 0, 0, 0, 0};          // 32 levels
uint8_t p2p_sequence(uint8_t i_key, uint8_t c) {  // player vs player
  uint8_t p_byte = (c >> 2);
  uint8_t p_key  = (c % 4) << 1;
  uint8_t d_key, o_key;

  if (c == lvl)                                {  // save key
    d_key = (i_key & 0b00000011) << p_key;
    s[p_byte] |= d_key;
    return i_key;
  }
  else if (c < lvl)                            {  // get key
    d_key = s[p_byte];
    o_key = d_key >> p_key;
    return o_key & 0b00000011;
  }
};


ISR(TIM0_OVF_vect)                             {  // Timer 0 INT - Speaker
  PORTB ^= 1 << PB4;
}


ISR(WDT_vect)                                  {  // Watchdog INT - BaseTime
  time++;                                         // increase each 16ms
  if (  TCCR0B & (1 << CS00))                  {  // seed generator form T0
    seed = (seed << 1) ^ TCNT0;
  }
}


void delay_wdt(uint8_t t)                      {  // Delay using 16ms Base Time
  time = 0;
  while (time <= t);                              // max delay 255 * 16ms = 4,08s
}


void resetCtx()                                {  // Seed expansion 0 padding
  ctx = (uint16_t)seed;
}


void resetNow()                                {  // WatchDog Autoreset
  cli();
  WDTCR = (1 << WDCE);
  WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);

  //  wdt_enable( WDTO_4S );
  while (1);
}

/* no used
  static inline void wdt_disable1 (void)         {  // WatchDod Disable - optimized
  uint8_t register temp_reg;
  asm volatile (
    "wdr"                        "\n\t"
    "in  %[TEMPREG],%[WDTREG]"   "\n\t"
    "ori %[TEMPREG],%[WDCE_WDE]" "\n\t"
    "out %[WDTREG],%[TEMPREG]"   "\n\t"
    "out %[WDTREG],__zero_reg__" "\n\t"
    : [TEMPREG] "=d" (temp_reg)
    : [WDTREG]  "I"  (_SFR_IO_ADDR(WDTCR)),
    [WDCE_WDE]  "n"  ((uint8_t)((1<<WDCE) | (1<<WDE)))
    : "r0"
  );
  }
*/

static inline void wdt_disable2 (void)             {  // WatchDod Disable - optimized
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
  bool p2p = false;

  { // watchdog timeout reset
    MCUSR &= ~(1 << WDRF);
    wdt_disable2(); // wdt_disable()
  }

  PORTB = 0b00001111;                               // Enable pull-up in buttons

  { // Seed Shuffle
    TCCR0B = (1 << CS00);                           // timer0 in normal mode
    WDTCR  = (1 << WDTIE);                          // start base time 16ms
    sei();                                          // global interrupt enable
    delay_wdt((uint8_t)8);                          // repeat for fist 8 WDT
  }

  // set Timer0 in PWM Pase Correct: WGM02:00 = 0b001
  //                   prescaler /8: CS02:00  = 0b010
  //       pin B output disconneted: COM0B1:0 = 0b00
  { // Timer 0 - Tones
    TCCR0B = (1 << CS01);                           // PWM Phase Correct 1/8
    TCCR0A = (1 << WGM00);                          // pin disconnected
    TIMSK0 = (1 << TOIE0);                          // timer overflow interrupt
  }

  maxLvl  = ~eeprom_read_byte((uint8_t*) 0);        // Best Score

  switch (PINB & 0b00001111)                     {  // Secret Codes
    case 0b00000111:                                // yellow button ---- reset score
      eeprom_write_byte((uint8_t*) 0, 255);
      maxLvl = 0;
      break;
    case 0b00001011:                                // blue button ------ p2p game
      p2p = true;
      break;
    case 0b00001010:                                // blue&red button -- future
      break;
    case 0b00001101:                                // green button ----- extra coin
      lvl = maxLvl;
    case 0b00001110:                                // red button ------- repeat
      seed = (((uint8_t) eeprom_read_byte((uint8_t*) 1)));
      break;
  }

  while ((PINB & 0b00001111) != 0b00001111) {};     // Wait to button release


  if (p2p)                                      {   // P2P led animation
    delay_wdt(BT(0.256));
    ledWin();
    ledLoss();
  }

  while (1)                                     {   // Main loop
    uint8_t presK;
    uint8_t lastK;
    uint8_t cnt;
    resetCtx();

    if (!p2p)                                   {
      uint8_t d = 14 - ((lvl & 0x1F) >> 2);
      for (cnt = 0; cnt <= lvl; cnt++)          { // play new sequence
        // never ends if lvl == 255
        delay_wdt(d);
        play(simple_random4(), d);
      }
    }

    time = 0;
    lastK = 5;
    resetCtx();
    for (cnt = 0; cnt <= lvl; cnt++)             {  // player sequence
      bool next = false;
      while (!next)                              {  // player iteraction
        for (presK = 0; presK < 4; presK++)      {  // polling buttons
          if (!(PINB & buttons[presK] & 0x0F))   {
            if (time > 1 || presK != lastK)      {  // key validation
              play(presK);
              next = true;
              uint8_t correct;
              if (p2p)
                correct = p2p_sequence(presK, cnt);
              else
                correct = simple_random4();

              if (presK != correct)              {  // you loss!
                for (uint8_t i = 0; i < 3; i++)  {
                  delay_wdt(BT(0.048));
                  play(correct, BT(0.096));
                }
                delay_wdt(BT(0.256));
                gameOver();
              }
              else                               {  // you win!
                time = 0;
                lastK = presK;
                break;
              }
            }
            time = 0;
          }
        }
        if (time >= (uint8_t)250 )               {  // timeout, you loss!
          gameOver();
        }
      }
    }

    delay_wdt(BT(0.256));
    ledWin();

    if ((s[0] == e0) & (s[1] == e1))             {  // level update - easer
      delay_wdt(BT(0.256));
      for (uint8_t i = 0; i < 32; i++)           {
        play(i%8,BT(0.056));
        delay_wdt(BT(0.08));
      }
      delay_wdt(BT(0.256));
      easer_egg();
    }
    else if (lvl < pericia)                      {  // level update - increase
      lvl++;
      delay_wdt(BT(0.176));
    }
    else                                         {  // levelupdate - max level
      lvl = combo;
    }
  }
}

void gameOver()                                  {  // Game Over sequence
  ledLoss();
  if (lvl > maxLvl) {
    saveLevel();
    ledScore();
  }
  resetNow();
}


void ledLoss()                                   {  // Loss chase
  for (uint8_t i = 0; i < 4; i++)
    play(3 - i, BT(0.112));
}


void ledWin()                                    {  // Win chase
  for (uint8_t i = 0; i < 4; i++)
    play(i, BT(0.112));
}


void ledScore()                                  {  // Best score chase
  for (uint8_t i = 0; i < 3; i++)
    ledWin();
}


void saveLevel()                                 {  // Save max level and seed
  eeprom_write_byte((uint8_t*) 0, ~lvl);
  eeprom_write_byte((uint8_t*) 1, (seed));
}


void easer_egg()
{
  while (1)                    {  // Lets dance
    jazz(BT(0.144));
    delay_wdt(BT(0.144));
    if (simple_random4() != 2) {
      jazz(BT(0.096));
    }
    delay_wdt(BT(0.096));
  }
}

void jazz(uint8_t t) {
  uint8_t switchval;
  uint8_t note = 0;

  switchval = simple_random4();
  //switchval = simple_random8() % 5;
  switch (switchval) {
    case 0:  note += note;     break;
    case 1:  note += note + 1; break;
    case 2:  note += note - 1; break;
    case 3:  note += note + 2; break;
    //    case 4:  note = note - 2; break;
    default: note = 0;        break;
  }
  play(note % 8, t);
};





