#include "function.h"


int getFanVol(float h, int passDays)
{
  int setFanVol;
  if (passDays <= (int(defaultModes[0].days)-int(defaultModes[0].last_days)))
  {
    if(int(h)<defaultModes[0].min_hum_start-10) setFanVol= 255;
    else if(int(h)>=defaultModes[0].min_hum_start && int(h)<=defaultModes[0].max_hum_start) setFanVol= 76;
    else if(int(h)>=defaultModes[0].max_hum_start) setFanVol= 255;
  }
  else if(passDays>(int(defaultModes[0].days)-int(defaultModes[0].last_days)) && passDays<=int(defaultModes[0].days)) 
  {
    if(int(h)<defaultModes[0].min_hum_end-10) setFanVol= 255;
    else if(int(h)>=defaultModes[0].min_hum_end && int(h)<=defaultModes[0].max_hum_end) setFanVol= 76;
    else if(int(h)>=defaultModes[0].max_hum_end) setFanVol= 255;
  }
  return setFanVol;
}

void displayPrint(int displayMode){
  switch (displayMode)
  {
  case 1:
    /* Режим отображения настроек Setup */
    break;
  
  case 2:
    /* Режим отражения Настроек */
    break;
  case 3:
    /* Режим оторобажения режима работы. */
    break;
  
  
  default:
    break;
  }
}