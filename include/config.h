#ifndef CONFIG_H_
#define CONFIG_H_
#include <Arduino.h>

const uint8_t MIDI_TX_PIN = 12; // MIDI out pin
const uint8_t MIDI_RX_PIN = 13; // MIDI in pin
const uint8_t STYLUS_PIN = 3;
const uint8_t STRINGS_ORDER[16] = {8, 15, 14, 13, 12, 11, 10, 9,
                                   0, 7,  6,  5,  4,  3,  2,  1};

const uint8_t LEDS_ORDER[16] = {3,  2,  1, 0, 7,  6,  5,  4,
                                11, 10, 9, 8, 15, 14, 13, 12};

enum ParametersNames {
  KEYS_OCTAVE,
  TRANSPOSE,
  KEYS_CHANNEL,
  STRINGS_CHANNEL,
  BRIGHTNESS,
  KEYBOARD,
  SAVE = 15,
  PARAM_NUM
};

const uint8_t WHEEL_PIN = A0;

const uint8_t DATA_PIN = 20;
const uint8_t LATCH_PIN = 19;
const uint8_t CLOCK_PIN = 18;

const uint8_t ROWS = 4; // rows
const uint8_t COLS = 4; // columns
uint8_t colPins[ROWS] = {4, 5, 6, 7};
uint8_t rowPins[COLS] = {8, 9, 10, 11};

const uint8_t keys[ROWS][COLS] = {
    {0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}};

const uint8_t LED_PIN = 2; // Data out pin for SK6812 LEDs
const uint8_t LED_COUNT = 16;

const uint8_t USB_MIDI_CHANNEL = 1;
const uint8_t UPDATE_INTERVAL = 10;

const uint16_t SCREEN_SAVER_TIMOUT = 60000;

enum Colors {
  RED = 0x00FF0000,
  GREEN = 0x00AAFF00,
  BLUE = 0x000000FF,
  CYAN = 0x0000BBFF,
  YELLOW = 0x00FF2200,
  PURPLE = 0x00FF00AA,
  WHITE = 0x00FFFFFF
};
const uint32_t colors[4] = {RED, YELLOW, BLUE, CYAN};
#endif