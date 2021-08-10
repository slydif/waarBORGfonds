#ifndef GAMEPLAY_H
#define GAMEPLAY_H

// ======== INCLUDES =====================
#include <Arduino.h>
#include "wbf_data.h"

// ======== DEFINES ======================
#define SONAR_NO_SEE_DISTANCE 300
#define SONAR_POSWINDOW 5

// ======== GLOBAL VARIABLES ============= 
void taskGameplay(void * parameter);
bool checkQuestionIsOk(int question);

void setupGameplay() 
{
  Serial.printf("Create gameplay task\n");
  xTaskCreatePinnedToCore(
    taskGameplay,               // The function containing the task
    "taskGameplay",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    1,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on
}

void taskGameplay(void *parameter) 
{
  int gameStep;
  
  
  while(true) 
  {
    switch(data.gameStep)
    {
      case gpIdle:
        if(data.btnPlayTouched)
        {
          portENTER_CRITICAL(&dataAccessMux);
            data.allAnswersOk=false;
            data.gameStep=gpQ1;
          portEXIT_CRITICAL(&dataAccessMux);
        }
        break;
      case gpQ1:
        if(data.btnOkTouched)
        {
          if(checkQuestionIsOk(1))
          data.gameStep=gpQ2;
        }
        break;
      case gpQ2:
        break;
    }
 
    if(data.btnPlayTouched)
    {
      Serial.printf("button play touched: %d!\n",touchRead(PIN_BUT_1));
      portENTER_CRITICAL(&dataAccessMux);
        data.btnPlayTouched=false;
      portEXIT_CRITICAL(&dataAccessMux);

    }
    if(data.btnOkTouched)
    {
      Serial.printf("button ok touched:%d!\n",touchRead(PIN_BUT_2));
      portENTER_CRITICAL(&dataAccessMux);
        data.btnOkTouched=false;
      portEXIT_CRITICAL(&dataAccessMux);
    }

    // Blocking stepper functions can be used.
    vTaskDelay(100);
  } // while
}

bool checkQuestionIsOk(int question)
{
  bool ok=true;
  bool objectFoundWithinAnswerPosition=false;
  bool isWithinAnswerPosition,prevIsWithinAnswerPosition;
  double dist;
  for(int pos=STEPPERLEFTPOS ; pos<=STEPPERRIGHTPOS ; pos++)
  {
    //check if radarposition is within answerposition
    isWithinAnswerPosition=(
      abs(config.answerArr[question].pos1-pos) < (SONAR_POSWINDOW/2) ||
      abs(config.answerArr[question].pos2-pos) < (SONAR_POSWINDOW/2));

    portENTER_CRITICAL_ISR(&dataAccessMux);
      dist=data.radarData[pos].cur.distance;
    portEXIT_CRITICAL_ISR(&dataAccessMux); 

    //check if object is found
    if(dist<SONAR_NO_SEE_DISTANCE)
    {
      //something is found
      if(isWithinAnswerPosition) 
      {
        objectFoundWithinAnswerPosition=true;
      }else
      {
        //object found outside answerzone
        ok=false;
      }
    }
      
    //check end of good-answer zone/window
    if(!isWithinAnswerPosition && prevIsWithinAnswerPosition) 
    {
      if(!objectFoundWithinAnswerPosition) 
      {
        // no object found in answer zone
        ok=false;
      }
      objectFoundWithinAnswerPosition=false;
    }

    prevIsWithinAnswerPosition=isWithinAnswerPosition;
    
    //stop looping if there is a fail
    if(!ok)break;
  }//for loop
  return ok;
}
#endif
