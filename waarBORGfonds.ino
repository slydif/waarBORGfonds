#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#if CONFIG_FREERTOS_UNICORE
  #define ARDUINO_RUNNING_CORE 0
#else
  #define ARDUINO_RUNNING_CORE 1
#endif

#include "wbf_data.h"
#include "wbf_1sweep.h"
#include "wbf_2display.h"
#include "wbf_3measure.h"
#include "wbf_4gameplay.h"

using namespace std;


// ======== DEFINES ==========================
#define FORMAT_SPIFFS_IF_FAILED false


// ======== GLOBAL VARIABLES ================= 
hw_timer_t * timer = NULL;


// ======== INTERRUPT HANDLERS   ============= 
void IRAM_ATTR onEveryMicrosecond() 
{
  //
  // This interrupt is used for the buttons as well as the distance measurement of the HC-SR04
  //
  //count the number of calls of the interrupt
  static volatile int isrCntr = 0;
  isrCntr++;
  if(isrCntr>=1000)isrCntr=0;

  //Call keycheck every 10 microseconds.
  if(isrCntr%10==0)onDisplayCheckKeys();

  //Call the radar measurement every microsecond
  onRadarMeasure();
}


void setup() 
{
  Serial.begin(115200);  

  // Initialize SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
      Serial.println("SPIFFS Mount Failed");
      return;
  }
  Serial.println("SPIFFS loaded");  

  // Load configuration data from SPIFFS
  config.Load();
  Serial.println("Config loaded");  

  // Start interrupt
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onEveryMicrosecond, true);
  timerAlarmWrite(timer, 10000, true); // 10
  timerAlarmEnable(timer);
    
  // Setup all sub processes
  setupSweep();
  setupMeasure();
  setupDisplay();  
  setupGameplay();
}

void loop() 
{
  // All tasks delegated to separate threads
  // Delay to avoid stupid spinning
  vTaskDelay(1);
}
