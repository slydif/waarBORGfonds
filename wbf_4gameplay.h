#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// ======== INCLUDES =====================
#include <Arduino.h>
#include "wbf_data.h"

// ======== DEFINES ======================

// ======== GLOBAL VARIABLES ============= 
void taskGameplay(void * parameter);

void setupGameplay() 
{
  Serial.printf("Create gameplay task\n");
  xTaskCreatePinnedToCore(
    taskGameplay,               // The function containing the task
    "taskGameplay",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    2,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on
}

void taskGameplay(void *parameter) 
{
  tGameplayStep gameStep=gpIdle;
  while(true) 
  {
    switch(gameStep)
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
     }
 

    // Blocking stepper functions can be used.
    vTaskDelay(100);
  } // while
}

#endif
