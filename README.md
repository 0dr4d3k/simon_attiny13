# Simon Says Game in ATTiny13 MCU

## Description
This project is based in the e-radionica solder kit [1], [2] and the fantastic hackaday.io Simon Says project [3] by Vodja [4] with some modifications.
The aim of this project is to create a simple but still playable toy which can be used to learn basic skills in electronics.

## Characteristics
- Simon Says game
- With original sound and original timeout
- Less than 1kB code
- Power ON/OFF switch
- Sound ON/OFF switch (or not kill me neighbour's mode :-)
- Persistent best score and best sequence
- Secret mode: try it aganin
- Secret mode: restart same sequence
- Secret mode: clear best score
- Secret mode: two players mode - original emulation
- Loss, Win, Best Score, Final, Easer LED animations
- Easer Egg - Random Jazz Machine (to dance with your friends)
- Easer Egg = Cycle warning light (stay safety :-)
- Power ON forgot alarm
-

## Details
The heart of the game is ATtiny13. It is low power AVR 8-bit microcontroller with 1 kB flash memory, 64 bytes EEPROM and 64 bytes SRAM.

It has six I/O pins, from which one acts as a reset not used in this project, four are connected to LEDs and buttons and the last one leads to the magnetodynamic buzzer. The device is powered from CR2032 lithium cell or compatible.

As mentioned, the device contains four LEDs of different colors (red, orange, yellow and green) connected with four button switches. (Blue or 525 nm green LEDs can't be used because they have too high forward voltage and wouldn't work well with partially discharged battery.)

After pressing each of those buttons, the LED lights up and information about the button state can be read by a microcontroller. The pins of a microcontroller are bi-directional, so they can be changed from input to output in any time and can control LEDs instead of the buttons (see simplified diagram).

To avoid to making short-circuit on output pins, changing between input and output can be done only in low state (disabled internal pull-up resistor). Because each LED has different efficiency, resistors of different values must be connected in series with them.

The high-impedance (42 ohm) buzzer is connected in series with a capacitor (to eliminate DC voltage) to the output of internal 8-bit timer/counter. Like in an original game, each color is producing a particular tone when it is pressed or activated by the microcontroller and the frequencies are in an exact ratio 3:4:5:6 (50% duty cycle). 

Sufficient pseudo-random sequences are generated by a linear congruential generator defined by the recurrence relation:

x[i+1] = (2053 * x[i] + 13849) mod 216

Eight bit pairs of 16-bit number x[i] are XORed together to produce a number 0 to 3. Quality of this generator can be checked displaying the whole 65536 elements long sequence (it is good enough for this purpose).

The value of random seed x[0] is generated from combination of non-connected input of A/D converter and jitter between watchdog timer and calibrated RC oscillator. So the probability of repeating the same sequence is very low.

The watchdog timer is used also for buttons debouncing and for auto power-off after a minute of inactivity. In power-down mode (also after the game is over) the consumption is approximately 0.15 microamps.

Best score is saved to the EEPROM (including the seed), so it is possible to re-play the best game (even after removing the battery).

The concept of this electronic set is to make it easy to build without any special tools or skills. The single-sided printed circuit board is optimized to be easily drawn by hand with a permanent marker and etched in a solution of ferric chloride (tested at summer camps and hobby groups for children). 

The game can run in five modes:

    When the game is started only using the middle (reset) button, random seed is generated and you have to memorize and repeat increasing sequence of colors and tones. In higher levels the game goes faster. If you make a mistake, the correct LED blinks three times and game-over animation is played. If you have beaten the best score, your game (achieved level and seed value) is saved to the EEPROM and best-score animation is played.
    If you hold orange button during reset, you can continue with the best-scored game. It starts on the highest achieved level.
    When you hold yellow button during reset, you can play the best-scored game from the beginning.
    If the red button is pressed during reset, the best score is erased and you start a new random game as in the first case.
    Pressing the green button during reset starts playing the pseudo-random sequence (starting by a random seed). It takes almost 3.5 hours before the sequence starts repeating.

## Modify the hardware:
<p align="center">
  <img src="assets/hardware_hack.jpg" width="350" title="Tiny13 Simon Hardware Modification">
</p>

## Secret codes

## Video
[![Watch the video](https://player.vimeo.com/video/477245412)](https://player.vimeo.com/video/477245412)

## Resistor 

## Consuption

## Compiling

## Reference Libraies:
 - [MicroCore]
 - [picoCore]





1. First ordered list item
2. Another item
⋅⋅* Unordered sub-list. 
1. Actual numbers don't matter, just that it's a number
⋅⋅1. Ordered sub-list
4. And another item.

⋅⋅⋅You can have properly indented paragraphs within list items. Notice the blank line above, and the leading spaces (at least one, but we'll use three here to also align the raw Markdown).

⋅⋅⋅To have a line break without a paragraph, you will need to use two trailing spaces.⋅⋅
⋅⋅⋅Note that this line is separate, but within the same paragraph.⋅⋅
⋅⋅⋅(This is contrary to the typical GFM line break behaviour, where trailing spaces are not required.)

* Unordered list can use asterisks
- Or minuses
+ Or pluses



[I'm an inline-style link](https://www.google.com)

[I'm an inline-style link with title](https://www.google.com "Google's Homepage")

[I'm a reference-style link][Arbitrary case-insensitive reference text]

[I'm a relative reference to a repository file](../blob/master/LICENSE)

[You can use numbers for reference-style link definitions][1]

Or leave it empty and use the [link text itself].

URLs and URLs in angle brackets will automatically get turned into links. 
http://www.example.com or <http://www.example.com> and sometimes 
example.com (but not on Github, for example).

Some text to show that the reference links can follow later.

[arbitrary case-insensitive reference text]: https://www.mozilla.org
[link text itself]: http://www.reddit.com


 
 
[1]: https://www.tindie.com/products/e-radionica/simon-says-diy-learn-to-solder-kit/ "buy on tindie" 
[2]: https://e-radionica.com/en/simon-says-kit.html "buy on e-radionica"
[3]: https://hackaday.io/project/18952-simon-game-with-attiny13 "reference project"
[4]: https://hackaday.io/vojtak "hackaday simon attiny13, father"

[MicroCore]: https://github.com/MCUdude/MicroCore
[picoCore]: https://github.com/nerdralph/picoCore
