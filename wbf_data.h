#ifndef DATA_H
#define DATA_H

#include "FS.h"
#include "SPIFFS.h"
#include <list>
#include <ArduinoJson.h>

#define STEPPERLEFTPOS 0
#define STEPPERRIGHTPOS 48
#define ANSWER_ARRSIZE 5
#define RADAR_ARRAY_SIZE STEPPERRIGHTPOS-STEPPERLEFTPOS+1

// ======== CONSTANTS ================
const char *CONFIG_FILE = "/config.jsn";

// ======== DATA TYPES ============= 
enum tScreen          {scRadar};
enum tSwitchEvent     {seStart,   seCompressorOn,  seCompressorOff,  seAlarm,            seSensorMalfunction };
enum tISRMeasureStep  {isrMsIdle, isrMsTriggering, isrMsWaitingEcho, isrMsMeasuringEcho, isrMsAnswer};
enum tMeasureStep     {msIdle,    msWaitAnswer};
enum tSweepStep       {ssInit,    ssForward,       ssBack};
enum tGameplayStep    {gpIdle,    gpQ1,            gpQ2};


typedef struct _tMeasurement  
{
  double distance;
  int screenX,screenY; 
} tMeasurement;

typedef struct _tPositionData
{
  tMeasurement cur;
  tMeasurement prev;
  bool processed=true;
} tPositionData;

typedef struct _tCanPositions
{
  int pos1;
  int pos2;
} tCanPositions;

class tData 
{
  public:
  
    //screen
    tScreen screen=scRadar;
    bool btnPlayTouched=false; // button 1
    bool btnOkTouched=false;   // button 2
    
    //measure
    bool startMeasure=false;   // measure with radar
    double radarDistanceMm=0.0;// distance in front of radar
    tPositionData radarData[RADAR_ARRAY_SIZE];
    

    //sweep
    bool sweepReInit=false;
    bool sweepBackward=false;
    long sweepPosition=-1;

    //gameplay
    tGameplayStep gameStep=gpIdle;
    bool allAnswersOk=false;

    
    // Initialize data 
    tData() 
    {
    }
};    


class tConfig 
{
  public:
    double stepperMaxSpeed=100.0;
    double stepperAcceleration=5.0;
    double tftMaxRadarAngle=90;//Angle of radar Pie
    double tftMaxDistMm=300;//Max shown measurement (for scaling to tft)
    tCanPositions answerArr[ANSWER_ARRSIZE];

    //initialize
    tConfig() 
    {
      answerArr[1].pos1=5;
      answerArr[1].pos2=22;
      answerArr[2].pos1=12;
      answerArr[2].pos2=23;
    }//tConfig
  
    void Load() 
    {    
      StaticJsonDocument<1024> doc;
      File input = SPIFFS.open(CONFIG_FILE);
      DeserializationError error = deserializeJson(doc, input);
      
      if (error) 
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
    }//load

    void Save() 
    {
      // To do: loop over access points
    }//save
};


// ======== GLOBAL VARIABLES ============= 
tConfig config; // Configuration data, to be stored as JSON file in SPIFFS
tData data;     // Data, lost if device is switched off

portMUX_TYPE dataAccessMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE configAccessMux = portMUX_INITIALIZER_UNLOCKED;

#endif
