#include <Wire.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
// #include <WiFi.h> //ESP32
#include <ESP8266WiFi.h> 
#include <WiFiUdp.h>
#include <DHT22.h> //DHT22
#include <EEPROM.h>
#include <TimeLib.h>
#include "function.h"

#define fanPin 12 // Fan Pin
#define heatPin 14 // Heating Pin
#define motorPin1 2 // Пин 1 для управления мотором с драйвером L293
#define motorPin2 0 // Пин 2 для управления мотором с драйвером L293
#define data 15 // pin DHT22 ESP32
const int buttonPin = 13; // Пин, к которому подключена кнопка

const unsigned long motorDuration = 8000; // Длительность включения мотора в миллисекундах (5 секунд)
const unsigned long motorOffInterval = 20000; // Интервал между включениями в миллисекундах (5 часов)

unsigned long previousMotorMillis = 0;
bool motorState = false; // Состояние мотора (включен/выключен)
bool motorDirection = true; // Направление вращения мотора (true - вперед, false - назад)

int moreDay = 3; // Дополнительный дни для инкубатора.аа

const int debounceDelay = 50; // Задержка для подавления дребезга кнопки
const int resetButtonHoldTime = 5000; // Время удержания кнопки для сброса (10 секунд)
// Массив, который содержит количество дней в каждом месяце
const int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressStartTime = 0;
bool resetFlag = false;
bool dateWritten = false;
bool modeInkub = false;
String defaultParams = "";
String getStartDate = "";
DHT22 dht22(data); 

Parameters defaultModes[] = 
{
    // name | min_temp | max_temp | min_hum_start | min_hum_end | max_hum_start | max_hum_end | days | last_days  
    {"Chicken", 36.5, 38.0, 55, 75, 60, 80, 21, 3}
};

SSD1306Wire display(0x3c, 4, 5); // OLED-дисплей с адресом 0x3c

const char* ssid = "X0GAE";
const char* password = "uD8E0-2oP";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 14400; // GMT+1 (поправка на временной пояс)
const int daylightOffset_sec = 3600; // Переход на летнее/зимнее время (поправка)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600); // Инициализация Serial
  // Инициализация пинов
  pinMode(fanPin, OUTPUT);
  pinMode(heatPin, OUTPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);

  display.init(); // Инициализация OLED-дисплея
  display.flipScreenVertically(); // Если нужно развернуть экран
  display.clear(); // Очистка дисплея
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  int i = 25;
  WiFi.begin(ssid, password);

// Save date in EEPROM
  EEPROM.begin(sizeof(String));
  // Записываем параметры в EEPROM только один раз при запуске
  if (EEPROM.read(0) == 0xFF) 
  {
      EEPROM.put(0, defaultParams);
      EEPROM.commit();
  } else 
  {
      dateWritten = true;
  }
  
  // Wi-Fi
  display.drawString (0, 0, "Wi-Fi ");
  while ( WiFi.status() != WL_CONNECTED ) 
  {
    delay ( 500 );
    Serial.print ( "." );
    display.drawString (i, 0, ".");
    i+=1;
    display.display();
  }
  
  // Время
  configTime(14400, 0, "time.windows.com"); // Используем сервер, который предоставляет текущее время и дату
  display.drawString (0, 10, "NTP ");
  // Ожидание получения времени от NTP-сервера
  while (!time(nullptr)) 
  {
    delay(1000);
    Serial.println("Ожидание синхронизации времени с NTP...");
    display.drawString (i, 10, ".");
    i+=1;
    display.display();
  }
  i = 25;
  timeClient.begin();
  
  display.display();
}

void loop() 
{
  // put your main code here, to run repeatedly:
  // timeClient.update();
  unsigned long currentMillis = millis();
  float t = dht22.getTemperature();
  float h = dht22.getHumidity();

  // Вывод времени и даты ------------------------------
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  String yearStr = "";
  String yearShotStr = "";
  String mouthStr = "";
  String dayStr = "";
  char timeString[30];
  char dateString[30];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", timeInfo);
  strftime(dateString, sizeof(dateString), "%Y-%m-%d", timeInfo);

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  // display.drawString(100, 0, "10:20");

  if (WiFi.status() != WL_CONNECTED)
    display.drawString(120, 0, "х");
  else
    display.drawString(120, 0, "w");

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Mode: " + String(defaultModes[0].modeName));
  display.drawLine(50, 18, 105, 18);
  display.setFont(ArialMT_Plain_10);

  int heatVol;
  int fanVol;
  float heatVolPercent;
  float fanVolPercent;

  if (String(dateString).substring(0, 4) != "1970")
  {
    yearStr = String(dateString).substring(0, 4);
    yearShotStr = String(dateString).substring(2, 4);
    mouthStr = String(dateString).substring(5, 7);
    dayStr = String(dateString).substring(8, 10);
    String passDaysStr = passDay(EEPROM.get(0, getStartDate), dateString);
    int passDays = passDaysStr.toInt();

    if (passDays <= (int(defaultModes[0].days)-int(defaultModes[0].last_days)))
    {
      // display.drawString(120, 10, String());
      if (!motorState && (currentMillis - previousMotorMillis >= motorOffInterval)) 
      {
        // Включаем мотор и устанавливаем флаг
        display.drawString(120, 10,  "m");
        digitalWrite(motorPin1, motorDirection ? HIGH : LOW);
        digitalWrite(motorPin2, motorDirection ? LOW : HIGH); // Направление зависит от motorDirection
        motorState = true;
        previousMotorMillis = currentMillis;
      } 
      else if (motorState && (currentMillis - previousMotorMillis >= motorDuration)) 
      {
        // Выключаем мотор и сбрасываем флаг
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, LOW);
        display.drawString(120, 10,  "x");
        motorState = false;
        
        // Инвертируем направление вращения мотора
        motorDirection = !motorDirection;
      }
    
      // .........................................................................................

      if(t<(defaultModes[0].max_temp-0.5))
      {
        heatVol = 255;
        fanVol = getFanVol(h, passDays);
      }
      else if (t>=(defaultModes[0].max_temp-0.5) && t<=defaultModes[0].max_temp) {
        heatVol = 0;
        fanVol = getFanVol(h, passDays);
      }
      else if (t>(defaultModes[0].max_temp) && t<=(defaultModes[0].max_temp+0.5)) {
        heatVol = 0;
        fanVol = getFanVol(h, passDays);
      }
      else if (t>(defaultModes[0].max_temp+0.5)) {
        heatVol = 0;
        fanVol = 0;
      }
      else {
        heatVol = 0;
        fanVol = 0;
      }
    }
<<<<<<< HEAD
    else if(passDays>(int(defaultModes[0].days)-int(defaultModes[0].last_days)) && passDays<=(int(defaultModes[0].days)+moreDay)) {
=======
    else if(passDays>(int(defaultModes[0].days)-int(defaultModes[0].last_days)) && passDays<=int(defaultModes[0].days)) 
    {
>>>>>>> parent of 223378d (Оптимизация кода)
      // Уапрвлением Нагревателем и Вентилятором Последний этап инкубации
      if(t<(defaultModes[0].min_temp-0.5))
      {
        heatVol = 255;
        fanVol = getFanVol(h, passDays);
      }
<<<<<<< HEAD
      else if (t>=(defaultModes[0].min_temp-0.5) && t<=defaultModes[0].min_temp) {
        heatVol = 0;
=======
      else if (t>=(defaultModes[0].min_temp-0.5) && t<=defaultModes[0].min_temp) 
      {
        heatVol = 102;
>>>>>>> parent of 223378d (Оптимизация кода)
        fanVol = getFanVol(h, passDays);
      }
      else if (t>defaultModes[0].min_temp && t<=(defaultModes[0].min_temp+0.5)) 
      {
        heatVol = 0;
        fanVol = getFanVol(h, passDays);
      }
      else if (t>(defaultModes[0].min_temp+0.5)) 
      {
        heatVol = 0;
        fanVol = 255;
      }
      else 
      {
        heatVol = 0;
        fanVol = 0;
      }
    }
<<<<<<< HEAD
    else if(passDays>(int(defaultModes[0].days)+moreDay)) {
=======
    else if(passDays>int(defaultModes[0].days)) 
    {
>>>>>>> parent of 223378d (Оптимизация кода)
      heatVol = 0;
      fanVol = 0;
    } 

    analogWrite(fanPin, fanVol);
    analogWrite(heatPin, heatVol);
    fanVolPercent = map(fanVol, 0, 255, 0.0, 100.0);
    heatVolPercent = map(heatVol, 0, 255, 0.0, 100.0);
    display.drawString(63, 30, "| Heat: " + String(heatVolPercent) + "%");
    display.drawString(63, 20, "| Fan: " + String(fanVolPercent) + "%" );
    // ------------------------------------------------------------ End

    


    // display.drawString(0, 20, "T: 37.50 (" + String(defaultModes[0].min_temp) +"-" + String(defaultModes[0].max_temp)+") on");
    // display.drawString(0, 40, "Days left: " + String(defaultModes[0].days));
    display.drawString(0, 40, "Days left: " + passDaysStr + "/" + String(defaultModes[0].days));
    if (EEPROM.get(0, getStartDate)!="")
    {
      modeInkub=true;
      display.drawString(0, 50, "Start: "+ EEPROM.get(0, getStartDate));
    }
    else
      display.drawString(0, 50, "Start: NULL");
    // ...........................................................................................
  }
  else
  {
    yearStr = "load";
    yearShotStr = "load";
    mouthStr = "";
    dayStr = "";
  }
  // ------------------------------------------------------------ End

  // Работа с кнопкой 
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) 
  {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) 
  {
    if (reading != buttonState) 
    {
      buttonState = reading;
      if (buttonState == LOW) 
      {
        buttonPressStartTime = millis();
      } 
      else 
      {
        unsigned long buttonPressDuration = millis() - buttonPressStartTime;
        if (buttonPressDuration >= resetButtonHoldTime) {
          // Удержание кнопки в течение 10 секунд - сброс данных
          EEPROM.put(0, defaultParams);
          EEPROM.commit();
          Serial.println("Данные сброшены и записаны заново в EEPROM");
          resetFlag = true;
          modeInkub = false;
          dateWritten = false;
        } 
        else {
          if (!dateWritten) {
            // Проверяем, была ли уже записана дата
            String currentDate = String(dateString); // Замените на фактический код для получения текущей даты
            String storedDate;
            EEPROM.get(0, storedDate);
            if (currentDate != storedDate) {
              EEPROM.put(0, currentDate);
              EEPROM.commit();
              Serial.println("Дата записана в EEPROM: " + currentDate);
              EEPROM.get(0, storedDate);
              Serial.println("Текущая дата в EEPROM: " + storedDate);
              dateWritten = true;
              modeInkub = true;
            } 
          }
          else {
            Serial.println("Дата уже была записана ранее");
          }
        }
        resetFlag = false;
      }
    }
  }
  lastButtonState = reading;
  // ------------------------------------------------------------ End

  display.drawString(0, 20, "T: " + String(t, 1) + "C'" );
  display.drawString(0, 30, "H: " + String(h, 1) + "%");
  if (modeInkub)
  {
    display.setFont(ArialMT_Plain_16);
    display.drawString(90, 45, "ON");
    display.setFont(ArialMT_Plain_10);
  }
  else {
    display.setFont(ArialMT_Plain_16);
    display.drawString(90, 45, "OFF");
    display.setFont(ArialMT_Plain_10);
  }
  display.display();
}

// Разбиваем строку с датой на год, месяц и день
String passDay (String startDateStr, String nowDateStr){
  // char* nowDateChar =  char*(nowDateStr);
  // char* startDateChar =  char*(startDateStr);
  if(startDateStr != ""){
    int nowYear, nowMonth, nowDay;
    sscanf(nowDateStr.c_str(), "%d-%d-%d", &nowYear, &nowMonth, &nowDay);

    int year, month, day;
    sscanf(startDateStr.c_str(), "%d-%d-%d", &year, &month, &day);

    // Вычисляем разницу в годах, месяцах и днях
    int yearDiff = nowYear - year;
    int monthDiff = nowMonth - month;
    int dayDiff = nowDay - day;

    if (dayDiff < 0) {
      // Если дни отрицательны, корректируем разницу
      monthDiff--; // Уменьшаем месяц на 1

      // Вычисляем количество дней в предыдущем месяце
      int daysInPreviousMonth = daysInMonth[month];
      if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        // Если февраль и год високосный, то добавляем день
        daysInPreviousMonth++;
      }

      dayDiff += daysInPreviousMonth;
    }

    if (monthDiff < 0) {
      // Если месяцы отрицательны, корректируем разницу
      monthDiff += 12; // Уменьшаем год на 1
      yearDiff--;
    }
    return String (dayDiff);
  }
  return String ("null");
}
// ------------------------------------------------------------ End

<<<<<<< HEAD

=======
int getFanVol(float h, int passDays)
{
  int setFanVol;
  if (passDays <= (int(defaultModes[0].days)-int(defaultModes[0].last_days)))
  {
    if(int(h)<defaultModes[0].min_hum_start-10)
    {
      setFanVol= 255;
    }
    else if(int(h)>=defaultModes[0].min_hum_start && int(h)<=defaultModes[0].max_hum_start)
    {
      setFanVol= 76;
    }
    else if(int(h)>=defaultModes[0].max_hum_start)
    {
      setFanVol= 0;
    }
  }
  else if(passDays>(int(defaultModes[0].days)-int(defaultModes[0].last_days)) && passDays<=int(defaultModes[0].days)) 
  {
    if(int(h)<defaultModes[0].min_hum_end-10)
    {
      setFanVol= 255;
    }
    else if(int(h)>=defaultModes[0].min_hum_end && int(h)<=defaultModes[0].max_hum_end)
    {
      setFanVol= 76;
    }
    else if(int(h)>=defaultModes[0].max_hum_end)
    {
      setFanVol= 0;
    }
  }
  return setFanVol;
}
>>>>>>> parent of 223378d (Оптимизация кода)
