#include "includes.h"
#include <Arduino.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USB_MIDI);
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Potentiometer wheel;

Adafruit_Keypad customKeypad =
    Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int8_t scales[4][16] = {
    {-12, 0, 2, 3, 5, 7, 9, 10, 12, 14, 15, 17, 19, 21, 22, 24}, // Dorian
    {-12, 0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24}, // Minor
    {-12, 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24}, // Major
    {-12, 0, 2, 3, 5, 7, 8, 11, 12, 14, 15, 17, 19, 20, 23, 24}  // Harmonic
};
uint8_t currentScale;
uint32_t currentColor;

uint8_t transpose = 60;

uint8_t stylusLastReading[16];
uint64_t lastUpdateTime;

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
  pixel.setBrightness(150);
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pixel.setPixelColor(i, 0, 0, 100);
  }
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
      currentScale = CCvalue >> 5;
      currentColor = colors[CCvalue >> 5];
      for (uint8_t i = 0; i < LED_COUNT; i++) {
        pixel.setPixelColor(i, currentColor);
        pixel.show();
      }
      USB_MIDI.sendControlChange(100, CCvalue, USB_MIDI_CHANNEL);
    }
    ////// STYLUS //////
    for (uint8_t i = 0; i < 16; i++) {
      digitalWrite(LATCH_PIN, LOW);
      uint16_t currentPad = 1 << i;
      shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, currentPad >> 8);
      shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, currentPad);
      digitalWrite(LATCH_PIN, HIGH);
      uint8_t stylusReading = digitalRead(STYLUS_PIN);
      if (stylusLastReading[i] != stylusReading) {
        uint8_t pad = PADS_ORDER[i];
        uint8_t led = LEDS_ORDER[pad];
        uint8_t note = transpose + scales[currentScale][pad];
        USB_MIDI.sendControlChange(note, stylusReading, 1);
        if (stylusReading) {
          USB_MIDI.sendNoteOn(note, 127, 1);
          pixel.setPixelColor(led, currentColor);
          pixel.show();
        } else {
          USB_MIDI.sendNoteOff(note, 0, 1);
          pixel.setPixelColor(led, 0, 0, 0);
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
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        USB_MIDI.sendNoteOn(transpose + scales[currentScale][e.bit.KEY], 127,
                            1);
        pixel.setPixelColor(led, currentColor);
        pixel.show();
      } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        USB_MIDI.sendNoteOff(transpose + scales[currentScale][e.bit.KEY], 0, 1);
        pixel.setPixelColor(led, 0, 0, 0);
        pixel.show();
      }
    }
  }
}
