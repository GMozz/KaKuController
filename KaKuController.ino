
#include <NewRemoteReceiver.h>
#include <NewRemoteTransmitter.h>

const byte INTERRUPT_PIN = 0; //Interrupt 0 (= digital pin 2)
const byte TRANSMIT_PIN = 11;
const int MAX_ARRAY_SIZE = 4;

String tempString = "";
int arrayPosition = 0;
String input[MAX_ARRAY_SIZE] = {""};  // a string array to hold incoming data
boolean stringComplete = false;       // whether the string is complete

void setup() {
  //Initialize serial:
  Serial.begin(115200);
  //Reserve 200 bytes for the tempString:
  tempString.reserve(200);

  /*
   * Initialize receiver on interrupt 0 (= digital pin 2), calls the callback "showCode"
   * after 2 identical codes have been received in a row. (thus, keep the button pressed
   * for a moment)
   *
   * See the interrupt-parameter of attachInterrupt for possible values (and pins)
   * to connect the receiver.
   */
  NewRemoteReceiver::init(INTERRUPT_PIN, 2, showCode);
}

void loop() {
  //Wait until serialEvent() flags stringComplete
  if (stringComplete) {
    transmitCode(input);
    //Clear the input array and stringComplete flag
    input[MAX_ARRAY_SIZE] = {""};
    stringComplete = false;
  }
}

/**
 * SerialEvent occurs whenever a new data comes in the
 * hardware serial RX.  This routine is run between each
 * time loop() runs, so using delay inside loop can delay
 * response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  /*
   * Expecting strings in the following format:
   * Address;Unit;Period;On/Off
   * e.g. 9033309;2;253;1
   */
  while (Serial.available()) {
    //Get the new byte:
    char inChar = (char)Serial.read();

    
    if (inChar == ';') {
      //Add the content of the tempString to the input array
      input[arrayPosition] = tempString;
      tempString = "";

      //Safety check in case we get bad input
      if (arrayPosition < (MAX_ARRAY_SIZE - 1)) {
        arrayPosition++;
      }
    } else if (inChar == '\n') {
      //If the incoming character is a newline, set a flag
      //so the main loop can do something about it
      input[arrayPosition] = tempString;
      tempString = "";
      arrayPosition = 0;
      stringComplete = true;
    } else {
      //Add it to the tempString:
      tempString += inChar;
    }
  }
}

/**
 * Callback function is called only when a valid code is received.
 */
void showCode(NewRemoteCode receivedCode) {
  // Note: interrupts are disabled. You can re-enable them if needed.

  //Address - unique per remote
  Serial.print(receivedCode.address);

  //Unit - 0 based numbered button on the remote. Can be -1 when it's a group.
  if (receivedCode.groupBit) {
    Serial.print(";-1");
  } else {
    Serial.print(";");
    Serial.print(receivedCode.unit);
  }

  //Period in µs
  Serial.print(";");
  Serial.print(receivedCode.period);

  //Representation of on, off or dim level
  switch (receivedCode.switchType) {
    case NewRemoteCode::off:
      Serial.println(";0");
      break;
    case NewRemoteCode::on:
      Serial.println(";1");
      break;
    case NewRemoteCode::dim:
    Serial.print(";");
    Serial.println(receivedCode.dimLevel);
      break;
  }
}

/**
 * Expects a String array with the exact size of 4 (MAX_ARRAY_SIZE) items.
 * transmitCode[0] = 9033309 - Address of the remote
 * transmitCode[1] = 2       - Remote's unit, could be -1 when it's a group
 * transmitCode[2] = 253     - Period in µs
 * transmitCode[3] = 1       - On (1) or off (0)
 */
void transmitCode(String transmitCode[]) {
  //Just letting this stay here for debugging purposes
//  for (int i = 0; i < MAX_ARRAY_SIZE; i++) {
//    Serial.print("Position ");
//    Serial.print(i);
//    Serial.print(" = ");
//    Serial.println(transmitCode[i]);
//  }

  unsigned long address = transmitCode[0].toInt();
  int unit              = transmitCode[1].toInt();
  unsigned int period   = transmitCode[2].toInt();
  byte onOrOff          = transmitCode[3].toInt();
  
  //Disable the receiver; otherwise it might pick up the transmit
  NewRemoteReceiver::disable();
  NewRemoteTransmitter transmitter(address, TRANSMIT_PIN, period);

  bool isGroup = unit == -1;
  if (onOrOff > 1) {
    /*
     * Not quite sure how a dimmer should work, since I don't have one to test with,
     * so I'm just assuming here that it will provide an int which is higher than 1
     */
    if (isGroup) {
      transmitter.sendGroupDim(onOrOff);
    } 
    else {
      transmitter.sendDim(unit, onOrOff);
    }
  } 
  else {
    //On/Off signal received
    bool isOn = onOrOff == 1;
    
    if (isGroup) {
      transmitter.sendGroup(isOn);
    } else {
      // Send to a single unit
      transmitter.sendUnit(unit, isOn);
    }
  }

  NewRemoteReceiver::enable();
}

