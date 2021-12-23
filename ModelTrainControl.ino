#include "types.h"

const int PWM_PIN = 3;
const int PWM_50_PER_PIN = 11;
const int PLUS_PIN = 8;
const int MINUS_PIN = 9;
const int MAX_POWER = 62;
const int MIN_POWER = 0;  
const float MAX_PERCENT = 100.0f;
const float MIN_PERCENT = 0.0f;
const int CHAR_OFFSET = 48;
const unsigned long SPEED_CHANGE_INTERVAL = 250;

int currentPowerPercent = 0;
int requestedPowerPercent = 0;
track_direction_t trackDirection = TRACK_DIR_NONE;
track_direction_t requestedTrackDirection = TRACK_DIR_NONE;
request_status_t requestStatus = REQUEST_STATUS_NONE;
unsigned long triggerTime = 0;
int speedChangeValue = 0;
unsigned long cycleCount = 0;

void setup() 
{
  Serial.begin(9600); 
  pinMode(PWM_PIN, OUTPUT);
  pinMode(PWM_50_PER_PIN, OUTPUT);
  pinMode(PLUS_PIN, OUTPUT);
  pinMode(MINUS_PIN, OUTPUT);
  
  // Setup Pin 3 for PWM at 2KHz
  TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM20);
  TCCR2B = _BV(CS22);
  OCR2A = MAX_POWER;
  OCR2B = 0;
}

void loop() 
{
  //char buf[50];
  //sprintf(buf, "cycle: %d", cycleCount++);
  //Serial.println(buf);
  
  // Check for serial input
  int incomingByte = 0;
  if (Serial.available() > 0)
  {
    //get the incoming byte and process it
    incomingByte = Serial.read();
    char newByte = (char)incomingByte;
  
    if (newByte == 'X' || newByte == 'x')
    {
      Serial.println("x received, setting status to ready...");
      requestStatus = REQUEST_STATUS_READY;
    }
    else if (newByte == 's' || newByte == 'S')
    {
      Serial.println("s received, setting status to emergency stop...");
      requestStatus = REQUEST_STATUS_EMERGENCY_STOP;
    }
    else if (newByte >= '0' && newByte <= '9')
    {
      Serial.println("number received, setting status to none...");
      if (requestedPowerPercent > 0)
      {
        requestedPowerPercent *= 10;
      }
      requestedPowerPercent += newByte - CHAR_OFFSET;        
      requestStatus = REQUEST_STATUS_NONE;
    }
    else if (newByte == '+')
    {
      if (trackDirection != TRACK_DIR_TWO)
      {
        Serial.println("plus received, setting status to ready...");
        requestedPowerPercent = 0;
        requestedTrackDirection = TRACK_DIR_TWO;
        requestStatus = REQUEST_STATUS_READY;
      }
      else
      {
        Serial.println("plus received, already at DIR_TWO, setting status to none...");
        requestStatus = REQUEST_STATUS_NONE;
      }
    }
    else if (newByte == '-')
    {
      if (trackDirection != TRACK_DIR_ONE)
      {
        Serial.println("minus received, setting status to ready...");
        requestedPowerPercent = 0;
        requestedTrackDirection = TRACK_DIR_ONE;
        requestStatus = REQUEST_STATUS_READY;
      }
      else
      {
        Serial.println("minus received, already at DIR_ONE, setting status to none...");
        requestStatus = REQUEST_STATUS_NONE;
      }
    }
  }

  // Update output direction and power based on settings
  if (requestStatus == REQUEST_STATUS_READY)
  {
    Serial.println("Status: READY");
    // For now, just ramp up the power by 1 percent every 250 ms
    triggerTime = millis() + SPEED_CHANGE_INTERVAL;
    char buf[100];
    sprintf(buf, "reqPow: %d currPow: %d", requestedPowerPercent, currentPowerPercent);
    Serial.println(buf);
    
    if (requestedPowerPercent > currentPowerPercent)
    {
      Serial.println("positive speed change value...");
      speedChangeValue = 1;  
    }
    else
    {
      Serial.println("negative speed change value...");
      speedChangeValue = -1;
    }
    
    requestStatus = REQUEST_STATUS_IN_PROGRESS;
  }
  else if (requestStatus == REQUEST_STATUS_IN_PROGRESS)
  {
    Serial.println("Status: IN PROGRESS");
    unsigned long currentTime = millis();
    if (currentTime > triggerTime)
    {
      currentPowerPercent += speedChangeValue;
      setTrackPower(percentToPower(currentPowerPercent));
      triggerTime = currentTime + SPEED_CHANGE_INTERVAL;
    }

    // need to consider the sign of the speed change
    if (speedChangeValue > 0)
    {
      if (currentPowerPercent >= requestedPowerPercent || currentPowerPercent >= MAX_PERCENT)
      {
        requestStatus = REQUEST_STATUS_COMPLETE;
      }
    }
    else if (speedChangeValue < 0)
    {
      if (currentPowerPercent <= MIN_PERCENT || currentPowerPercent <= requestedPowerPercent)
      {
        requestStatus = REQUEST_STATUS_COMPLETE;
      }
    }
  }
  else if (requestStatus == REQUEST_STATUS_COMPLETE)
  {
    Serial.println("Status: COMPLETE");
    // Speed adjustment is complete, set direction as requested
    setTrackDirection(requestedTrackDirection);
    requestStatus = REQUEST_STATUS_NONE;
    requestedPowerPercent = 0;
  }
  else if (requestStatus == REQUEST_STATUS_EMERGENCY_STOP)
  {
    Serial.println("Status: EMERGENCY STOP");
    // Set power to 0 immediately
    setTrackPower(0);
    currentPowerPercent = 0;
    requestedTrackDirection = TRACK_DIR_NONE;
    requestStatus = REQUEST_STATUS_COMPLETE;
  }
  else if (requestStatus == REQUEST_STATUS_NONE)
  {
    //Serial.println("Status: NONE");
    // do nothing, just hang out until there is a request
  }
  else
  {
    //Serial.println("DEFAULT");
  }
}

// Convert percentage to a power output
int percentToPower(int percent)
{
  if (percent > MAX_PERCENT)
  {
    return MAX_POWER;
  }
  else if (percent < MIN_PERCENT)
  {
    return MIN_POWER;
  }
  else
  {
    return ((percent/100.0f) * MAX_POWER);
  }
}

void setTrackPower(int trackPower)
{
  int power = trackPower;
  if (power >= MAX_POWER)
  {
    power = MAX_POWER;
  }
  else if (power <= MIN_POWER)
  {
    power = MIN_POWER;
  }
  char buf[100];
  sprintf(buf, "Setting Power Pin to: %d...", power);
  Serial.println(buf);
  //analogWrite(PWM_PIN, power); 
  OCR2B = power;
}

void setTrackDirection(track_direction_t trackDir)
{
  switch(trackDir)
  {
    case TRACK_DIR_ONE:
      Serial.println("Setting direction to TRACK_DIR_ONE");     
      digitalWrite(PLUS_PIN, HIGH);
      digitalWrite(MINUS_PIN, LOW);
      break;
    case TRACK_DIR_TWO:
      Serial.println("Setting direction to TRACK_DIR_TWO");
      digitalWrite(PLUS_PIN, LOW);
      digitalWrite(MINUS_PIN, HIGH);
      break;
    case TRACK_DIR_NONE: // Intentional Fall-thru
    default:
      Serial.println("Setting direction to TRACK_DIR_NONE");
      digitalWrite(PLUS_PIN, LOW);
      digitalWrite(MINUS_PIN, LOW);
      break;
  };
}
