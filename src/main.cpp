#include "includes.h"
#include <Arduino.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USB_MIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, TRS_MIDI);
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Potentiometer wheel;

Adafruit_Keypad customKeypad =
    Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
////// LED //////
uint8_t brightness = 50;
uint64_t lastTouched;
bool isScreenSaver;
bool isChanged;
bool screenSvaerEnabled;
uint8_t page = KEYBOARD;

////// PARAMETERS //////
uint8_t scale;
uint8_t chord;
uint8_t keysOctaveIndex = 2;
uint8_t stringsOctaveIndex = 2;
const uint8_t KEYBOARD_OCTAVES[4] = {36, 48, 60, 72};
bool isKeyPressed[16] = {};
bool isStringPressed[16] = {};
uint8_t transpose = 0;
uint8_t keyChannel = 1;
uint8_t stringsChannel = 2;
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
////// CHORD TYPES //////
const int8_t chordTypesSteps[4][3] = {
    {0, 2, 4}, {0, 2, 6}, {0, 1, 4}, {1, 3, 6}};
const int8_t chordTypesSemitones[10][3] = {
    {0, 4, 7},  // Major
    {0, 3, 7},  // Minor
    {0, 3, 6},  // Diminished
    {0, 4, 8},  // Augmented
    {0, 2, 7},  // Suspended 2nd
    {0, 5, 7},  // Suspended 4th
    {0, 4, 10}, // Dominant 7th (no fifth)
    {0, 3, 10}, // Minor 7th (no fifth)
    {0, 4, 11}, // Major 7th (no fifth)
};
////// GET NOTE //////
uint8_t getNote(uint8_t scaleIndex, uint8_t noteIndex) {
  if (noteIndex > 0) {
    uint8_t octave = ((noteIndex - 1) / 7) * 12;
    uint8_t note = SCALES[scaleIndex][((noteIndex - 1) % 7)];
    return (note + KEYBOARD_OCTAVES[keysOctaveIndex] + transpose) + octave;
  } else {
    return KEYBOARD_OCTAVES[keysOctaveIndex] + transpose - 12;
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
  uint8_t step = chordTypesSteps[chordType][(noteIndex % NOTES_NUM)];
  uint8_t note = SCALES[scaleIndex][(step + root) % 7];
#if (DEBUGGING == ON)
  USB_MIDI.sendControlChange(1, octave, 1);
  USB_MIDI.sendControlChange(2, root, 1);
  USB_MIDI.sendControlChange(3, step, 1);
  USB_MIDI.sendControlChange(4, note, 1);
  USB_MIDI.sendControlChange(5, noteIndex, 1);
  USB_MIDI.sendControlChange(6, chordType, 1);
#endif
  return 36 + octave + note + transpose;
}
////// SCREEN SAVER //////
void screenSaver(uint16_t delTime) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pixel.setPixelColor(LEDS_ORDER[i], 15 * i, 0, 255 - (i * 15));
    pixel.show();
    delay(delTime); // 20
  }
  isScreenSaver = true;
}
void redrawLEDs() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    uint32_t color;
    if (isStringPressed[i] == true) {
      color = GREEN;
    } else if (i == chord) {
      color = RED;
    } else if (isKeyPressed[i] == true) {
      color = BLUE;
    } else if (i == scale) {
      color = YELLOW;
    } else if (i == keysOctaveIndex * 4) {
      color = CYAN;
    } else if (i == 1 + transpose + (transpose / 3)) {
      color = PURPLE;
    } else {
      color = 0;
    }
    pixel.setPixelColor(LEDS_ORDER[i], color);
  }
  pixel.show();
}
void clearScreen() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pixel.setPixelColor(LEDS_ORDER[i], 0);
  }
}
void setup() {
  analogReadResolution(12);
  Serial1.setRX(MIDI_RX_PIN);
  Serial1.setTX(MIDI_TX_PIN);
  pinMode(STYLUS_PIN, INPUT_PULLDOWN);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  ////// WHEEL //////
  wheel.begin(WHEEL_PIN);
  ////// KEYBOARD //////
  customKeypad.begin();
  ////// MIDI //////
  USB_MIDI.begin(MIDI_CHANNEL_OMNI);
  TRS_MIDI.begin(MIDI_CHANNEL_OMNI);
  USB_MIDI.turnThruOff();
  ////// EEPROM //////
  EEPROM.begin(1024);
  brightness = EEPROM.read(BRIGHTNESS);
  keyChannel = EEPROM.read(KEYS_CHANNEL);
  stringsChannel = EEPROM.read(STRINGS_CHANNEL);
  transpose = EEPROM.read(TRANSPOSE);
  screenSvaerEnabled = EEPROM.read(SCREEN_SAVER_ENABLED);
  keysOctaveIndex = EEPROM.read(KEYS_OCTAVE);
  ////// PIXELS //////
  pixel.begin();
  pixel.setBrightness(brightness);
  screenSaver(40);
  delay(300);
  redrawLEDs();
}

void loop() {
  static uint64_t lastUpdateTime;
  if (millis() > lastUpdateTime + UPDATE_INTERVAL) {
    lastUpdateTime = millis();
    ////// WHEEL //////
    if (wheel.update()) {
      lastTouched = millis();
      isScreenSaver = false;
      uint8_t CCvalue = wheel.getValue();
#if (DEBUGGING == ON)
      USB_MIDI.sendControlChange(100, CCvalue, 1);
#endif
      // LOG TO LIN CONVERSION //
      float normalized = (float)CCvalue / 127;
      float linValue = pow(normalized, 0.6) * 127;
      if (page == KEYBOARD) { ////// SCALE //////
        static uint8_t lastScale;
        uint8_t reading = map(linValue, 0, 127, 0, 15);
        if (reading != lastScale) {
          scale = reading;
          redrawLEDs();
          lastScale = scale;
#if (DEBUGGING == ON)
          USB_MIDI.sendControlChange(101, scale, 1);
#endif
        }
      } else if (page == KEYS_OCTAVE) { ////// OCTAVE //////
        static uint8_t lastOctaveIndex = 2;
        uint8_t reading = map(linValue, 0, 127, 0, 3);
        if (reading != lastOctaveIndex) {
          keysOctaveIndex = reading;
          redrawLEDs();
          uint8_t lastNote = KEYBOARD_OCTAVES[lastOctaveIndex] + transpose - 12;
          uint8_t newNote = KEYBOARD_OCTAVES[keysOctaveIndex] + transpose - 12;
          USB_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          USB_MIDI.sendNoteOn(newNote, 127, keyChannel);
          TRS_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          TRS_MIDI.sendNoteOn(newNote, 127, keyChannel);
          lastOctaveIndex = keysOctaveIndex;
          isChanged = true;
        }
      } else if (page == TRANSPOSE) { ////// TRANSPOSE //////
        static uint8_t lastTranspose = 0;
        uint8_t reading = map(linValue, 0, 127, 0, 11);
        if (reading != lastTranspose) {
          transpose = reading;
          uint8_t lastNote = KEYBOARD_OCTAVES[keysOctaveIndex] + lastTranspose;
          uint8_t newNote = KEYBOARD_OCTAVES[keysOctaveIndex] + transpose;
          USB_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          USB_MIDI.sendNoteOn(newNote, 127, keyChannel);
          TRS_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          TRS_MIDI.sendNoteOn(newNote, 127, keyChannel);
          uint8_t lastLed = 1 + lastTranspose + (lastTranspose / 3);
          uint8_t newLed = 1 + transpose + (transpose / 3);
          redrawLEDs();
          lastTranspose = transpose;
          isChanged = true;
        }
      } else if (page == KEYS_CHANNEL) { ////// KEYBOARD MIDI CHANNEL //////
        static uint8_t lastKeyChannel = 1;
        uint8_t reading = map(linValue, 0, 127, 1, 16);
        if (reading != lastKeyChannel) {
          uint8_t note = getNote(scale, 2);
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
          keyChannel = reading;
          clearScreen();
          pixel.setPixelColor(LEDS_ORDER[keyChannel - 1], WHITE);
          pixel.show();
          lastKeyChannel = keyChannel;
          isChanged = true;
        }
      } else if (page == STRINGS_CHANNEL) { ////// STRINGS MIDI CHANNEL //////
        static uint8_t lastStringsChannel = 2;
        uint8_t reading = map(linValue, 0, 127, 1, 16);
        if (reading != lastStringsChannel) {
          uint8_t note = getNote(scale, 3);
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
          stringsChannel = reading;
          clearScreen();
          pixel.setPixelColor(LEDS_ORDER[stringsChannel - 1], WHITE);
          pixel.show();
          lastStringsChannel = stringsChannel;
          isChanged = true;
        }
      } else if (page == BRIGHTNESS) { ////// STRINGS MIDI CHANNEL //////
        static uint8_t lastBrightness;
        uint8_t reading = map(linValue, 0, 127, 2, 255);
        if (reading != lastBrightness) {
          brightness = reading;
          screenSaver(0);
          pixel.setBrightness(brightness);
          pixel.show();
          lastBrightness = brightness;
          isChanged = true;
        }
      } else if (page == SCREEN_SAVER_ENABLED) {
        static uint8_t lastscreenSaverEnabled;
        uint8_t reading = map(linValue, 0, 127, 0, 1);
        if (reading != lastscreenSaverEnabled) {
          screenSvaerEnabled = !screenSvaerEnabled;
          uint8_t note = getNote(scale, 5);
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
          if (screenSvaerEnabled) {
            screenSaver(1);
          } else {
            clearScreen();
            pixel.show();
          }
          isChanged = true;
          lastscreenSaverEnabled = reading;
        }

      } else if (page == SAVE) { ////// SAVE TO EEPROM //////
        if (isChanged) {
          EEPROM.write(BRIGHTNESS, brightness);
          EEPROM.write(TRANSPOSE, transpose);
          EEPROM.write(KEYS_CHANNEL, keyChannel);
          EEPROM.write(STRINGS_CHANNEL, stringsChannel);
          EEPROM.write(SCREEN_SAVER_ENABLED, screenSvaerEnabled);
          EEPROM.write(KEYS_OCTAVE, keysOctaveIndex);
          EEPROM.commit();
          isChanged = false;
          screenSaver(1);
          redrawLEDs();
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
      static uint8_t stylusLastReading[16];
      if (stylusLastReading[i] != stylusReading) {
        uint8_t string = STRINGS_ORDER[i];
        uint8_t led = LEDS_ORDER[string];
        uint8_t note = getChordStep(scale, chord, string);
        static uint8_t lastNote;
        lastTouched = millis();
        isScreenSaver = false;
        if (stylusReading) {
          if (lastNote > 0) {
            USB_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
            TRS_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
          }
          USB_MIDI.sendNoteOn(note, 127, stringsChannel);
          TRS_MIDI.sendNoteOn(note, 127, stringsChannel);
#if (DEBUGGING == ON)
          USB_MIDI.sendControlChange(7, string, 2);
#endif
          isStringPressed[string] = true;
          lastNote = note;
        } else {
          USB_MIDI.sendNoteOff(note, 0, stringsChannel);
          TRS_MIDI.sendNoteOff(note, 0, stringsChannel);
          isStringPressed[string] = false;
        }
        redrawLEDs();
        stylusLastReading[i] = stylusReading;
      }
    }
    ////// SWITCHES //////
    customKeypad.tick();
    while (customKeypad.available()) {
      keypadEvent e = customKeypad.read();
      uint8_t key = e.bit.KEY;
      uint8_t led = LEDS_ORDER[key];
      uint8_t note = getNote(scale, key);
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        lastTouched = millis();
        page = key;
        chord = key;
        isKeyPressed[key] = true;
        isScreenSaver = false;
        redrawLEDs();
#if (DEBUGGING == ON)
        USB_MIDI.sendControlChange(80, page, 1);
        USB_MIDI.sendControlChange(8, chord, 1);
#endif
        USB_MIDI.sendNoteOn(note, 127, keyChannel);
        TRS_MIDI.sendNoteOn(note, 127, keyChannel);
      } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        page = KEYBOARD;
        isKeyPressed[key] = false;
        redrawLEDs();
#if (DEBUGGING == ON)
        USB_MIDI.sendControlChange(80, page, 1);
#endif
        USB_MIDI.sendNoteOff(note, 0, keyChannel);
        TRS_MIDI.sendNoteOff(note, 0, keyChannel);
      }
    }
    ////// SCREEN SAVER //////
    if (millis() > lastTouched + SCREEN_SAVER_TIMOUT && !isScreenSaver &&
        screenSvaerEnabled) {
      screenSaver(20);
    }
  }
}