#include <EEPROM.h>
#include <ezButton.h>
#include <thermistor.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// RTC time module, pins D6,D7,D8
virtuabotixRTC myRTC(6, 7, 8);

// Display 16x2, I2C, pins A4,A5
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define buttons pin A2, A3, A4, A5
ezButton buttonUp(2);
ezButton buttonDown(3);
ezButton buttonBack(4);
ezButton buttonEnter(5);

// Analog pin used to read the NTC
#define NTCB3_PIN A3
// Thermistor object boiler high position (Analog pin,Nominal resistance at 25 ºC,thermistor's beta coefficient,Value of the series resistor)
THERMISTOR thermistorB3(NTCB3_PIN, 10000, 3950, 10000);
// Global temperature reading
int tempB3;

#define NTCB2_PIN A2
// Thermistor object boiler middle position (Analog pin,Nominal resistance at 25 ºC,thermistor's beta coefficient,Value of the series resistor)
THERMISTOR thermistorB2(NTCB2_PIN, 10000, 3950, 10000);
// Global temperature reading
int tempB2;

#define NTCB1_PIN A1
// Thermistor object boiler low position (Analog pin,Nominal resistance at 25 ºC,thermistor's beta coefficient,Value of the series resistor)
THERMISTOR thermistorB1(NTCB1_PIN, 10000, 3950, 10000);
// Global temperature reading
int tempB1;

#define NTCSOLAR_PIN A0
// Thermistor object solar
THERMISTOR thermistorSolar(NTCSOLAR_PIN, 10000, 3950, 10000);
// Global temperature reading
int tempSolar;

struct timerPump
{
  byte startHour;
  byte startMin;
  byte endHour;
  byte endMin;
  int start;
  int end;
  bool active;
  bool days[7];
};

timerPump timers[10];

#define pumpPin 9 // Pump relay pin

// RGB LEDs pin
#define redLed 10
#define greenLed 11
#define blueLed 12

byte stat[24];
byte homeScreenTime;
byte backLightTime = 10;
bool pumpIsOn = false;
timerPump tenMinutesPump;
int lowTemp = 40;
int highTemp = 47;

// EEPROM Addresses
int backLightTimeAddress = 10;
int lowTempAddress = 20;
int highTempAddress = 30;
int pumpTimersAddress = 50;

void setup()
{
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);

  lcd.print("WELLCOME!!!");
  // seconds, minutes, hours, day of the week, day of the month, month, year
  // myRTC.setDS1302Time(10, 32, 18, 1, 10, 4, 2022);

  // buttonUp.setDebounceTime(450);
  // buttonDown.setDebounceTime(450);
  // buttonBack.setDebounceTime(450);
  // buttonEnter.setDebounceTime(450);

  startSession();
  // PumpRelay
  pinMode(pumpPin, OUTPUT);

  // RGB pins
  pinMode(blueLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  // Clear statistics
  for (size_t i = 0; i < 23; i++)
  {
    stat[i] = 0;
  }

  // EEPROM operations
  backLightTime = EEPROM.read(backLightTimeAddress);
  if (backLightTime > 60)
  {
    backLightTime = 10;
  }

  EEPROM.get(lowTempAddress, lowTemp);
  if (lowTemp > 200)
  {
    lowTemp = 39;
  }
  EEPROM.get(highTempAddress, highTemp);
  if (highTemp > 200)
  {
    highTemp = 47;
  }

  // initial timers
  EEPROM.get(pumpTimersAddress, timers);

  for (size_t i = 0; i < 10; i++)
  {
    if (timers[i].startHour == 255)
    {
      timers[i].active = false;
      timers[i].startHour = 0;
      timers[i].startMin = 0;
      timers[i].endHour = 0;
      timers[i].endMin = 0;
      timers[i].start = 1555;
      timers[i].end = -1;

      for (size_t j = 0; j < 7; j++)
      {
        timers[i].days[j] = true;
      }
    }
  }
}

void loop()
{
  buttonUp.loop();
  buttonDown.loop();
  buttonBack.loop();
  buttonEnter.loop();
  endSession();

  if (buttonUpIsPressed())
  {
    startSession();
  }
  else if (buttonDownIsPressed())
  {
    startSession();
    tenMinutesPumpON();
  }
  else if (buttonBackIsPressed())
  {
    printStatistics();
  }
  else if (buttonEnterIsPressed())
  {
    settings();
  }
  pumpControl();
  tempB3 = thermistorB3.read();       // Read temperature
  tempB2 = thermistorB2.read();       // Read temperature
  tempB1 = thermistorB1.read();       // Read temperature
  tempSolar = thermistorSolar.read(); // Read temperature
  printFrontScreen();
  rgbControl();
  statistics();
}

void printStatistics()
{
  startSession();
  int position = 1;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();
    if (position == 1)
    {
      String firstLine ="T:00 01 02 03 04";
      String secondLine = "C:";
      for (size_t i = 0; i < 5; i++)
      {
        secondLine += twoDigit(stat[i]) + " ";
      }
      customLines(firstLine,secondLine);
    }
    else if (position ==2)
    {
      String firstLine ="T:05 06 07 08 09  ";
      String secondLine = "C:";
      for (size_t i = 5; i < 10; i++)
      {
        secondLine += twoDigit(stat[i]) + " ";
      }
      customLines(firstLine,secondLine);
    }
    else if (position == 3)
    {
      String firstLine ="T:10 11 12 13 14  ";
      String secondLine = "C:";
      for (size_t i = 10; i < 15; i++)
      {
        secondLine += twoDigit(stat[i]) + " ";
      }
      customLines(firstLine,secondLine);
    }
    else if (position == 4)
    {
      String firstLine ="T:15 16 17 18 19  ";
      String secondLine = "C:";
      for (size_t i = 15; i < 20; i++)
      {
        secondLine += twoDigit(stat[i]) + " ";
      }
      customLines(firstLine,secondLine);
    }
    else if (position == 5)
    {
      String firstLine ="T:20 21 22 23  ";
      String secondLine = "C:";
      for (size_t i = 20; i < 24; i++)
      {
        secondLine += twoDigit(stat[i]) + " ";
      }
      customLines(firstLine,secondLine);
    }
    
    if (buttonBackIsPressed())
    {
      startSession();
      break;
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      position --;
      if (position < 1)
      {
        position = 5;
      }
      
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      position++;
      if (position > 5)
      {
        position =1;
      }
      
    }
    else if (buttonEnterIsPressed())
    {
      startSession();
      position++;
      if (position > 5)
      {
        position =1;
      }
    }
  }
}
void statistics()
{
  if (myRTC.minutes == 0)
  {
    byte temp = ((tempB1 + tempB2 + tempB3) / 10) / 3;
    if (myRTC.hours >= 0 && myRTC.hours <= 23)
    {
      stat[myRTC.hours] = temp;
    }
  }
  if (myRTC.hours == 23 && myRTC.minutes == 59)
  {
    for (size_t i = 0; i < 23; i++)
    {
      stat[i] = 0;
    }
  }
}
void rgbControl()
{
  int temp = ((tempB2 + tempB3) / 2) / 10;
  if (temp < lowTemp)
  {
    digitalWrite(blueLed, HIGH);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
  }
  else if (temp >= lowTemp && temp <= highTemp)
  {
    digitalWrite(blueLed, LOW);
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
  }
  else if (temp > highTemp)
  {
    digitalWrite(blueLed, LOW);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
  }
}
void settingsTemperature()
{
  startSession();
  int tempLow = lowTemp;
  int tempHigh = highTemp;
  int blink = 0;
  int position = 1;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();
    blink++;
    if (blink < 10)
    {
      String firstLine = "Low temp:  " + String(tempLow);
      String secondLine = "High temp: " + String(tempHigh);
      customLines(firstLine, secondLine);
    }
    else if (blink > 10)
    {
      if (position == 1)
      {
        lcd.setCursor(11, 0);
        lcd.print("__");
      }
      else if (position == 2)
      {
        lcd.setCursor(11, 1);
        lcd.print("__");
      }

      if (blink >= 20)
      {
        blink = 0;
      }
    }

    if (buttonBackIsPressed())
    {
      startSession();
      break;
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      if (position == 1)
      {
        tempLow--;
      }
      else if (position == 2)
      {
        tempHigh--;
      }
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      if (position == 1)
      {
        tempLow++;
      }
      else if (position == 2)
      {
        tempHigh++;
      }
    }
    else if (buttonEnterIsPressed())
    {
      startSession();
      position++;
      if (position >= 3)
      {
        if (tempLow >= tempHigh)
        {
          customLines("ERROR!!!", "low < high");
          delay(1000);
          break;
        }

        lowTemp = tempLow;
        highTemp = tempHigh;

        EEPROM.put(lowTempAddress, lowTemp);

        EEPROM.put(highTempAddress, highTemp);
        customLines("SUCCESSFUL !!!", " ");
        delay(1000);
        break;
      }
    }
  }
}
void settingsPumpTimersEdit(int index)
{
  startSession();

  int position = 1;
  byte startHour = timers[index].startHour;
  byte startMinutes = timers[index].startMin;
  byte endHour = timers[index].endHour;
  byte endMinutes = timers[index].endMin;
  int start = timers[index].start;
  int end = timers[index].end;
  bool active = timers[index].active;
  bool days[7];
  for (size_t i = 0; i < 7; i++)
  {
    days[i] = timers[index].days[i];
  }

  int blink = 0;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();
    blink++;

    if (blink < 10)
    {
      String firstLine = String(index);
      if (active)
      {
        firstLine += "* ";
      }
      else
      {
        firstLine += "  ";
      }
      firstLine += clockFiveSymbols(startHour, startMinutes);
      firstLine += " MTWTFSS";
      String secondLine = "   ";
      secondLine += clockFiveSymbols(endHour, endMinutes);
      secondLine += " ";
      for (size_t i = 0; i < 7; i++)
      {
        if (days[i])
        {
          secondLine += "*";
        }
        else
        {
          secondLine += " ";
        }
      }
      customLines(firstLine, secondLine);
    }
    else if (blink >= 10)
    {
      switch (position)
      {
      case 1:
        lcd.setCursor(1, 0);
        lcd.print("_");
        break;
      case 2:
        lcd.setCursor(3, 0);
        lcd.print("__");
        break;
      case 3:
        lcd.setCursor(6, 0);
        lcd.print("__");
        break;
      case 4:
        lcd.setCursor(3, 1);
        lcd.print("__");
        break;
      case 5:
        lcd.setCursor(6, 1);
        lcd.print("__");
        break;
      case 6:
        lcd.setCursor(9, 1);
        lcd.print("_");
        break;
      case 7:
        lcd.setCursor(10, 1);
        lcd.print("_");
        break;
      case 8:
        lcd.setCursor(11, 1);
        lcd.print("_");
        break;
      case 9:
        lcd.setCursor(12, 1);
        lcd.print("_");
        break;
      case 10:
        lcd.setCursor(13, 1);
        lcd.print("_");
        break;
      case 11:
        lcd.setCursor(14, 1);
        lcd.print("_");
        break;
      case 12:
        lcd.setCursor(15, 1);
        lcd.print("_");
        break;
      default:
        break;
      }
    }
    if (blink >= 20)
    {
      blink = 0;
    }

    if (buttonEnterIsPressed())
    {
      startSession();
      position++;
      if (position > 12)
      {
        start = startHour * 60 + startMinutes;
        end = endHour * 60 + endMinutes;
        if (start < end)
        {
          timers[index].active = active;
          timers[index].startHour = startHour;
          timers[index].startMin = startMinutes;
          timers[index].endHour = endHour;
          timers[index].endMin = endMinutes;
          timers[index].start = start;
          timers[index].end = end;

          for (size_t i = 0; i < 7; i++)
          {
            timers[index].days[i] = days[i];
          }

          EEPROM.put(pumpTimersAddress, timers);
          customLines("SUCCESSFUL !!!", " ");
          delay(1000);
          break;
        }
        else
        {
          customLines("ERROR !!!", "start,end time");
          delay(1000);
          break;
        }
      }
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      switch (position)
      {
      case 1:
        active = !active;
        break;
      case 2:
        startHour++;
        if (startHour > 23)
        {
          startHour = 0;
        }
        break;
      case 3:
        startMinutes++;
        if (startMinutes > 59)
        {
          startMinutes = 0;
        }
        break;
      case 4:
        endHour++;
        if (endHour > 23)
        {
          endHour = 0;
        }
        break;
      case 5:
        endMinutes++;
        if (endMinutes > 59)
        {
          endMinutes = 0;
        }
        break;
      case 6:
        if (days[0])
        {
          days[0] = false;
        }
        else
        {
          days[0] = true;
        }

        break;
      case 7:
        if (days[1])
        {
          days[1] = false;
        }
        else
        {
          days[1] = true;
        }
        break;
      case 8:
        if (days[2])
        {
          days[2] = false;
        }
        else
        {
          days[2] = true;
        }
        break;
      case 9:
        if (days[3])
        {
          days[3] = false;
        }
        else
        {
          days[3] = true;
        }
        break;
      case 10:
        if (days[4])
        {
          days[4] = false;
        }
        else
        {
          days[4] = true;
        }
        break;
      case 11:
        if (days[5])
        {
          days[5] = false;
        }
        else
        {
          days[5] = true;
        }
        break;
      case 12:
        if (days[6])
        {
          days[6] = false;
        }
        else
        {
          days[6] = true;
        }
        break;
      default:
        break;
      }
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      switch (position)
      {
      case 1:
        active = !active;
        break;
      case 2:
        startHour--;
        if (startHour > 23)
        {
          startHour = 23;
        }
        break;
      case 3:
        startMinutes--;
        if (startMinutes > 59)
        {
          startMinutes = 59;
        }
        break;
      case 4:
        endHour--;
        if (endHour > 23)
        {
          endHour = 23;
        }
        break;
      case 5:
        endMinutes--;
        if (endMinutes > 59)
        {
          endMinutes = 59;
        }
        break;
      case 6:
        if (days[0])
        {
          days[0] = false;
        }
        else
        {
          days[0] = true;
        }

        break;
      case 7:
        if (days[1])
        {
          days[1] = false;
        }
        else
        {
          days[1] = true;
        }
        break;
      case 8:
        if (days[2])
        {
          days[2] = false;
        }
        else
        {
          days[2] = true;
        }
        break;
      case 9:
        if (days[3])
        {
          days[3] = false;
        }
        else
        {
          days[3] = true;
        }
        break;
      case 10:
        if (days[4])
        {
          days[4] = false;
        }
        else
        {
          days[4] = true;
        }
        break;
      case 11:
        if (days[5])
        {
          days[5] = false;
        }
        else
        {
          days[5] = true;
        }
        break;
      case 12:
        if (days[6])
        {
          days[6] = false;
        }
        else
        {
          days[6] = true;
        }
        break;
      default:
        break;
      }
    }
    else if (buttonBackIsPressed())
    {
      startSession();
      break;
      ;
    }
  }
}
void settingsPumpTimers()
{
  startSession();

  int position = 0;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();

    position = abs(position) % 10;
    String firstLine = String(position);
    if (timers[position].active)
    {
      firstLine += "* ";
    }
    else
    {
      firstLine += "  ";
    }
    firstLine += clockFiveSymbols(timers[position].startHour, timers[position].startMin);
    firstLine += " MTWTFSS";
    String secondLine = "   ";
    secondLine += clockFiveSymbols(timers[position].endHour, timers[position].endMin);
    secondLine += " ";
    for (size_t i = 0; i < 7; i++)
    {
      if (timers[position].days[i])
      {
        secondLine += "*";
      }
      else
      {
        secondLine += " ";
      }
    }

    customLines(firstLine, secondLine);

    if (buttonEnterIsPressed())
    {
      settingsPumpTimersEdit(position);
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      position += 1;
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      position -= 1;
    }
    else if (buttonBackIsPressed())
    {
      startSession();
      break;
    }
  }
}
void settings()
{
  startSession();

  int position = 0;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();

    position = abs(position) % 4;
    printSettings(position);

    if (buttonEnterIsPressed())
    {
      openSetting(position);
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      position += 1;
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      position -= 1;
      if (position < 0)
      {
        position = 3;
      }
    }
    else if (buttonBackIsPressed())
    {
      startSession();
      break;
    }
  }
}

void SettingsBacklight()
{
  startSession();
  int temp = backLightTime;
  int blink = 0;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();
    blink++;
    if (blink < 10)
    {
      customLines("Backlight time", "seconds: " + String(temp));
    }
    else if (blink > 10)
    {
      customLines("Backlight time", "seconds: ");
      if (blink == 20)
      {
        blink = 0;
      }
    }

    if (buttonBackIsPressed())
    {
      startSession();
      break;
    }
    else if (buttonDownIsPressed())
    {
      startSession();
      temp -= 1;
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      temp += 1;
    }
    else if (buttonEnterIsPressed())
    {
      startSession();
      backLightTime = temp;
      EEPROM.update(backLightTimeAddress, backLightTime);
      customLines("SUCCESSFUL !!!", " ");
      delay(1000);
      break;
    }
  }
}

void settingsDateTime()
{
  startSession();
  int position = 1;

  int hours = myRTC.hours;
  int minutes = myRTC.minutes;
  int dayOfWeek = myRTC.dayofweek;
  int dayOfMount = myRTC.dayofmonth;
  int mount = myRTC.month;
  int year = myRTC.year;
  int blink = 0;
  while (!endSession())
  {
    buttonUp.loop();
    buttonDown.loop();
    buttonBack.loop();
    buttonEnter.loop();
    blink++;

    if (blink < 10)
    {
      String firstLine = clockFiveSymbols(hours, minutes);
      firstLine += " Day:";
      firstLine += daysOfWeek(dayOfWeek);
      String secondLine = dateTimeTenSymbol(dayOfMount, mount, year);
      customLines(firstLine, secondLine);
    }
    else if (blink >= 10)
    {
      switch (position)
      {
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("__");
        break;
      case 2:
        lcd.setCursor(3, 0);
        lcd.print("__");
        break;
      case 3:
        lcd.setCursor(10, 0);
        lcd.print("__");
        break;
      case 4:
        lcd.setCursor(0, 1);
        lcd.print("__");
        break;
      case 5:
        lcd.setCursor(3, 1);
        lcd.print("__");
        break;
      case 6:
        lcd.setCursor(6, 1);
        lcd.print("____");
        break;
      default:
        break;
      }
    }
    if (blink >= 20)
    {
      blink = 0;
    }

    if (buttonBackIsPressed())
    {
      startSession();
      break;
    }

    if (buttonDownIsPressed())
    {
      startSession();
      switch (position)
      {
      case 1:
        hours--;
        hours = hours % 24;
        if (hours < 0)
        {
          hours = 23;
        }

        break;
      case 2:
        minutes--;
        minutes = minutes % 60;
        if (minutes < 0)
        {
          minutes = 59;
        }

        break;
      case 3:
        dayOfWeek--;
        if (dayOfWeek < 1)
        {
          dayOfWeek = 7;
        }
        if (dayOfWeek > 7)
        {
          dayOfWeek = 1;
        }
        break;
      case 4:
        dayOfMount--;
        if (dayOfMount < 1)
        {
          dayOfMount = 31;
        }
        if (dayOfMount > 31)
        {
          dayOfMount = 1;
        }

        break;
      case 5:
        mount--;
        if (mount < 1)
        {
          mount = 12;
        }
        if (mount > 12)
        {
          mount = 1;
        }

        break;
      case 6:
        year--;
        break;
        ;
      default:
        break;
      }
    }
    else if (buttonUpIsPressed())
    {
      startSession();
      switch (position)
      {
      case 1:
        hours++;
        hours = hours % 24;
        break;
      case 2:
        minutes++;
        minutes = minutes % 60;
        break;
      case 3:
        dayOfWeek++;
        if (dayOfWeek < 1)
        {
          dayOfWeek = 7;
        }
        if (dayOfWeek > 7)
        {
          dayOfWeek = 1;
        }
        break;
      case 4:
        dayOfMount++;
        if (dayOfMount < 1)
        {
          dayOfMount = 31;
        }
        if (dayOfMount > 31)
        {
          dayOfMount = 1;
        }
        break;
      case 5:
        mount++;
        if (mount < 1)
        {
          mount = 12;
        }
        if (mount > 12)
        {
          mount = 1;
        }
        break;
      case 6:
        year++;
        break;
      default:
        break;
      }
    }
    else if (buttonEnterIsPressed())
    {
      startSession();
      position++;
      if (position > 6)
      {
        startSession();
        // seconds, minutes, hours, day of the week, day of the month, month, year
        myRTC.setDS1302Time(0, minutes, hours, dayOfWeek, dayOfMount, mount, year);
        customLines("SUCCESSFUL !!!", " ");
        delay(1000);
        break;
      }
    }
  }
}
String daysOfWeek(int day)
{
  switch (day)
  {
  case 1:
    return "MO";
    break;
  case 2:
    return "TU";
    break;
  case 3:
    return "WE";
    break;
  case 4:
    return "TH";
    break;
  case 5:
    return "FR";
    break;
  case 6:
    return "SA";
    break;
  case 7:
    return "SU";
    break;

  default:
    return "";
    break;
  }
}

void openSetting(int position)
{
  switch (position)
  {
  case 0:
    settingsDateTime();
    break;
  case 1:
    settingsPumpTimers();
    break;
  case 2:
    SettingsBacklight();
    break;
  case 3:
    settingsTemperature();
    break;
  default:
    return;
    break;
  }
}
void printSettings(int position)
{
  switch (position)
  {
  case 0:
    customLines(">DateTime", " Pump timers");
    break;
  case 1:
    customLines(">Pump timers", " BackLight");
    break;
  case 2:
    customLines(">BackLight", " Temp");
    break;
  case 3:
    customLines(">Temp", " DateTime");
    break;
  default:
    customLines("Error", String(position));
    break;
  }
}

void customLines(String firstLine, String secondLine)
{
  for (size_t i = firstLine.length() - 1; i < 16; i++)
  {
    firstLine += " ";
  }

  for (size_t i = secondLine.length() - 1; i < 16; i++)
  {
    secondLine += " ";
  }

  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
}

void tenMinutesPumpON()
{
  tenMinutesPump.start = myRTC.hours * 60 + myRTC.minutes;
  tenMinutesPump.end = tenMinutesPump.start + 10;
}

void pumpControl()
{
  bool activePump = false;
  int time = myRTC.hours * 60 + myRTC.minutes;
  for (size_t i = 0; i < 10; i++)
  {
    if (timers[i].active)
    {
      int dayOfWeek = myRTC.dayofweek - 1;
      if (timers[i].days[dayOfWeek])
      {
        if ((timers[i].start <= time && timers[i].end >= time))
        {
          activePump = true;
        }
      }
    }
  }
  if (tenMinutesPump.start <= time && tenMinutesPump.end >= time)
  {
    activePump = true;
  }
  else
  {
    tenMinutesPump.start = 1555;
    tenMinutesPump.end = -1;
  }

  if (activePump)
  {
    digitalWrite(pumpPin, HIGH);
    pumpIsOn = true;
  }
  else
  {
    digitalWrite(pumpPin, LOW);
    pumpIsOn = false;
  }
}

String twoDigit(byte num){
  String result ="";
  if (num < 10)
  {
    result += " " + String(num);
  }
  else if (num >= 10)
  {
    result += String(num);
  }
  else if (num > 99 || num < 0)
  {
    result += "9*";
  }
  return result;
}

// Return time in 5 symbols
String clockFiveSymbols()
{
  int hour = myRTC.hours;
  int minutes = myRTC.minutes;

  String result = "";
  if (hour < 10)
  {
    result += "0";
    result += hour;
  }
  else
  {
    result += hour;
  }

  result += ":";

  if (minutes < 10)
  {
    result += "0";
    result += minutes;
  }
  else
  {
    result += minutes;
  }
  return result;
}

String clockFiveSymbols(int hour, int minutes)
{
  String result = "";
  if (hour < 10)
  {
    result += "0";
    result += hour;
  }
  else
  {
    result += hour;
  }

  result += ":";

  if (minutes < 10)
  {
    result += "0";
    result += minutes;
  }
  else
  {
    result += minutes;
  }
  return result;
}

String dateTimeTenSymbol(int day, int mounth, int year)
{
  String result = "";
  if (day < 10)
  {
    result += "0";
    result += day;
  }
  else
  {
    result += day;
  }

  result += "/";

  if (mounth < 10)
  {
    result += "0";
    result += mounth;
  }
  else
  {
    result += mounth;
  }
  result += "/";
  result += String(year);
  return result;
}

String tempInFourSymbols(int temp)
{
  return String((float)temp / 10, 1);
}

void printFrontScreen()
{
  String firstRow = "";
  firstRow += tempInFourSymbols(tempSolar) + " ";
  firstRow += pumpStatusInFiveSymbol() + " ";
  firstRow += clockFiveSymbols();

  lcd.setCursor(0, 0);
  lcd.print(firstRow);
  String secondRow = "";
  secondRow += String((float)tempB1 / 10, 1);
  secondRow += "  ";
  secondRow += String((float)tempB2 / 10, 1);
  secondRow += "  ";
  secondRow += String((float)tempB3 / 10, 1);

  lcd.setCursor(0, 1);
  lcd.print(secondRow);
}

void startSession()
{
  lcd.backlight();
  myRTC.updateTime();
  homeScreenTime = (myRTC.seconds + backLightTime) % 60;
}

bool endSession()
{
  bool end = false;
  myRTC.updateTime();
  if (myRTC.seconds == homeScreenTime)
  {
    lcd.noBacklight();
    homeScreenTime = -1;
    return true;
  }
  else if (homeScreenTime == ((byte)-1))
  {
    return true;
  }

  return end;
}

String pumpStatusInFiveSymbol()

{
  String result = "";
  result += "P:";
  if (pumpIsOn)
  {
    result += "ON ";
  }
  else
  {
    result += "OFF";
  }
  return result;
}

bool buttonUpIsPressed()
{
  if (buttonUp.getState() == LOW)
  {
    delay(350);
    return true;
  }
  return false;
}

bool buttonDownIsPressed()
{
  if (buttonDown.getState() == LOW)
  {
    delay(350);
    return true;
  }
  return false;
}

bool buttonBackIsPressed()
{
  if (buttonBack.getState() == LOW)
  {
    delay(350);
    return true;
  }
  return false;
}

bool buttonEnterIsPressed()
{
  if (buttonEnter.getState() == LOW)
  {
    delay(350);
    return true;
  }
  return false;
}
