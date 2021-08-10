#ifndef SWEEP_H
#define SWEEP_H

// ======== INCLUDES =====================
#include <Arduino.h>
#include <AccelStepper.h>
#include "wbf_data.h"
#include "wbf_pins.h"

// ======== DEFINES ======================

// ======== GLOBAL VARIABLES ============= 
AccelStepper stepper(AccelStepper::FULL4WIRE,PIN_STEPA,PIN_STEPB,PIN_STEPC,PIN_STEPD);

void taskSweep(void * parameter);

void setupSweep() 
{
  Serial.printf("Create sweep task\n");
  xTaskCreatePinnedToCore(
    taskSweep,               // The function containing the task
    "TaskKeyboard",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    2,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on
}

void taskSweep(void *parameter) 
{
  
  long leftPos,rightPos,nrSweeps=0;
  tSweepStep sweepStep=ssInit;
  while(true) 
  {
    switch(sweepStep)
    {
      case ssInit:
        portENTER_CRITICAL(&configAccessMux);
          stepper.setAcceleration(config.stepperAcceleration);
          stepper.setMaxSpeed(config.stepperMaxSpeed);
          leftPos=STEPPERLEFTPOS;
          rightPos=STEPPERRIGHTPOS;
        portEXIT_CRITICAL(&configAccessMux);
        stepper.moveTo(leftPos);
        sweepStep=ssForward;
        break;
      case ssForward:
        if(stepper.distanceToGo()==0)
        {
          stepper.moveTo(rightPos);
          sweepStep=ssBack;
        }
        break;
      case ssBack:
        if(stepper.distanceToGo()==0)
        {
          Serial.printf("Sweep %d\n",++nrSweeps);
          if(data.sweepReInit)
          {
            sweepStep=ssInit;
            data.sweepReInit=false;
          }else
          {
            stepper.moveTo(leftPos);
            sweepStep=ssForward;
          }
        }
        break;
    }
    stepper.run();
    portENTER_CRITICAL(&dataAccessMux);
      data.sweepPosition=stepper.currentPosition();
      data.sweepBackward=(sweepStep==ssBack);
    portEXIT_CRITICAL(&dataAccessMux);

    // Blocking stepper functions can be used.
    vTaskDelay(1);

  } // while
}

#endif
