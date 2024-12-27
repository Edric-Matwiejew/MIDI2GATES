#include <MIDIUSB.h>

// -------------------------------
//   USER DEFINES & GLOBALS
// -------------------------------

// Pins for notes and clock
const byte noteToPin[] = {9, 8, 7, 6, 5, 4, 3, 10};
const byte inputNotes[] = {60, 61, 62, 63, 64, 65, 66, 67};
const byte midiClockPin = 2;
const byte numNotes = 8;

// On-board LED pin on Pro Micro
const byte ledPin = 17;         // SparkFun Pro Micro typically uses pin 17 for the onboard LED
const unsigned long blinkTime = 100;  // How long (ms) to keep LED on after receiving a note

// Clock divider
const byte clockDivider = 12;
byte clockCount = 0;
bool clockState = false;

// Track when to turn off the LED (non-blocking timing)
unsigned long ledTurnOffAt = 0;

void setup() {
  // Set pins for note gates
  for (byte i = 0; i < numNotes; i++) {
    pinMode(noteToPin[i], OUTPUT);
    digitalWrite(noteToPin[i], LOW);
  }

  // Set pin for MIDI clock out
  pinMode(midiClockPin, OUTPUT);
  digitalWrite(midiClockPin, LOW);

  // Set up the on-board LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  // Read MIDI
  midiEventPacket_t rx = MidiUSB.read();
  if (rx.header != 0) {
    byte command = rx.byte1 & 0xF0;  // e.g., 0x90 = Note On, 0x80 = Note Off
    byte channel = rx.byte1 & 0x0F;  // 0 = Channel 1, 1 = Channel 2, etc.
    byte note    = rx.byte2;
    byte velocity= rx.byte3;

    // --------------------
    // CHANNEL 1 CHECK
    // --------------------
    if (channel == 0) { // Listening to MIDI Channel 1
      switch (command) {
        case 0x90: // Note On
          if (velocity > 0) {
            handleNoteOn(note);
          } else {
            handleNoteOff(note); // velocity == 0 -> Note Off
          }
          break;

        case 0x80: // Note Off
          handleNoteOff(note);
          break;

        default:
          // Ignore other channel messages
          break;
      }
    }

    // --------------------
    // CLOCK HANDLING
    // --------------------
    if (rx.byte1 == 0xF8) {
      handleMidiClock();
    }
  }

  // -------------------------
  //  LED BLINK TIMER CHECK
  // -------------------------
  if (ledTurnOffAt > 0 && millis() >= ledTurnOffAt) {
    // Time to turn off LED
    digitalWrite(ledPin, LOW);
    ledTurnOffAt = 0;
  }
}

// -----------------------------
//        NOTE ON / OFF
// -----------------------------
void handleNoteOn(byte note) {
  for (byte i = 0; i < numNotes; i++) {
    if (note == inputNotes[i]) {
      digitalWrite(noteToPin[i], HIGH);
      blinkOnboardLED(); // Blink the onboard LED for each Note On
    }
  }
}

void handleNoteOff(byte note) {
  for (byte i = 0; i < numNotes; i++) {
    if (note == inputNotes[i]) {
      digitalWrite(noteToPin[i], LOW);
    }
  }
}

// -----------------------------
//        MIDI CLOCK
// -----------------------------
void handleMidiClock() {
  clockCount++;
  if (clockCount >= clockDivider) {
    clockCount = 0;
    clockState = !clockState;
    digitalWrite(midiClockPin, clockState ? HIGH : LOW);
  }
}

// -----------------------------
//   BLINK ONBOARD LED LOGIC
// -----------------------------
void blinkOnboardLED() {
  // Turn LED on immediately
  digitalWrite(ledPin, HIGH);

  // Schedule it to turn off after 'blinkTime' ms (non-blocking)
  ledTurnOffAt = millis() + blinkTime;
}
