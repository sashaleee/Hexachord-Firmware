#include "includes.h"
#include <Arduino.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USB_MIDI);
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Potentiometer wheel;

Adafruit_Keypad customKeypad =
    Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

uint32_t currentColor;
uint8_t red[16] = {};
uint8_t green[16] = {};
uint8_t blue[16] = {};

uint8_t stylusLastReading[16];
uint64_t lastUpdateTime;

uint8_t page = KEYBOARD;

////// PARAMETERS //////
uint8_t currentScale;
uint8_t currentChord;
uint8_t currentChordType;
uint8_t octaveIndex = 2;
uint8_t keyboardOctaves[4] = {36, 48, 60, 72};
uint8_t transpose = 0;
uint8_t keyChannel = 1;
uint8_t stringsChannel = 2;
////// SCALES //////
int8_t scales[16][7] = {
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
////// CHORD TYPES //////
int8_t chordTypes[4][3] = {{0, 2, 4}, {0, 2, 6}, {0, 1, 4}, {1, 3, 6}};
////// GET NOTE //////
uint8_t getNote(uint8_t scaleIndex, uint8_t noteIndex) {
  if (noteIndex > 0) {
    uint8_t octave = ((noteIndex - 1) / 7) * 12;
    uint8_t note = scales[scaleIndex][((noteIndex - 1) % 7)];
    return (note + keyboardOctaves[octaveIndex] + transpose) + octave;
  } else {
    return keyboardOctaves[octaveIndex] + transpose - 12;
  }
}

////// GET CHORD //////
uint8_t getChordStep(uint8_t scaleIndex, uint8_t chordIndex,
                     uint8_t noteIndex) {
  const uint8_t NOTES_NUM = 3;

  uint8_t octave = (noteIndex / NOTES_NUM) * 12;
  uint8_t root = chordIndex - (chordIndex > 0);
  // uint8_t chordType = (chordIndex > 7) + (noteIndex > 7);
  uint8_t chordType = (chordIndex > 7);
  uint8_t step = chordTypes[chordType][(noteIndex % NOTES_NUM)];
  uint8_t note = scales[scaleIndex][(step + root) % 7];

  USB_MIDI.sendControlChange(1, octave, 1);    // debugging
  USB_MIDI.sendControlChange(2, root, 1);      // debugging
  USB_MIDI.sendControlChange(3, step, 1);      // debugging
  USB_MIDI.sendControlChange(4, note, 1);      // debugging
  USB_MIDI.sendControlChange(5, noteIndex, 1); // debugging
  USB_MIDI.sendControlChange(6, chordType, 1); // debugging

  return 36 + octave + note + transpose;
}

void setup() {
  analogReadResolution(12);
  pinMode(STYLUS_PIN, INPUT_PULLDOWN);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  ////// WHEEL //////
  wheel.begin(WHEEL_PIN);
  ////// PIXELS //////
  pixel.begin();
  pixel.setBrightness(255);
  pixel.show();
  ////// KEYBOARD //////
  customKeypad.begin();
  ////// MIDI //////
  USB_MIDI.begin(MIDI_CHANNEL_OMNI);
  USB_MIDI.turnThruOff();
}

void loop() {
  if (millis() > lastUpdateTime + UPDATE_INTERVAL) {
    lastUpdateTime = millis();
    ////// WHEEL //////
    if (wheel.update()) {
      uint8_t CCvalue = wheel.getValue();
      // LOG TO LIN CONVERSION //
      float normalized = (float)CCvalue / 127;
      float linValue = pow(normalized, 0.6) * 127;
      if (page == KEYBOARD) {
        // TURN THE LAST LED OFF //
        green[currentScale] = 0;
        pixel.setPixelColor(LEDS_ORDER[currentScale], red[currentScale],
                            green[currentScale], blue[currentScale]);
        // ASSIGN NEW VALUE //
        currentScale = map(linValue, 0, 127, 0, 15);
        // currentScale = CCvalue >> 3;
        // TURN THE LED ON //
        green[currentScale] = 255;
        pixel.setPixelColor(LEDS_ORDER[currentScale], red[currentScale],
                            green[currentScale], blue[currentScale]);
        pixel.show();

        USB_MIDI.sendControlChange(100, CCvalue, 1);      // debugging
        USB_MIDI.sendControlChange(101, currentScale, 1); // debugging
      } else if (page == KEYS_OCTAVE) {                   ////// OCTAVE //////
        static uint8_t lastOctaveIndex = 2;
        uint8_t reading = map(linValue, 0, 127, 0, 3);
        if (reading != lastOctaveIndex) {
          octaveIndex = reading;
          pixel.setPixelColor(LEDS_ORDER[lastOctaveIndex * 4], 0, 0, 0);
          pixel.setPixelColor(LEDS_ORDER[octaveIndex * 4], 255, 100, 0);
          pixel.show();
          uint8_t lastNote = keyboardOctaves[lastOctaveIndex] + transpose - 12;
          uint8_t newNote = keyboardOctaves[octaveIndex] + transpose - 12;
          USB_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          USB_MIDI.sendNoteOn(newNote, 127, keyChannel);
          lastOctaveIndex = octaveIndex;
        }
      } else if (page == TRANSPOSE) { ////// TRANSPOSE //////
        static uint8_t lastTranspose = 0;
        uint8_t reading = map(linValue, 0, 127, 0, 11);
        if (reading != lastTranspose) {
          transpose = reading;
          uint8_t lastNote = keyboardOctaves[octaveIndex] + lastTranspose;
          uint8_t newNote = keyboardOctaves[octaveIndex] + transpose;
          USB_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          USB_MIDI.sendNoteOn(newNote, 127, keyChannel);
          uint8_t lastLed = 1 + lastTranspose + (lastTranspose / 3);
          uint8_t newLed = 1 + transpose + (transpose / 3);
          pixel.setPixelColor(LEDS_ORDER[lastLed], 0, 0, 0);
          pixel.setPixelColor(LEDS_ORDER[newLed], 255, 0, 255);
          pixel.show();
          lastTranspose = transpose;
        }
      }
    }
    ////// STYLUS //////
    for (uint8_t i = 0; i < 16; i++) {
      digitalWrite(LATCH_PIN, LOW);
      uint16_t currentString = 1 << i;
      shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, currentString >> 8);
      shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, currentString);
      digitalWrite(LATCH_PIN, HIGH);
      uint8_t stylusReading = digitalRead(STYLUS_PIN);
      if (stylusLastReading[i] != stylusReading) {
        uint8_t string = STRINGS_ORDER[i];
        uint8_t led = LEDS_ORDER[string];
        uint8_t note = getChordStep(currentScale, currentChord, string);
        static uint8_t lastNote;
        if (stylusReading) {
          if (lastNote > 0) {
            USB_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
          }
          USB_MIDI.sendNoteOn(note, 127, stringsChannel);
          USB_MIDI.sendControlChange(7, string, 2); // debuggig
          lastNote = note;
          red[string] = 255;
          pixel.setPixelColor(led, red[string], green[string], blue[string]);
          pixel.show();
        } else {
          USB_MIDI.sendNoteOff(note, 0, stringsChannel);
          red[string] = 0;
          pixel.setPixelColor(led, red[string], green[string], blue[string]);
          pixel.show();
        }
        stylusLastReading[i] = stylusReading;
      }
    }
    ////// SWITCHES //////
    customKeypad.tick();
    while (customKeypad.available()) {
      keypadEvent e = customKeypad.read();
      uint8_t led = LEDS_ORDER[e.bit.KEY];
      uint8_t note = getNote(currentScale, e.bit.KEY);
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        // PAGE
        if (e.bit.KEY <= STRINGS_CHANNEL) {
          page = e.bit.KEY;
          USB_MIDI.sendControlChange(80, page, 1); // debuggig
        }
        // CHORD
        currentChord = e.bit.KEY;
        USB_MIDI.sendControlChange(8, currentChord, 1); // debuggig
        USB_MIDI.sendNoteOn(note, 127, keyChannel);
        blue[e.bit.KEY] = 255;
        pixel.setPixelColor(led, red[e.bit.KEY], green[e.bit.KEY],
                            blue[e.bit.KEY]);
        pixel.show();
      } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        if (e.bit.KEY <= STRINGS_CHANNEL) {
          page = KEYBOARD;
          USB_MIDI.sendControlChange(80, page, 1); // debuggig
          pixel.setPixelColor(LEDS_ORDER[currentScale], red[currentScale],
                              green[currentScale], blue[currentScale]);
          pixel.show();
        }
        USB_MIDI.sendNoteOff(note, 0, keyChannel);
        blue[e.bit.KEY] = 0;
        pixel.setPixelColor(led, red[e.bit.KEY], green[e.bit.KEY],
                            blue[e.bit.KEY]);
        pixel.show();
      }
    }
  }
}
