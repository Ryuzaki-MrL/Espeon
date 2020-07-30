![Espeon](/logo/espeon_dmg_small.png)

Espeon is a gameboy emulator for Espressif's ESP32 SoC.

This project started out as a [rewrite](https://github.com/Ryuzaki-MrL/m5stack-gameboy) in order to be more accurate and optimized for the ESP32.

## About Espeon

This emulator is being developed primarily with the M5Stack device in mind. You can find more info about it here: https://m5stack.com/

M5Stack Arduino API, M5Stack TreeView library, and M5Stack FACES are required.

If you don't own a M5Stack, feel free to adapt this codebase to your setup.

### Recommended setup:

* A board containg an ESP32 chip and at least 4MB (32Mbit) of SPI flash, plus the tools to program it
* A 320x240 ILI9341 display, controllable by a 4-wire SPI interface
* Some kind of I2C gamepad
* SD card reader (optional)

## Compiling Espeon

To compile for the M5Stack, you'll need to:

* [Set up your Arduino IDE for the M5Stack](https://docs.m5stack.com/#/en/arduino/arduino_development)
* (OPTIONAL) Get a copy of the gameboy's bootrom as a const array called gb_bios
* (OPTIONAL) If you wish to bundle Espeon with a ROM, drag and drop the ROM file into rom2h.bat (max size of 512 KB)
* Run espeon.ino
* Compile the sketch and upload it to the board

Recommended: upload directly via esptool by using the provided partition scheme, or by using [M5Burner](http://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/software/M5Burner.zip).

A precompiled release will be provived for each new version.

## Running games

If running on a M5Stack, you'll be presented with a menu where you can choose to either load a ROM from the SD card into the ESP32's flash (maximum of 2MB), or boot up the currently flashed ROM.

If either fails, the bundled ROM will be used instead (maximum of 512 KB).

## Credits

* Code structure inspired by [zid](https://github.com/zid).
* Base Arduino code and ROM bundle script by [lualiliu](https://github.com/lualiliu).
* M5Stack TreeView UI library and example code by [lovyan](https://github.com/lovyan03).
* Espeon artwork by [natsuki-o-bento](https://www.deviantart.com/natsukio-bento).

## Features

* Fully taking advantage of both ESP32 cores (currently faster than the real gameboy, needs some tinkering)
* Flash a ROM from the SD card
* SRAM is saved to the SD card

### TODO:

* Rewrite CPU/LR35902 code
* Audio emulation
* Object-oriented approach
* SRAM autosaving (currently needs a button press)
* Customizable palette and border
