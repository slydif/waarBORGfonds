#ifndef DISPLAY_H
#define DISPLAY_H

//
// ======== INCLUDES =====================
//
#include <SPI.h>
#include <TFT_eSPI.h>  
#include <time.h>       
#include "wbf_data.h"

//
// ======== DEFINES ======================
//
#define CLR_BACKGROUND   0x0000   // 00, 00, 00 = black
//#define CLR_DARK       0x2945   // 28, 28, 28 = dark grey
#define CLR_TEXT         0xFFFF   // FF, FF, FF = white
//#define CLR_LABELS     0x8410   // 80, 80, 80 = grey
#define CLR_GREEN        0x0400   // 00, 80, 00 = green
#define CLR_RED          0xF800   // FF, 00, 00 = red
#define KEY_THRESHOLD  50

//
// ======== GLOBAL VARIABLES ============= 
//
TFT_eSPI tft = TFT_eSPI();  

void taskDisplay(void * parameter );
void drawBmp(TFT_eSPI& tft, const char *filename, int16_t x, int16_t y);


//
// ======== ISR ==========================
//
void onDisplayCheckKeys() 
{
  static volatile int keyPlayCntr  = 0;
  static volatile int keyOkCntr = 0;

  if (touchRead(PIN_BUT_1)<KEY_THRESHOLD) keyPlayCntr++;  else keyPlayCntr =0; 
  if (touchRead(PIN_BUT_2)<KEY_THRESHOLD) keyOkCntr++; else keyOkCntr=0; 

  if(keyPlayCntr==3 || keyOkCntr==3)
  {
    //only enter critical when key is pressed
    portENTER_CRITICAL_ISR(&dataAccessMux);
    if(keyPlayCntr==3)data.btnPlayTouched=true;
    if(keyOkCntr==3)data.btnOkTouched=true;
    portEXIT_CRITICAL_ISR(&dataAccessMux); 
  }
}


//
// ======== ROUTINES ===================== 
//
void setupDisplay() 
{
  // Initialize screen
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  
  Serial.printf("Create display task\n");
  xTaskCreatePinnedToCore(
    taskDisplay,            // The function containing the task
    "TaskDisplay",          // Name for mortals
    12000,                  // Stack size 
    NULL,                   // Parameters
    1,                      // Priority, 0=lowest, 3=highest
    NULL,                   // Task handle
    ARDUINO_RUNNING_CORE);  // Core to run the task on
}

void taskDisplay(void * parameter )
{
  tScreen prevScreen=scRadar; 
  bool updateScreen;
  int x,y,prevX,prevY,color;
  double dist,prevDist;
  //calculate position in screen of radarsensor
  //That depends on angle to show.
  //If angle is 0 or 180, sin=0, crash....to be secured.
  
  double maxDistPx=tft.width()/sin(config.tftMaxRadarAngle*DEG_TO_RAD/2);
  int x0=tft.width()/2;
  int y0=tft.height()-round(maxDistPx);//can be negative
  
  while(true) 
  {
    // Check if the whole screen needs to be updated
    portENTER_CRITICAL(&dataAccessMux);
      updateScreen=(data.screen!=prevScreen);          // If screen has changed, redraw screen
      prevScreen=data.screen;
    portEXIT_CRITICAL(&dataAccessMux);
    
    if(updateScreen) 
    { 
      tft.fillScreen(CLR_BACKGROUND);
    }//if(updateScreen)
    

    switch(data.screen) 
    {
      case scRadar:
        // Radar screen
        // Loop over dots  
        for(int i=0;i<RADAR_ARRAY_SIZE;i++)
        {
          if(!data.radarData[i].processed)//assume bool does not need entercritical
          {
            portENTER_CRITICAL(&dataAccessMux);
              double angle=map(data.sweepPosition,STEPPERLEFTPOS,STEPPERRIGHTPOS,-(config.tftMaxRadarAngle/2),(config.tftMaxRadarAngle/2));
              dist=data.radarData[i].cur.distance;
              prevDist=data.radarData[i].prev.distance;
              x=round(x0 + dist*sin(angle*DEG_TO_RAD));
              y=round(y0 + dist*cos(angle*DEG_TO_RAD));
              data.radarData[i].cur.screenX=x;
              data.radarData[i].cur.screenY=y;
              prevX=data.radarData[i].prev.screenX;
              prevY=data.radarData[i].prev.screenY;
              color=data.sweepBackward ? CLR_RED : CLR_GREEN;
              data.radarData[i].processed=true;
            portEXIT_CRITICAL(&dataAccessMux);
            
            //wipe dot of previous sweep
            if(prevDist>0 and prevY>0) tft.fillCircle(prevX, prevY, 3, CLR_BACKGROUND);
            
            //draw new dot
            if(dist>0 and y>0)tft.fillCircle(x, y, 3, color);
          }
        }
        break;
        
    }// Switch screen


    //drawBmp(tft, data.connected         ? "/WifiConn.bmp" : "/WifiDis.bmp" , 180, 0);
    //drawBmp(tft, data.timeSynched       ? "/TmSync.bmp"   : "/TmNSync.bmp" , 210, 0);
    //drawBmp(tft, data.sensorMalfunction ? "/SnBroken.bmp" : "/SensorOK.bmp", 150, 0);

      
    //data.displayHighWaterMark= uxTaskGetStackHighWaterMark( NULL );
    //Serial.printf("Display: %d bytes of stack unused\n", data.displayHighWaterMark);
    vTaskDelay(100);
  } // while true

}//taskDisplay


// Functions to display bitmaps, provided by Bodmer

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) 
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) 
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Bodmers BMP image rendering function
void drawBmp(TFT_eSPI& tft, const char *filename, int16_t x, int16_t y) 
{

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SPIFFS
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}


#endif
