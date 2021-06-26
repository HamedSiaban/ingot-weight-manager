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
const int sensor_left_pin = A8;
const int sensor_right_pin = A9;

// sensor input values
int sensor_left_currentState = 1;
int sensor_left_previousState = 1;
int sensor_right_currentState = 1;
int sensor_right_previousState = 1;

// these manage what our sensors are watching over a 4-cycle period
char sensorTriggerHistory[] = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
int sensorTriggerHistory_pointer = 0;
int sensorTiggerHistory_pattern = PATTERN_EMPTY;

// these manage serial2 data received from bascule
String basculeSerialDataInput = "";
int lastBasculeNumber = 0;
int previousWeight = 0;
int currentWeight = 0;

int weightQueue[3] = {};
int weightQueue_size = 3;
int weightQueue_pointer = 0;

// flags
bool isEdgeDetected = false;
bool isBasculeDataReachedNewLine = false;
bool isWeightQueueFull = false;
bool isLoggerOn = true;
/////////////////////////////////////////////////

///////////////////////////////////////////////////////////// helper functions
//prints logs to the serial terminal
void log(String message , int priority){
  //TODO some work with priority of the messages
  if(isLoggerOn)
    Serial.println(message);
}
void logln(String message)
{
  if (isLoggerOn)
    Serial.println(message);
}
void logln(int message)
{
  if (isLoggerOn)
    Serial.println(message);
}
void log(String message)
{
  if (isLoggerOn)
    Serial.print(message);
}
void log(int message)
{
  if (isLoggerOn)
    Serial.print(message);
}
/////////////////////////////////////////////////

///////////////////////////////////////////////////////////// assigning functions that handle our sensorTriggerHistory
// when system detects any edge, this function will be called 
// in order to check for any valid patterns
void updateSensorTriggerHistory(char edge){  //*** system switched to active low. so RISE and FALL values are inverted.
  sensorTriggerHistory[sensorTriggerHistory_pointer] = edge;
  
  if(sensorTiggerHistory_pattern == PATTERN_EMPTY){ // wich means sensorTriggerHistory_pointer = 0
    if(edge == LEFT_FALL){ // L detected
      logln("L detected");
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVING_FORWARD;
    } else if(edge == RIGHT_FALL){ // R detected
      logln("R detected");
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVING_BACKWARD;
    } else { // invalid pattern detected
      logln("INVALID detected");
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }

  if(sensorTiggerHistory_pattern == PATTERN_CARGO_MOVING_FORWARD){ // wich means sensorTriggerHistory_pointer = 1,2, or 3
    if(sensorTriggerHistory_pointer == 1 && edge == RIGHT_FALL){ // LR detected
      logln("LR detected");
    }
    else if(sensorTriggerHistory_pointer == 2 && edge == LEFT_RISE){ // LRl detected
      logln("LRl detected");
    }
    else if(sensorTriggerHistory_pointer == 3 && edge == RIGHT_RISE){ // LRlr detected
      logln("LRlr detected");
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVED_FORWARD;
    } else { // invalid pattern detected
      logln("INVALID detected");
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }

  if(sensorTiggerHistory_pattern == PATTERN_CARGO_MOVING_BACKWARD){ // wich means sensorTriggerHistory_pointer = 1,2, or 3
    if(sensorTriggerHistory_pointer == 1 && edge == LEFT_FALL){ // RL detected
      logln("RL detected");
    }
    else if(sensorTriggerHistory_pointer == 2 && edge == RIGHT_RISE){ // RLr detected
      logln("RLr detected");
    }
    else if(sensorTriggerHistory_pointer == 3 && edge == LEFT_RISE){ // RLrl detected
      logln("RLrl detected");
      sensorTiggerHistory_pattern = PATTERN_CARGO_MOVED_BACKWARD;
    } else { // invalid pattern detected
      logln("INVALID detected");
      sensorTiggerHistory_pattern = PATTERN_INVALID;
    }
    return;
  }
  
}

// resets everything that works with pattern detecting
void resetSensorTriggerHistory(){
  memset(sensorTriggerHistory, UNDEFINED, sizeof(sensorTriggerHistory)); // fills the array with Undefined
  sensorTriggerHistory_pointer = 0;
  sensorTiggerHistory_pattern = PATTERN_EMPTY;
}
///////////////////////////////////////////////////

/////////////////////////////////////////////////////////// arduino setup and loop phases
void setup(){
  // initializing sensor and led pins
  pinMode(sensor_left_pin, INPUT);
  pinMode(sensor_right_pin, INPUT);

  // initialize serial communication
  Serial.begin(9600); // debugger pc
  Serial2.begin(9600);  // bascule
}

void loop(){


  // read the sensors input pin
  sensor_left_currentState = digitalRead(sensor_left_pin);
  sensor_right_currentState = digitalRead(sensor_right_pin);


  // starting edge detection
  if(sensor_left_currentState != sensor_left_previousState){ // left sensor detected a change
  isEdgeDetected = true;
  sensor_left_previousState = sensor_left_currentState;

    if(sensor_left_currentState == HIGH){ // rising edge detected from the left sensor
      updateSensorTriggerHistory(LEFT_RISE);
    } else { // sensor_left_currentState == LOW _ falling edge detected from the left sensor
      updateSensorTriggerHistory(LEFT_FALL);
    }
  }

  if(sensor_right_currentState != sensor_right_previousState){ // right sensor detected a change
  isEdgeDetected = true;
  sensor_right_previousState = sensor_right_currentState;

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
        //get current weight and push previous non-zero weight into weight queue
        currentWeight = lastBasculeNumber;
        if(previousWeight != 0){ // if previousWeight==0 it means it's either the first weight or we had a cargo moved backward before this.
          if (isWeightQueueFull){ // if the flag is on, send the weight to pc because it will be overwritten.
            log("***this weight sent to the pc:  ");
            logln(weightQueue[weightQueue_pointer]);
          }
          weightQueue[weightQueue_pointer] = previousWeight;
          if(weightQueue_pointer == weightQueue_size-1){ // the queue is full. move pointer to 0 and turn the flag on.
            weightQueue_pointer = 0;
            isWeightQueueFull = true;     // this flag will make sure that from now on, we send the head weight to the pc with each enqueu.
          } else {
            weightQueue_pointer++;
          }
        }
        previousWeight = currentWeight;
        resetSensorTriggerHistory();
        break;
      case PATTERN_CARGO_MOVED_BACKWARD:
        //nullify the previous weight so when the cargo moves forward again, it won't enque
        previousWeight = 0;
        resetSensorTriggerHistory();
        break;
      case PATTERN_INVALID:
        resetSensorTriggerHistory();
        break;
    }
    


    log("last bascule number: ");
    logln(lastBasculeNumber);
    log("previous weight: ");
    logln(previousWeight);
    log("current weight: ");
    logln(currentWeight);
    log("weight queue: ");
    for(int i=0; i<3; i++){
      log(weightQueue[i]);
      log("  ,  ");
    }
    logln("");

    
   
    isEdgeDetected = false;
    delay(10);
  }

  if (isBasculeDataReachedNewLine){
    //log("rached new line");
    lastBasculeNumber = basculeSerialDataInput.toInt();
    //Serial.println(lastBasculeNumber);

    //log(basculeSerialDataInput);
    // clear the string:
    basculeSerialDataInput = "";
    isBasculeDataReachedNewLine = false;
  }
}
////////////////////////////////////////////////

void serialEvent2() { // serial data received from the bascule
  while (Serial2.available()) {
    // get the new byte:
    char inputCharacter = (char)Serial2.read();
    // add it to the inputString:
    basculeSerialDataInput += inputCharacter;
    // if the incoming character is a newline, set a flag so the main loop can do something about it:
    if (inputCharacter == '\n') {
      isBasculeDataReachedNewLine = true;
    }
  }
}
////////////////////////////////////////////////
