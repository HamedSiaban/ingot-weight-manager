#include <Arduino.h>

//////////////////////////////////////////////////////// assigning static values for code reading efficiency
// defining what our system can detect via sensors _ inputs
static const char LEFT_RISE = 'L';
static const char LEFT_FALL = 'l';
static const char RIGHT_RISE = 'R';
static const char RIGHT_FALL = 'r';
static const char UNDEFINED = 'U';

// defining what state is our system in _ outputs 
static const int PATTERN_EMPTY = 0;
static const int PATTERN_CARGO_MOVING_FORWARD = 1;
static const int PATTERN_CARGO_MOVING_BACKWARD = 2;
static const int PATTERN_CARGO_MOVED_FORWARD = 10;
static const int PATTERN_CARGO_MOVED_BACKWARD = 11;
static const int PATTERN_INVALID = 100;
/////////////////////////////////////////////////

//////////////////////////////////////////////////////////// assigning variables
// defining arduino pins
const int sensor_left_pin = 1;
const int sensor_right_pin = 2;

// sensor input values
int sensor_left_currentState = 0;
int sensor_left_previousState = 0;
int sensor_right_currentState = 0;
int sensor_right_previousState = 0;

// these manage what our sensors are watching over a 4-cycle period
char sensorTriggerHistory[] = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
int sensorTriggerHistory_pointer = 0;
int sensorTiggerHistory_pattern = PATTERN_EMPTY;

// Serial data received from bascule
String serialDataInput = "";
int reservedStorageSize = 200;

// flags
bool isEdgeDetected = false;
bool isDataReachedNewLine = false;
/////////////////////////////////////////////////

///////////////////////////////////////////////////////////// assigning functions that handle our sensorTriggerHistory
// when system detects any edge, this function will be called 
// in order to check for any valid patterns
void updateSensorTriggerHistory(char edge){
  sensorTriggerHistory[sensorTriggerHistory_pointer] = edge;
  
  if(sensorTiggerHistory_pattern == PATTERN_EMPTY){ // wich means sensorTriggerHistory_pointer = 0
    if(edge == LEFT_RISE){ // L detected
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVING_FORWARD;
    } else if(edge == RIGHT_RISE){ // R detected
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVING_BACKWARD;
    } else { // invalid pattern detected
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }

  if(sensorTiggerHistory_pattern == PATTERN_CARGO_MOVING_FORWARD){ // wich means sensorTriggerHistory_pointer = 1,2, or 3
    if(sensorTriggerHistory_pointer == 1 && edge == RIGHT_RISE){ // LR detected
      Serial.println("LR detected");
    }
    else if(sensorTriggerHistory_pointer == 2 && edge == LEFT_FALL){ // LRl detected
    }
    else if(sensorTriggerHistory_pointer == 3 && edge == RIGHT_FALL){ // LRlr detected
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVED_FORWARD;
    } else { // invalid pattern detected
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }

  if(sensorTiggerHistory_pattern == PATTERN_CARGO_MOVING_BACKWARD){ // wich means sensorTriggerHistory_pointer = 1,2, or 3
    if(sensorTriggerHistory_pointer == 1 && edge == LEFT_RISE){ // RL detected
    }
    else if(sensorTriggerHistory_pointer == 2 && edge == RIGHT_FALL){ // RLr detected
    }
    else if(sensorTriggerHistory_pointer == 3 && edge == LEFT_FALL){ // RLrl detected
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVED_BACKWARD;
    } else { // invalid pattern detected
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }
  
}

// resets everything that works with pattern detecting
void resetSensorTriggerHistory(){
  char sensorTriggerHistory[] = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
  int sensorTriggerHistory_pointer = 0;
  int sensorTiggerHistory_pattern = PATTERN_EMPTY;
}
///////////////////////////////////////////////////

/////////////////////////////////////////////////////////// arduino setup and loop phases
void setup(){
  // initializing sensor and led pins
  pinMode(sensor_left_pin, INPUT);
  pinMode(sensor_right_pin, INPUT);

  // initialize serial communication
  Serial.begin(9600);
  // reserve 200 bytes for the serialDataInput
  serialDataInput.reserve(200);
}


void loop(){

  // print the string when a newline arrives:
  if (isDataReachedNewLine) {
    Serial.println(serialDataInput);
    // clear the string:
    serialDataInput = "";
    isDataReachedNewLine = false;
  }


  // read the sensors input pin
  sensor_left_currentState = digitalRead(sensor_left_pin);
  sensor_right_currentState = digitalRead(sensor_right_pin);


  // starting edge detection
  if(sensor_left_currentState != sensor_left_previousState){ // left sensor detected a change
  isEdgeDetected = true;

    if(sensor_left_currentState == HIGH){ // rising edge detected from the left sensor
      updateSensorTriggerHistory(LEFT_RISE);
    } else { // sensor_left_currentState == LOW _ falling edge detected from the left sensor
      updateSensorTriggerHistory(LEFT_FALL);
    }
  }

  if(sensor_right_currentState != sensor_right_previousState){ // right sensor detected a change
  isEdgeDetected = true;

    if(sensor_right_currentState == HIGH){ // rising edge detected from the right sensor
      updateSensorTriggerHistory(RIGHT_RISE);
    } else { // sensor_right_currentState == LOW _ falling edge detected from the right sensor
      updateSensorTriggerHistory(RIGHT_FALL);
    }
  }

  if(isEdgeDetected){ // we check patterns only when the system detects an edge
    switch(sensorTiggerHistory_pattern){
      case PATTERN_CARGO_MOVING_FORWARD:
        sensorTriggerHistory_pointer++;
        break;
      case PATTERN_CARGO_MOVING_BACKWARD:
        sensorTriggerHistory_pointer++;
        break;
      case PATTERN_CARGO_MOVED_FORWARD:
        //TODO get weight and push it into stack
        resetSensorTriggerHistory();
        break;
      case PATTERN_CARGO_MOVED_BACKWARD:
        //TODO pop the last weight value from stack
        resetSensorTriggerHistory();
        break;
      case PATTERN_INVALID:
        resetSensorTriggerHistory();
        break;
    }
    isEdgeDetected = false;
    delay(10);
  }

}
////////////////////////////////////////////////

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inputCharacter = (char)Serial.read();
    // add it to the inputString:
    serialDataInput += inputCharacter;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inputCharacter == '\n') {
      isDataReachedNewLine = true;
    }
  }
}