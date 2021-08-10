#ifndef MEASURE_H
#define MEASURE_H

#include <Arduino.h>
#include "wbf_data.h"

#include <time.h>       

// ======== DEFINES ================


// ======== GLOBAL VARIABLES ============= 

//
// ======== ISR ==========================
// Interrupt to calculate distance of a sonar sensor type HC-SR04
// Called every µs, to have more accuracy.
// When startMeasure bit is true, start measuring.
// First create a triggerpuls of 10µs at the trigger pin.
// Than wait for answer at the echo pin, but not longer than 2000µs.
// When echo pin answers (true), count the pulswidth, which is time of flight.
// When echo pin low, puls is complete, calculate the distance in mm. 
// This is done with the speed of sound, which would be 340m/s = 340*1000mm/1e6µs = 0.34mm/µs.
// distance*2(mm)=duration(µs) *0.34(mm/µs). Duration is calculated in µs.
// distance*2 because the sonar pulse travels to the object and back.
// Every interrupt tick is 1µs.
//
void onRadarMeasure() 
{
  static volatile long pulsLength=0L;
  static volatile tISRMeasureStep ISRMeasureStep=isrMsIdle;
  switch(ISRMeasureStep)
  {
    case isrMsIdle:
      //wait for startMeasure bit.
      if(data.startMeasure)
      {
        //Reset pulslength, set trigger output, and go to next step.
        pulsLength=0L;
        digitalWrite(PIN_SONIC_TRIG,true);
        ISRMeasureStep=isrMsTriggering;
      }
      break;
    case isrMsTriggering:
      //Count 10 pulses (µs's)
      pulsLength++;
      if(pulsLength==10)
      {
        digitalWrite(PIN_SONIC_TRIG,false);
        pulsLength=0L;
        ISRMeasureStep=isrMsWaitingEcho;
      }
      break;
    case isrMsWaitingEcho:
      //Count pulses to timeOut(µs)
      pulsLength++;
      if(digitalRead(PIN_SONIC_ECHO))
      {
        pulsLength=0;
        ISRMeasureStep=isrMsMeasuringEcho;
      }
      //0.034cm/µs or 340 m/s 
      //distance=duration*0.034/2 in cm. Ignore above 30cm.
      //duration would than be 30/0.034*2 = 1765, use 1990 to be able to measure every 2000µs (2ms)
      if(pulsLength>1990)
      {
        ISRMeasureStep=isrMsAnswer;
      }
      break;
    case isrMsMeasuringEcho:
      pulsLength++;
      if(!digitalRead(PIN_SONIC_ECHO))
      {
        ISRMeasureStep=isrMsAnswer;
      }
      break;
    case isrMsAnswer:
      portENTER_CRITICAL_ISR(&dataAccessMux);
      data.radarDistanceMm = pulsLength>2000 ? -1.0 : pulsLength*0.343/2;
      data.startMeasure=false;
      portEXIT_CRITICAL_ISR(&dataAccessMux); 
      ISRMeasureStep=isrMsIdle;
      break;
  }//switch
}//ISR

void taskMeasure(void * parameter );

void setupMeasure() 
{
  Serial.printf("Create measure task\n");
  xTaskCreatePinnedToCore(
    taskMeasure,            // The function containing the task
    "TaskMeasure",          // Name for mortals
    16000,                  // Stack size 
    NULL,                   // Parameters
    2,                      // Priority, 0=lowest, 3=highest
    NULL,                   // Task handle
    ARDUINO_RUNNING_CORE);  // Core to run the task on
}

//
// The measure task looks at the position of the sweep, starts the measurement at every
// position of the stepper, and stores the answer in an array. 
// It keeps track of an index that shows the
// position
//
void taskMeasure(void * parameter )
{
  tMeasureStep measureStep=msIdle;
  long prevSweepPosition=-1;
  while(true) 
  {
    
    switch(measureStep) 
    {
      case msIdle:
        // Check if sweepposition has changed
        // If so, then start a distance measurement.
        // 
        portENTER_CRITICAL(&dataAccessMux);
          if(data.sweepPosition!=prevSweepPosition)
          {
            //start measurement when position is changed
            data.startMeasure=true; 
            prevSweepPosition=data.sweepPosition;
            measureStep=msWaitAnswer;
          }
        portEXIT_CRITICAL(&dataAccessMux);
        break;
      case msWaitAnswer:
        // If answer is received, 
        if(!data.startMeasure)
        {
          portENTER_CRITICAL(&dataAccessMux);
            data.radarData[prevSweepPosition].prev.distance=data.radarData[prevSweepPosition].cur.distance;
            data.radarData[prevSweepPosition].prev.screenX=data.radarData[prevSweepPosition].cur.screenX;
            data.radarData[prevSweepPosition].prev.screenY=data.radarData[prevSweepPosition].cur.screenY;
            data.radarData[prevSweepPosition].cur.distance=data.radarDistanceMm;
            data.radarData[prevSweepPosition].cur.screenX=0;
            data.radarData[prevSweepPosition].cur.screenY=0;
            data.radarData[prevSweepPosition].processed=false;
          portEXIT_CRITICAL(&dataAccessMux);
          measureStep=msIdle;
        }
        break;
    }
      
    vTaskDelay(1);

  } // while true
}


#endif
