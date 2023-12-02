//Included Libraries
#include "BluetoothA2DPSink.h"

#include <analogWrite.h>
#include <ESP32MX1508.h>


// Note: to avoid clipping, keep laptop volume at around 50
// Motor A: Body motor -> Yellow wires, IN1 to GPIO 16, IN2 to GPIO 17 ->  White to black, green to green
// Motor B: Mouth motor -> Blue wires, IN3 to GPIO 18, IN4 to GPIO 19 -> White to black, green to brown

#define DANCE_BUTTON 22



const int IN1 = 16;
const int IN2 = 17;
const int IN3 = 18;
const int IN4 = 19;

boolean boredAlready = false;
boolean dancing = false;

//Volume threshold for mouth being open
int threshold = 130;

// Loop counter for closing mouth completely
int loopCounter = 0;

//State of Billy 
int billystate = 0;

// Two extra stored values to check for outliers 
int prev = 0;
int twoBack = 0;

// Grace period for opening/ closing the mouth
boolean delaying = true;
int closedDelayTime = 300;
int openDelayTime = 380;

// "Sleep" time before Billy starts to look at you for instruction
int sleepDelay = 10000;
int sleepTime = millis();

int startTimeoutTime = sleepTime;
int buttonTime = sleepTime;
int currentTime = startTimeoutTime;

int buttonStatus = 0;

MX1508 bodyMotor(IN1, IN2);
MX1508 mouthMotor(IN3, IN4);

BluetoothA2DPSink a2dp_sink;


void setup() {

    //Bluetooth connection
    Serial.begin(115200);
    pinMode(DANCE_BUTTON, INPUT_PULLUP);



    // Start Bluetooth connection
    static const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100, // corrected by info from bluetooth
        .bits_per_sample = (i2s_bits_per_sample_t) 16, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

   
    a2dp_sink.set_stream_reader(read_data_stream);
    a2dp_sink.set_i2s_config(i2s_config);
    a2dp_sink.start("Bluetooth Billy Bass");


    
    mouthMotor.setSpeed(0);
    bodyMotor.setSpeed(0);

    }

void loop() {
    currentTime = millis();
    int danceStatus = digitalRead(DANCE_BUTTON);
    /*if (danceStatus != buttonStatus && finishedDelay(buttonTime, 500)){
      buttonStatus = danceStatus;
      Serial.println("Button pressed");
      buttonTime = millis();
    }*/
    if (!boredAlready && finishedDelay(sleepTime, sleepDelay)){
      // Billy looks up as if asking you to play something
      faceUp();
      delay(3000);
      faceDown();
      delay(200);
      billystate = 0; // go back to listening for sound
      boredAlready = true;
    }
}


// Then somewhere in your sketch:
void read_data_stream(const uint8_t *data, uint32_t length)
{
    // Update current time
    currentTime = millis();
    // Retrieve bluetooth data
    int16_t *samples = (int16_t*) data;
    uint32_t sample_count = length/2;
    // Retrieve sound data
    //Serial.println(*(data));
    // Determine course of action based on sound
    determineMovement(*(data));
  
    // Update the two stored values 
    twoBack = prev;
    prev = *(data);
}

void determineMovement(int soundVal){

  int recentAvg = ((soundVal + prev + twoBack)/3);

  switch (billystate){
    case 0: // Starting state (default)
      if (recentAvg > threshold){
        openMouth();
        billystate = 2;
        startTimeoutTime = millis();
      }else if (loopCounter < 4){
        closeMouth();
        loopCounter++;
      }else{
        billystate = 1;
        loopCounter = 0;
        // start a brief timeout
        startTimeoutTime = millis();
      }
      break;
    case 1: // Delay for when the mouth is closed (shorter)
      sleepTime = millis();
      if (finishedDelay(startTimeoutTime, closedDelayTime)){
          billystate = 0; 
      }
      break;

    case 2: // Delay for when mouth is open (longer)
      sleepTime = millis();
      if (finishedDelay(startTimeoutTime, openDelayTime)){
          billystate = 0;
      }
      break;
  }
}


bool finishedDelay(int startTime, int ms){
  return (currentTime >= (startTime + ms));
}

void openMouth() {
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(255); //set the mouth motor speed
  mouthMotor.forward(); //open the mouth
}

void closeMouth() {
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(255); //set the mouth motor speed
  //mouthMotor.halt();
  mouthMotor.backward();
}

void faceUp(){
  bodyMotor.halt();
  bodyMotor.setSpeed(255);
  bodyMotor.forward();
  bodyMotor.setSpeed(0);
}

void faceDown(){
  bodyMotor.halt();
  bodyMotor.setSpeed(255);
  bodyMotor.backward();
  delay(200);
  bodyMotor.halt();
}

void tailUp(){
  bodyMotor.halt();
  bodyMotor.setSpeed(255);
  bodyMotor.backward();
}

void tailDown(){
  bodyMotor.halt();
  bodyMotor.setSpeed(0);
  bodyMotor.forward();
  delay(200);
  bodyMotor.halt();
}
