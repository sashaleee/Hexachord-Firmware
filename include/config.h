#ifndef CONFIG_H_
#define CONFIG_H_
#include <Arduino.h>

#define OFF 0
#define ON 1
#define DEBUGGING OFF
////// ORDER //////
const uint8_t ROWS = 4; // rows
const uint8_t COLS = 4; // columns
const uint8_t keys[ROWS][COLS] = {
    {0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}};
const uint8_t STRINGS_ORDER[16] = {8, 15, 14, 13, 12, 11, 10, 9,
                                   0, 7,  6,  5,  4,  3,  2,  1};
const uint8_t LEDS_ORDER[16] = {3,  2,  1, 0, 7,  6,  5,  4,
                                11, 10, 9, 8, 15, 14, 13, 12};
const uint8_t LED_COUNT = 16;
////// PINS //////
const uint8_t MIDI_TX_PIN = 12; // MIDI out pin
const uint8_t MIDI_RX_PIN = 13; // MIDI in pin
uint8_t colPins[ROWS] = {4, 5, 6, 7};
uint8_t rowPins[COLS] = {8, 9, 10, 11};
const uint8_t LED_PIN = 2; // Data out pin for SK6812 LEDs
const uint8_t STYLUS_PIN = 3;
const uint8_t WHEEL_PIN = A0;
const uint8_t DATA_PIN = 20;
const uint8_t LATCH_PIN = 19;
const uint8_t CLOCK_PIN = 18;
////// TIME //////
const uint8_t UPDATE_INTERVAL = 10;
const uint16_t SCREEN_SAVER_TIMOUT = 60000;
////// SCALES //////
const int8_t SCALES[16][7] = {
    {0, 2, 4, 5, 7, 9, 11},  // 1.Major (Ionian)
    {0, 2, 3, 5, 7, 9, 10},  // 2.Dorian
    {0, 1, 3, 5, 7, 8, 10},  // 3.Phrygian
    {0, 2, 4, 6, 7, 9, 11},  // 4.Lydian
    {0, 2, 4, 5, 7, 9, 10},  // 5.Mixolydian
    {0, 2, 3, 5, 7, 8, 10},  // 6.Minor (Aeolian)
    {0, 1, 3, 5, 6, 8, 10},  // 7.Locrian
    {0, 2, 3, 5, 7, 8, 11},  // 8.Harmonic Minor
    {0, 2, 3, 5, 7, 9, 11},  // 9.Melodic Minor (Ascending)
    {0, 1, 4, 5, 7, 8, 10},  // 10.Phrygian Dominant
    {0, 1, 3, 5, 7, 9, 10},  // 11.Dorian ♭2 (also known as Phrygian #6)
    {0, 2, 4, 5, 7, 8, 11},  // 12.Lydian #5
    {0, 2, 4, 6, 7, 8, 10},  // 13.Lydian Dominant (Lydian ♭7)
    {0, 2, 3, 5, 6, 8, 10},  // 14.Locrian #2
    {0, 3, 5, 6, 7, 10, 11}, // 15.Augmented Scale
    {0, 2, 3, 6, 7, 8, 11}   // 16.Double Harmonic Major
};
////// CIRCLE OF FIFTH IN SEMITONES //////
const uint8_t circleOfFifth[] = {5, 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10};
////// CHORD TYPES //////
const int8_t chordTypesSteps[4][3] = {{0, 2, 4},  // Triad
                                      {0, 2, 6},  // 7th chord
                                      {0, 1, 4},  //
                                      {1, 3, 6}}; //
const int8_t chordTypesSemitones[10][4] = {
    {0, 4, 7, -1}, // Major
    {0, 3, 7, -1}, // Minor
    {0, 4, 7, 11}, // Major 7th
    {0, 3, 7, 10}, // Minor 7th
    {0, 4, 8, -1}, // Augmented
    {0, 3, 6, -1}, // Diminished

    {0, 2, 7, -1}, // Suspended 2nd
    {0, 5, 7, -1}, // Suspended 4th
    {0, 4, 7, 10}  // Dominant 7th
};
enum Modes { DIATONIC, CHROMATIC };
enum ChromaticChords { MAJOR, MINOR, MAJ7, MIN7, AUGMENTED, DIMINISHED };
enum DiatonicParameters {
  KEYS_OCTAVE,
  KEYS_CHANNEL,
  STRINGS_CHANNEL,
  BRIGHTNESS,
  SCREEN_SAVER_ENABLED,
  SCALE,
  TRANSPOSE,
  DEFAULT,
  SAVE = 15
};

enum Colors {
  RED = 0x00FF0000,
  GREEN = 0x00AAFF00,
  BLUE = 0x000000FF,
  CYAN = 0x0000BBFF,
  YELLOW = 0x00FF2200,
  PURPLE = 0x00FF00AA,
  WHITE = 0x00FFFFFF
};
#endif