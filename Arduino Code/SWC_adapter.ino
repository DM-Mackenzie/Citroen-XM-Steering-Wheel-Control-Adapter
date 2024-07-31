#include <MCP4151.h> // This project uses the MCP4151 IC, so also its library
#include <SPI.h> // SPI, or Serial Peripheral Interface; Necessary for communication with the MCP4151

// Introduction of RadioProfile in which various buttons are declared as variables.
// These are specific to the buttons on the XM steering wheel. Extra variables can be added for a double press or longpress, on any button you desire. 
struct RadioProfile {
  const char* name;
  uint8_t volDown;
  uint8_t volUp;
  uint8_t skipSingle;
  uint8_t skipDouble;
  uint8_t memo;
  uint8_t memoLong;
  uint8_t mode;
  uint8_t prevTrack;
  uint8_t nextTrack;
  uint8_t mute;
};

/* 
Create the profiles themselves. For now, these are Pioneer and Sony. These are saved as arrays, in which Arduino code can easily select them.
See the structure above to see in which order you should input these. 
Calculating the steps required for the resistance you want:
The digipot has 255 steps between 0 and 100k Ohm. To calculate the necessary steps for (let's say) 62000 ohm, calculate as follows:
Necessary steps = (255 * 62000) / 100000 = 158,1 -> 158 steps on the potentiometer.
*/

RadioProfile profiles[] = {
  {"Pioneer", 31, 20, 10, 14, 255, 255, 0, 14, 10, 4}, // Values for Pioneer, mostly placeholders, not yet correct
  {"Sony", 65, 50, 35, 25, 19, 14, 2, 4, 6, 8} // Values for Sony, mostly placeholders, not yet correct
};

int currentProfile = 0; // Index of the current profile

// Create function to change profiles. 
void switchProfile(int profileIndex) {
  if (profileIndex < 0 || profileIndex >= (sizeof(profiles) / sizeof(profiles[0]))) {
    Serial.println("Invalid profile index");
    return;
  }
  currentProfile = profileIndex;
  Serial.print("Switched to profile: ");
  Serial.println(profiles[currentProfile].name);
}

// Arduino pin assignments
const int pin2 = A0; // Pin 2 of SWC connector
const int pin4 = A1; // Pin 4 of SWC connector
const float knownResistor = 1000.0; // 1k ohm reference resistor

// MCP4151 settings
const int csPin = 10; // Chip Select pin for MCP4151
const int mosiPin = 11; // Mosi Select pin for MCP4151
const int misoPin = 12; // Miso Select pin for MCP4151
const int sckPin = 13;  // Miso Select pin for MCP4151
MCP4151 pot(csPin, mosiPin, misoPin, sckPin); // assign pins through MCP4151 library function "pot"

const int mosfetGate = 9; // MOSFET pin, to enable extra functions for Pioneer headunits (ring-sleeve connection)

// Time related variables for double press detection
unsigned long lastPressTimePin2Skip = 0;
unsigned long lastPressTimePin2Memo = 0;
unsigned long lastPressTimePin4Mode = 0;
unsigned long lastPressTimePin4Mute = 0;

bool waitingForSecondPressPin2Skip = false;
bool waitingForSecondPressPin2Memo = false;
bool waitingForSecondPressPin4Mode = false;
bool waitingForSecondPressPin4Mute = false;

const unsigned long doublePressDelay = 500; // 500 milliseconds

// Variables for detection of long press
bool memoLongPressActive = false;
unsigned long memoPressStartTime = 0;
const unsigned long longPressDuration = 2000; // 2 seconds

void setPotentiometer(uint8_t value) {
  pot.writeValue(value); // set potentiometer write value with variable "value"
}

// Setup, code that is to be run once at the initialization of the microcontroller
void setup() {
  Serial.begin(9600); // enable serial monitor for debugging
  SPI.begin(); // Start SPI communication with MCP4151
  pot.writeValue(0); // start digital potentiometer with 0 ohm resistance
  pinMode(mosfetGate, OUTPUT); // set mosfetGate as an output
  digitalWrite(mosfetGate, LOW); // Ensure MOSFET is off when started

  // Enable the resistance profile
  switchProfile(currentProfile);
}

// Loop, code that will run continuously in a loop infinitely
void loop() {
  int value2 = analogRead(pin2); // Simple analogRead from pin 2
  int value4 = analogRead(pin4); // Simple analogRead from pin 4

  // Calculate the analogread to a voltage
  float voltage2 = value2 * (5.0 / 1023.0); // Float, must become int for quicker and less heavy calculations.
  float voltage4 = value4 * (5.0 / 1023.0); 

  // Calculate resistance, useful for monitoring through serial monitor
  float resistance2 = (voltage2 == 0) ? 0 : knownResistor * ((5.0 / voltage2) - 1.0);
  float resistance4 = (voltage4 == 0) ? 0 : knownResistor * ((5.0 / voltage4) - 1.0);

  // 10% tolerance
  float tol = 0.10;

  unsigned long currentTime = millis();

  // Load current profile
  RadioProfile profile = profiles[currentProfile];

  // Buttons on pin 2
  if (resistance2 < 0 * (1 + tol)) {
    Serial.println("Pin 2: Volume Down");
    setPotentiometer(profile.volDown); // set potentiometer value with the profile value
  } else if (resistance2 >= 120 * (1 - tol) && resistance2 <= 120 * (1 + tol)) {
    Serial.println("Pin 2: Volume Up");
    setPotentiometer(profile.volUp); // set potentiometer value with the profile value
  } else if (resistance2 >= 340 * (1 - tol) && resistance2 <= 340 * (1 + tol)) {
    handleButtonPress("Pin 2: Skip", &lastPressTimePin2Skip, &waitingForSecondPressPin2Skip, currentTime, profile);
  } else if (resistance2 >= 1020 * (1 - tol) && resistance2 <= 1020 * (1 + tol)) {
    handleButtonPress("Pin 2: Memo", &lastPressTimePin2Memo, &waitingForSecondPressPin2Memo, currentTime, profile);
    handleLongPress("Pin 2: Memo", &memoPressStartTime, &memoLongPressActive, currentTime, profile);
    setPotentiometer(profile.memo); // Placeholder value
  } else {
    memoLongPressActive = false;
    setPotentiometer(0);
  }

  // Buttons on pin 4
  if (resistance4 < 0 * (1 + tol)) {
    handleButtonPress("Pin 4: Mode", &lastPressTimePin4Mode, &waitingForSecondPressPin4Mode, currentTime, profile);
    setPotentiometer(profile.mode); // Placeholder value
  } else if (resistance4 >= 120 * (1 - tol) && resistance4 <= 120 * (1 + tol)) {
    Serial.println("Pin 4: Previous Track");
    setPotentiometer(profile.prevTrack); // set potentiometer value with the profile value
  } else if (resistance4 >= 340 * (1 - tol) && resistance4 <= 340 * (1 + tol)) {
    Serial.println("Pin 4: Next Track");
    setPotentiometer(profile.nextTrack); // set potentiometer value with the profile value
  } else if (resistance4 >= 1020 * (1 - tol) && resistance4 <= 1020 * (1 + tol)) {
    handleButtonPress("Pin 4: Mute", &lastPressTimePin4Mute, &waitingForSecondPressPin4Mute, currentTime, profile);
    setPotentiometer(profile.mute); //set potentiometer value with the profile value
  } else {
    setPotentiometer(0); // set potentiometer value to 0 when no button is pushed
  }

  delay(50); // small delay to make serial monitor more readable. Comment out when using this permanent
}

// Function to handle button presses. Can also detect single or double press.
void handleButtonPress(String button, unsigned long* lastPressTime, bool* waitingForSecondPress, unsigned long currentTime, RadioProfile profile) {
  if (*waitingForSecondPress) {
    if (currentTime - *lastPressTime <= doublePressDelay) {
      Serial.print(button);
      Serial.println(" Double Press Detected");
      if (button == "Pin 2: Skip") {
        // Double press on "skip" button: Preset folder down
        digitalWrite(mosfetGate, HIGH); // Connect ring and sleeve
        setPotentiometer(profile.skipDouble); // Set potentiometer value to the current profile's: skipDouble
      }
      *waitingForSecondPress = false;
    } else {
      *waitingForSecondPress = false;
    }
  } else {
    Serial.print(button);
    Serial.println(" Single Press Detected");
    if (button == "Pin 2: Skip") {
      // Single button press "skip"
      digitalWrite(mosfetGate, HIGH); // Connect ring and sleeve
      setPotentiometer(profile.skipSingle); // Set potentiometer value to the current profile's: skipSingle
    }
    *waitingForSecondPress = true;
    *lastPressTime = currentTime;
  }
}

// Function to detect a long press on a button. This can add additional functionality.
void handleLongPress(String button, unsigned long* pressStartTime, bool* longPressActive, unsigned long currentTime, RadioProfile profile) {
  if (!*longPressActive) {
    *pressStartTime = currentTime;
    *longPressActive = true;
  } else if (*longPressActive && (currentTime - *pressStartTime >= longPressDuration)) {
    Serial.print(button);
    Serial.println(" Long Press Detected");
    if (button == "Pin 2: Memo") {
      setPotentiometer(profile.memoLong); // Set potentiometer value to the current profile's: MemoLong
    }
    *longPressActive = false; // Reset long press status
  }
}
