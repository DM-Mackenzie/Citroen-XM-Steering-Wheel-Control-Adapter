#include <MCP4151.h>
#include <SPI.h>

const int pin2 = A0; // Pin 2 of steering wheel stalk
const int pin4 = A1; // Pin 4 of steering wheel stalk
const float knownResistor = 1000.0; // 1k ohm reference resistor

// MCP4151 settings
const int csPin = 10; // Chip Select pin voor de MCP4151
const int mosiPin = 11;
const int misoPin = 12;
const int sckPin = 13; 
MCP4151 pot(csPin, mosiPin, misoPin, sckPin);

const int mosfetGate = 9; // Mosfet pin, to allow for extra functions in Pioneer radio's on the 3.5mm ring

// Time related variables to store time between presses, used for double-press functionality
unsigned long lastPressTimePin2Skip = 0;
unsigned long lastPressTimePin2Memo = 0;
unsigned long lastPressTimePin4Mode = 0;
unsigned long lastPressTimePin4Mute = 0;

bool waitingForSecondPressPin2Skip = false;
bool waitingForSecondPressPin2Memo = false;
bool waitingForSecondPressPin4Mode = false;
bool waitingForSecondPressPin4Mute = false;

const unsigned long doublePressDelay = 500; // 500 milliseconds, can be adjusted for better feel

// Variables for detection of long press onbuttons
bool memoLongPressActive = false;
unsigned long memoPressStartTime = 0;
const unsigned long longPressDuration = 2000; // 2 seconden

void setup() {
  Serial.begin(9600);
  SPI.begin();
  pot.writeValue(0);
  pinMode(mosfetGate, OUTPUT);
  digitalWrite(mosfetGate, LOW); // Ensure mosfet starts in OFF state
}

void loop() {
  int value2 = analogRead(pin2);
  int value4 = analogRead(pin4);

  // Converting to voltage 
  float voltage2 = value2 * (5.0 / 1023.0); // Not happy with float, make int calculation
  float voltage4 = value4 * (5.0 / 1023.0); 

  // Calculating resistance, useful for serial monitoring
  float resistance2 = (voltage2 == 0) ? 0 : knownResistor * ((5.0 / voltage2) - 1.0);
  float resistance4 = (voltage4 == 0) ? 0 : knownResistor * ((5.0 / voltage4) - 1.0);

  // Tolerance of 10%
  float tol = 0.10;

  unsigned long currentTime = millis();

  // Buttons on pin 2
  if (resistance2 < 0 * (1 + tol)) {
    Serial.println("Pin 2: Volume Down");
    setPotentiometer(31); // Set Digital potentiometer to 24 kOhm
  } else if (resistance2 >= 120 * (1 - tol) && resistance2 <= 120 * (1 + tol)) {
    Serial.println("Pin 2: Volume Up");
    setPotentiometer(20); // Set Digital potentiometer to 16 kOhm
  } else if (resistance2 >= 340 * (1 - tol) && resistance2 <= 340 * (1 + tol)) {
    handleButtonPress("Pin 2: Skip", &lastPressTimePin2Skip, &waitingForSecondPressPin2Skip, currentTime);
  } else if (resistance2 >= 1020 * (1 - tol) && resistance2 <= 1020 * (1 + tol)) {
    handleButtonPress("Pin 2: Memo", &lastPressTimePin2Memo, &waitingForSecondPressPin2Memo, currentTime);
    handleLongPress("Pin 2: Memo", &memoPressStartTime, &memoLongPressActive, currentTime);
    setPotentiometer(255); // Placeholder value
  } else {
    memoLongPressActive = false;
    setPotentiometer(0);
  }

  // Buttons on pin 4
  if (resistance4 < 0 * (1 + tol)) {
    handleButtonPress("Pin 4: Mode", &lastPressTimePin4Mode, &waitingForSecondPressPin4Mode, currentTime);
    setPotentiometer(0); // Placeholder value
  } else if (resistance4 >= 120 * (1 - tol) && resistance4 <= 120 * (1 + tol)) {
    Serial.println("Pin 4: Previous Track");
    setPotentiometer(14); // Set Digital potentiometer to  11.25 kOhm
  } else if (resistance4 >= 340 * (1 - tol) && resistance4 <= 340 * (1 + tol)) {
    Serial.println("Pin 4: Next Track");
    setPotentiometer(10); // Set Digital potentiometer to 8 kOhm
  } else if (resistance4 >= 1020 * (1 - tol) && resistance4 <= 1020 * (1 + tol)) {
    handleButtonPress("Pin 4: Mute", &lastPressTimePin4Mute, &waitingForSecondPressPin4Mute, currentTime);
    setPotentiometer(4); // Set Digital potentiometer to 3.5 kOhm
  }
  else {
    setPotentiometer(0); // Set digital potentiometer to 0 ohm
  }

  delay(50); // Small delay for serial monitoring, to make reading easier
}

void handleButtonPress(String button, unsigned long* lastPressTime, bool* waitingForSecondPress, unsigned long currentTime) {
  if (*waitingForSecondPress) {
    if (currentTime - *lastPressTime <= doublePressDelay) {
      Serial.print(button);
      Serial.println(" Double Press Detected");
      if (button == "Pin 2: Skip") {
        // Double press on "Skip" button: Preset folder down
        digitalWrite(mosfetGate, HIGH); // Connect ring and sleeve
        setPotentiometer(14); // Set Digital potentiometer to 11.25 kOhm
      }
      *waitingForSecondPress = false;
    } else {
      *waitingForSecondPress = false;
    }
  } else {
    Serial.print(button);
    Serial.println(" Single Press Detected");
    if (button == "Pin 2: Skip") {
      // Single press on "skip" button: Preset folder up
      digitalWrite(mosfetGate, HIGH); // Connect ring and sleeve
      setPotentiometer(10); // Set Digital potentiometer to 8 kOhm
    }
    *waitingForSecondPress = true;
    *lastPressTime = currentTime;
  }
}

void handleLongPress(String button, unsigned long* pressStartTime, bool* longPressActive, unsigned long currentTime) {
  if (!*longPressActive) {
    *pressStartTime = currentTime;
    *longPressActive = true;
  } else if (*longPressActive && (currentTime - *pressStartTime >= longPressDuration)) {
    Serial.print(button);
    Serial.println(" Long Press Detected");
    *longPressActive = false; // Reset long press state
  }
}

void setPotentiometer(uint8_t value) {
  pot.writeValue(value); // Set value of digital potentiometer
}
