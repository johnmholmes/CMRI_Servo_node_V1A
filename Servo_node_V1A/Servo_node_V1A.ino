#include <CMRI.h>    // Include the CMRI library
#include <Servo.h>   // Include the Servo library
#include <Auto485.h> // For 3-wire RS485 module communication
#include <EEPROM.h>  // Include the EEPROM library

// Sketch version
#define SKETCH_VERSION "Servo Node v1A 23 Nov 2024 J Holmes"

// Pin and address definitions
#define DE_PIN 2            // Pin for RS485 direction control
#define CMRI_ADDR 3         // Node address; must be unique for each node

// EEPROM addresses for turnout positions
#define EEPROM_CLOSED_POS_ADDR 0
#define EEPROM_THROWN_POS_ADDR 1
int eepromDelay = 3000;

// Timer for servo movement
unsigned long turnoutMoveTimer = 0;

// Struct for turnout-related data
struct Turnout {
  Servo servo;          // Servo instance
  int closedPos;        // Closed position
  int thrownPos;        // Thrown position
  byte position;        // Current servo position
  byte target;          // Target servo position
  int state;            // 0 = closed, 1 = thrown
  int speed;            // Speed of movement in milliseconds
};

// Create instances for CMRI and Turnout
Auto485 bus(DE_PIN);                    // For 3-wire RS485 module
CMRI cmri(CMRI_ADDR, 24, 48, bus);      // CMRI instance
Turnout turnout;                        // Turnout struct instance

void setup() {
  delay(2000); // Startup delay to avoid simultaneous node initialization

  // Start Serial for debugging (if connected)
  Serial.begin(9600);
  while (!Serial) {
    // Wait for the serial connection to be established (optional)
  }

  // Print sketch version
  Serial.println("Sketch Information:");
  Serial.print("Version: ");
  Serial.println(SKETCH_VERSION);
  Serial.println();

  // Read initial values for turnout positions from EEPROM
  turnout.closedPos = EEPROM.read(EEPROM_CLOSED_POS_ADDR);
  turnout.thrownPos = EEPROM.read(EEPROM_THROWN_POS_ADDR);

  // Set default values if EEPROM is uninitialized (255)
  if (turnout.closedPos == 255) turnout.closedPos = 85; // Default closed position
  if (turnout.thrownPos == 255) turnout.thrownPos = 95; // Default thrown position

  // Constrain to valid range (60 to 140)
  turnout.closedPos = constrain(turnout.closedPos, 60, 140);
  turnout.thrownPos = constrain(turnout.thrownPos, 60, 140);

  // Initialize the speed for the turnout movement
  turnout.speed = 10; // Default speed in milliseconds

  // Print the saved settings to Serial Monitor
  Serial.println("EEPROM Settings:");
  Serial.print("Turnout Closed Position: ");
  Serial.println(turnout.closedPos);
  Serial.print("Turnout Thrown Position: ");
  Serial.println(turnout.thrownPos);
  Serial.print("Turnout Speed: ");
  Serial.println(turnout.speed);

  // Initialize the servo and set it to the midpoint position
  int midpoint = (turnout.closedPos + turnout.thrownPos) / 2;
  Serial.print("Starting at midpoint: ");
  Serial.println(midpoint);
  turnout.servo.attach(3);
  turnout.servo.write(midpoint);
  turnout.position = midpoint; // Sync the position to midpoint
  turnout.target = midpoint;   // Set target to midpoint initially
  turnout.state = 0;           // Initialize turnout state
  delay(5000); // Allow the servo to stabilize

  // Initialize communication for the 3-wire RS485 module
  bus.begin(115200);
}

// Function to handle turnout activation
void activateTurnout(int targetPosition, int newState) {
  turnout.servo.attach(3);              // Reattach servo for movement
  turnout.target = targetPosition;      // Set the new target position
  turnout.state = newState;             // Update turnout state
  cmri.set_bit(0, newState);            // Update CMRI bit for JMRI feedback
}

// Function to update the servo position incrementally
void updateServoPosition() {
  if (turnout.position < turnout.target) turnout.position++;
  if (turnout.position > turnout.target) turnout.position--;
  turnout.servo.write(turnout.position);
}

// Function to handle position adjustments via CMRI
void handlePositionAdjustments() {
  bool adjustClosedUp = cmri.get_bit(1);   // Increment closed position
  bool adjustClosedDown = cmri.get_bit(2); // Decrement closed position
  bool adjustThrownUp = cmri.get_bit(3);   // Increment thrown position
  bool adjustThrownDown = cmri.get_bit(4); // Decrement thrown position

  // Temporary position to hold adjustments
  int tempPosition = turnout.position;

  // Adjust the closed position
  if (adjustClosedUp && turnout.closedPos < 140) {
    turnout.closedPos++;
    EEPROM.write(EEPROM_CLOSED_POS_ADDR, turnout.closedPos);
    Serial.print("Adjusted Closed Position: ");
    Serial.println(turnout.closedPos);
    tempPosition = turnout.closedPos; // Set temp position for servo movement
  }
  if (adjustClosedDown && turnout.closedPos > 60) {
    turnout.closedPos--;
    EEPROM.write(EEPROM_CLOSED_POS_ADDR, turnout.closedPos);
    Serial.print("Adjusted Closed Position: ");
    Serial.println(turnout.closedPos);
    tempPosition = turnout.closedPos; // Set temp position for servo movement
  }

  // Adjust the thrown position
  if (adjustThrownUp && turnout.thrownPos < 140) {
    turnout.thrownPos++;
    EEPROM.write(EEPROM_THROWN_POS_ADDR, turnout.thrownPos);
    Serial.print("Adjusted Thrown Position: ");
    Serial.println(turnout.thrownPos);
    tempPosition = turnout.thrownPos; // Set temp position for servo movement
  }
  if (adjustThrownDown && turnout.thrownPos > 60) {
    turnout.thrownPos--;
    EEPROM.write(EEPROM_THROWN_POS_ADDR, turnout.thrownPos);
    Serial.print("Adjusted Thrown Position: ");
    Serial.println(turnout.thrownPos);
    tempPosition = turnout.thrownPos; // Set temp position for servo movement
  }

  // Move the servo to the temporary adjusted position
  if (tempPosition != turnout.position) {
    turnout.servo.attach(3);
    turnout.servo.write(tempPosition);
    delay(500); // Allow the servo to stabilize
    turnout.servo.detach(); // Detach after moving to save power
  }
}

void loop() {
  cmri.process(); // Handle data communication with JMRI

  // Read turnout signal (0 = closed, 1 = thrown)
  int turnoutSignal = cmri.get_bit(0);

  // Check if turnout state needs to change (only on new CMRI signal)
  if (turnoutSignal == 1 && turnout.state == 0) {
    activateTurnout(turnout.thrownPos, 1); // Move to thrown position
  } else if (turnoutSignal == 0 && turnout.state == 1) {
    activateTurnout(turnout.closedPos, 0); // Move to closed position
  }

  // Detach the servo once it reaches the target position
  if (turnout.position == turnout.target) {
    turnout.servo.detach();
  }

  // Move the servo incrementally toward the target position
  if (turnout.position != turnout.target && millis() > turnoutMoveTimer) {
    turnoutMoveTimer = millis() + turnout.speed;
    updateServoPosition();
  }

  // Handle position adjustments via CMRI (manual adjustments)
  handlePositionAdjustments();
}
