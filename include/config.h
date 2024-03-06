#ifndef CONFIG_H_
#define CONFIG_H_
#include <Arduino.h>

const uint8_t STYLUS_PIN = 3;
const uint8_t PADS_ORDER[16] = {8, 15, 14, 13, 12, 11, 10, 9,
                                 0, 7,  6,  5,  4,  3,  2,  1};

const uint8_t LEDS_ORDER[16] = {3,  2,  1, 0, 7,  6,  5,  4,
                                11, 10, 9, 8, 15, 14, 13, 12};
const uint8_t WHEEL_PIN = A0;

const uint8_t DATA_PIN = 20;
const uint8_t LATCH_PIN = 19;
const uint8_t CLOCK_PIN = 18;

const byte ROWS = 4; // rows
const byte COLS = 4; // columns
byte colPins[ROWS] = {4, 5, 6, 7};
byte rowPins[COLS] = {8, 9, 10, 11};

const uint8_t keys[ROWS][COLS] = {
    {0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}};

const uint8_t LED_PIN = 2; // Data out pin for SK6812 LEDs
const uint8_t LED_COUNT = 16;

const uint8_t USB_MIDI_CHANNEL = 1;
const uint8_t UPDATE_INTERVAL = 10;

enum Colors {
  RED = 0x00FF0000,
  GREEN = 0x0000FF00,
  BLUE = 0x000000FF,
  CYAN = 0x0000BBFF,
  YELLOW = 0x00FF2200,
  PURPLE = 0x00FF00AA
};
const uint32_t colors[4] = {RED, YELLOW, BLUE, CYAN};
#endif