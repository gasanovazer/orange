#ifndef FUNCTION_H
#define FUNCTION_h

#include <Arduino.h>

struct Parameters 
{
    String modeName; 
    double min_temp;
    double max_temp;
    int min_hum_start;
    int min_hum_end;
    int max_hum_start;
    int max_hum_end;
    int days;
    int last_days;
    String start_date;
};

struct displayParametrs
{
  /* data */
  String statusWiFi;
  String modeName;
  String heatVolPercent;
  String fanVolPercent;
  String daysLeft;
  String days;
}; 

int getFanVol(float h, int passDays);
void displayPrint(int displayMode);

#endif