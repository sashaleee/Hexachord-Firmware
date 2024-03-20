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
bool isKeyPressed[16] = {};
bool isStringPressed[16] = {};
bool isScreenSaver;
bool screenSvaerEnabled;
////// PARAMETERS //////
uint8_t wheelMode = DEFAULT;
uint8_t deviceMode;

bool isChanged;

bool majorChord = true;
bool minorChord;
bool seventhChord;
bool dimAugChord;
uint8_t rootNote;

uint8_t scale;
uint8_t chord;
uint8_t transpose = 0;

uint8_t keysOctaveIndex = 2;

uint8_t keyChannel = 1;
uint8_t stringsChannel = 2;
////// GET NOTE //////
uint8_t getNote(uint8_t step) {
  if (step > 0) {
    uint8_t octave = ((step - 1) / 7) * 12;
    uint8_t noteIndex = ((step - 1) % 7);
    uint8_t note = SCALES[scale][noteIndex];
    return (note + KEYBOARD_OCTAVES[keysOctaveIndex] + transpose) + octave;
  } else {
    return KEYBOARD_OCTAVES[keysOctaveIndex] + transpose - 12;
  }
}
////// GET CHORD //////
uint8_t getChordStep(uint8_t noteIndex) {
  const uint8_t NOTES_NUM = 3;
  uint8_t octave = (noteIndex / NOTES_NUM) * 12;
  uint8_t root = chord - (chord > 0);
  uint8_t chordType = (chord > 7);
  uint8_t step = chordTypesSteps[chordType][(noteIndex % NOTES_NUM)];
  uint8_t note = SCALES[scale][(step + root) % 7];
  return 36 + octave + note + transpose;
}
////// OMNICHORD CHORD TYPE //////
uint8_t getChordType() {
  uint8_t result;
  if (majorChord & !minorChord & !seventhChord & !dimAugChord) {
    result = MAJOR;
  } else if (!majorChord & minorChord & !seventhChord & !dimAugChord) {
    result = MINOR;
  } else if (majorChord & !minorChord & seventhChord & !dimAugChord) {
    result = MAJ7;
  } else if (!majorChord & minorChord & seventhChord & !dimAugChord) {
    result = MIN7;
  } else if (majorChord & !minorChord & !seventhChord & dimAugChord) {
    result = AUGMENTED;
  } else if (!majorChord & minorChord & !seventhChord & dimAugChord) {
    result = DIMINISHED;
  }
  return result;
}
////// GET OMNICHORD //////
uint8_t getChordSemitones(uint8_t stringIndex) {
  uint8_t chordType = getChordType();
  uint8_t notesNum = 3 + (chordType == MAJ7 || chordType == MIN7);
  uint8_t octave = (stringIndex / notesNum) * 12;
  uint8_t step = stringIndex % notesNum;
  uint8_t note = octave + chordTypesSemitones[chordType][step];
  return note + 36 + circleOfFifth[rootNote];
}
////// SCREEN SAVER //////
void screenSaver(uint16_t delTime) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pixel.setPixelColor(LEDS_ORDER[i], 15 * i, 0, 255 - (i * 15));
    pixel.show();
    delay(delTime);
  }
  isScreenSaver = true;
}
////// REDRAW LEDS //////
void redrawLEDs() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    uint32_t color;
    if (deviceMode == DIATONIC) {
      if (isStringPressed[i] == true) { // string
        color = YELLOW;
      } else if (i == chord) { // chord
        color = RED;
      } else if (isKeyPressed[i] == true) { // key
        color = BLUE;
      }
      // else if (i == scale) { // scale
      //   color = YELLOW;
      // }
      //  else if (i == keysOctaveIndex * 4) {
      //   color = CYAN;
      // }
      // else if (i == 1 + transpose + (transpose / 3)) {
      //   color = PURPLE;
      // }
      else {
        color = 0;
      }
    } else if (deviceMode == CHROMATIC) {
      switch (i) {
      case 0:
        color = CYAN * majorChord;
        break;
      case 4:
        color = CYAN * minorChord;
        break;
      case 8:
        color = CYAN * seventhChord;
        break;
      case 12:
        color = CYAN * dimAugChord;
        break;
      default:
        if (i == 1 + rootNote + (rootNote / 3)) {
          color = PURPLE;
        } else {
          color = 0;
        }
        break;
      }
      //
    }
    pixel.setPixelColor(LEDS_ORDER[i], color);
  }
  pixel.show();
}
////// CLEAR SCREEN //////
void clearScreen() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pixel.setPixelColor(LEDS_ORDER[i], 0);
  }
}
////// SETUP //////
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
  scale = EEPROM.read(SCALE);
  ////// PIXELS //////
  pixel.begin();
  pixel.setBrightness(brightness);
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
      // LOG TO LIN CONVERSION //
      float normalized = (float)CCvalue / 127;
      float linValue = pow(normalized, 0.6) * 127;
      if (wheelMode == DEFAULT) { ////// MODES //////
        static uint8_t lastMode;
        uint8_t reading = map(linValue, 0, 127, 0, 1);
        if (lastMode != reading) {
          deviceMode = reading;
          lastMode = deviceMode;
          redrawLEDs();
        }
      } else if (wheelMode == SCALE) { ////// SCALE //////
        static uint8_t lastScale;
        uint8_t reading = map(linValue, 0, 127, 0, 15);
        if (reading != lastScale) {
          scale = reading;
          clearScreen();
          pixel.setPixelColor(LEDS_ORDER[scale], YELLOW);
          pixel.show();
          lastScale = scale;
          isChanged = true;
        }
      } else if (wheelMode == KEYS_OCTAVE) { ////// OCTAVE //////
        static uint8_t lastOctaveIndex = 2;
        uint8_t reading = map(linValue, 0, 127, 0, 3);
        if (reading != lastOctaveIndex) {
          keysOctaveIndex = reading;
          uint8_t lastNote = KEYBOARD_OCTAVES[lastOctaveIndex] + transpose;
          uint8_t newNote = KEYBOARD_OCTAVES[keysOctaveIndex] + transpose;
          USB_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          USB_MIDI.sendNoteOn(newNote, 127, keyChannel);
          TRS_MIDI.sendNoteOff(lastNote, 0, keyChannel);
          TRS_MIDI.sendNoteOn(newNote, 127, keyChannel);
          clearScreen();
          uint8_t led = keysOctaveIndex * 4;
          pixel.setPixelColor(LEDS_ORDER[led], CYAN);
          pixel.show();
          lastOctaveIndex = keysOctaveIndex;
          isChanged = true;
        }
      } else if (wheelMode == TRANSPOSE) { ////// TRANSPOSE //////
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
          clearScreen();
          uint8_t led = 1 + transpose + (transpose / 3);
          pixel.setPixelColor(LEDS_ORDER[led], PURPLE);
          pixel.show();
          lastTranspose = transpose;
          isChanged = true;
        }
      } else if (wheelMode ==
                 KEYS_CHANNEL) { ////// KEYBOARD MIDI CHANNEL //////
        static uint8_t lastKeyChannel = 1;
        uint8_t reading = map(linValue, 0, 127, 1, 16);
        if (reading != lastKeyChannel) {
          uint8_t note = getNote(2);
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
          keyChannel = reading;
          clearScreen();
          pixel.setPixelColor(LEDS_ORDER[keyChannel - 1], WHITE);
          pixel.show();
          lastKeyChannel = keyChannel;
          isChanged = true;
        }
      } else if (wheelMode ==
                 STRINGS_CHANNEL) { ////// STRINGS MIDI CHANNEL //////
        static uint8_t lastStringsChannel = 2;
        uint8_t reading = map(linValue, 0, 127, 1, 16);
        if (reading != lastStringsChannel) {
          uint8_t note = getNote(3);
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
          stringsChannel = reading;
          clearScreen();
          pixel.setPixelColor(LEDS_ORDER[stringsChannel - 1], WHITE);
          pixel.show();
          lastStringsChannel = stringsChannel;
          isChanged = true;
        }
      } else if (wheelMode == BRIGHTNESS) { ////// LED BRIGHTNESS //////
        static uint8_t lastBrightness;
        uint8_t reading = map(linValue, 0, 127, 2, 255);
        if (reading != lastBrightness) {
          brightness = reading;
          screenSaver(1);
          pixel.setBrightness(brightness);
          pixel.show();
          lastBrightness = brightness;
          isChanged = true;
        }
      } else if (wheelMode ==
                 SCREEN_SAVER_ENABLED) { ////// SCREEN SAVER ON //////
        static uint8_t lastscreenSaverEnabled;
        uint8_t reading = map(linValue, 0, 127, 0, 1);
        if (reading != lastscreenSaverEnabled) {
          screenSvaerEnabled = !screenSvaerEnabled;
          uint8_t note = getNote(5);
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
      } else if (wheelMode == SAVE) { ////// SAVE TO EEPROM //////
        if (isChanged) {
          EEPROM.write(SCALE, scale);
          EEPROM.write(BRIGHTNESS, brightness);
          EEPROM.write(TRANSPOSE, transpose);
          EEPROM.write(KEYS_CHANNEL, keyChannel);
          EEPROM.write(STRINGS_CHANNEL, stringsChannel);
          EEPROM.write(SCREEN_SAVER_ENABLED, screenSvaerEnabled);
          EEPROM.write(KEYS_OCTAVE, keysOctaveIndex);
          EEPROM.commit();
          isChanged = false;
          screenSaver(15);
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
        uint8_t note;
        static uint8_t lastNote;
        lastTouched = millis();
        isScreenSaver = false;
        if (deviceMode == DIATONIC) {
          note = getChordStep(string);
        } else if (deviceMode == CHROMATIC) {
          note = getChordSemitones(string);
        }
        if (stylusReading) {
          if (lastNote > 0) {
            USB_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
            TRS_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
          }
          USB_MIDI.sendNoteOn(note, 127, stringsChannel);
          TRS_MIDI.sendNoteOn(note, 127, stringsChannel);
          isStringPressed[string] = true;
          lastNote = note;
        } else {
          if (note == lastNote) {
            USB_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
            TRS_MIDI.sendNoteOff(lastNote, 0, stringsChannel);
            lastNote = 0;
          }
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
      uint8_t note = getNote(key);
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        lastTouched = millis();
        isScreenSaver = false;
        if (deviceMode == DIATONIC) {
          wheelMode = key;
          chord = key;
          isKeyPressed[key] = true;
          USB_MIDI.sendNoteOn(note, 127, keyChannel);
          TRS_MIDI.sendNoteOn(note, 127, keyChannel);
        } else if (deviceMode == CHROMATIC) {
          switch (key) {
          case 0:
            majorChord = true;
            minorChord = false;
            wheelMode = KEYS_OCTAVE;
            break;
          case 4:
            minorChord = true;
            majorChord = false;
            break;
          case 8:
            seventhChord = !seventhChord;
            dimAugChord = false;
            break;
          case 12:
            dimAugChord = !dimAugChord;
            seventhChord = false;
            break;
          default:
            rootNote = key - (key / 4) - 1;
            uint8_t notePressed =
                circleOfFifth[rootNote] + KEYBOARD_OCTAVES[keysOctaveIndex];
            USB_MIDI.sendNoteOn(notePressed, 127, keyChannel);
            TRS_MIDI.sendNoteOn(notePressed, 127, keyChannel);
            break;
          }
        }
      } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        wheelMode = DEFAULT;
        if (deviceMode == DIATONIC) {
          isKeyPressed[key] = false;
          USB_MIDI.sendNoteOff(note, 0, keyChannel);
          TRS_MIDI.sendNoteOff(note, 0, keyChannel);
        } else if (deviceMode == CHROMATIC) {
          uint8_t keyReleased = key - (key / 4) - 1;
          if (key != 0 && key != 4 && key != 8 && key != 12) {
            uint8_t noteReleased =
                circleOfFifth[keyReleased] + KEYBOARD_OCTAVES[keysOctaveIndex];
            USB_MIDI.sendNoteOff(noteReleased, 0, keyChannel);
            TRS_MIDI.sendNoteOff(noteReleased, 0, keyChannel);
          }
        }
      }
      redrawLEDs();
    }
    ////// SCREEN SAVER //////
    if (millis() > lastTouched + SCREEN_SAVER_TIMOUT && !isScreenSaver &&
        screenSvaerEnabled) {
      screenSaver(20);
    }
  }
}