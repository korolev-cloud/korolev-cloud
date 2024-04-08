/*
 * Описание работы
 * Arduino Uno => HW-262 => Relay module 4
 * При включении инициализируется таймер на 60 минут
 * после окончания работы таймера включается реле на 200мс
 * контактами реле включается нагреватель
 * затем снова стартует таймер на 60 минут
 */
#include <TimerOne.h>
#include <microDS18B20.h>
#include <Wire.h>
#include <EEPROMex.h>
#include <MultiFuncShield.h>

#define TIMER_VALUE_MAX 99
#define PIN_RELAY_1  5 // Arduino пин, подключенный через IN5 к реле на кнопку "Power" печки
#define PIN_RELAY_2  6 // Arduino пин, подключенный через IN6 к реле на кнопку "+" печки
#define PIN_RELAY_3  9 // Arduino пин, подключенный через IN6 к реле на кнопку "-" печки
#define PIN_RELAY_4  A5 // Arduino пин, подключенный через IN6 к реле на кнопку "Timer" печки

enum RunningModeValues
{
  INIT,
  SETUP,
  HEATING_DISTIL,
  DISTIL,
  STOP_DISTIL,
  HEATING_RECTIF,
  STABILE_RECTIF,
  STOP_RECTIF
};

// задаем режим работы при старте
byte RunningMode = SETUP;

MicroDS18B20<2> sensor;

float tempSensor = 0; //переменная температуры
float tempStopDistillers = 94.0f; //стоп дистилляции
float tempStabileColumn = 70.0f; //стабилизация колонны
unsigned long timerHeat = 7200000; //время перезапуска таймера печки
unsigned long time = 0; //переменная старта таймера
bool timerRestart = 0;

/*
void saveTemp(float stopDistil, char stabileColumn) {
  EEPROM.writeByte(0, stopDistil);
  EEPROM.writeByte(sizeof(byte), stabileColumn);
}

void readTemp(float stopDistil, char stabileColumn) {
  EEPROM.readByte(0, stopDistil);
  EEPROM.readByte(sizeof(byte), stabileColumn);
}
*/

void restartTimerHeating(){
  //продление нагрева печки нажатиями кнопок "Timer" и "+"
  //включить реле4 на 50мс
  digitalWrite(PIN_RELAY_4, HIGH);
  delay(50);
  // выключить реле4
  digitalWrite(PIN_RELAY_4, LOW);
  delay(400);
  // включить реле2 45 раз на 100мс
  for(int i = timerRestart ? 1 : 45; i > 0; i--){
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(100);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
    delay(100);
    timerRestart = 1;
  }

}

void powerDownStabile(){
  //уменьшение мощности печки для стабилизации колонны при ректификации нажатиями кнопок "-" 6 раз
  //delay(2000);
  for(int i = 6; i > 0; i--){
    //включить реле3 на 100мс
    digitalWrite(PIN_RELAY_3, HIGH);
    delay(100);
    // выключить реле3
    digitalWrite(PIN_RELAY_3, LOW);
    delay(100);
  }
}

void startHeating(){
  // включение печки и выбор мощности нагрева 2700 после старта на 2000
  // включить реле1
  digitalWrite(PIN_RELAY_1, HIGH);
  delay(100);
  // выключить реле1
  digitalWrite(PIN_RELAY_1, LOW);
  delay(2000);
  for(int i = 3; i > 0; i--){
    // включить реле2
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(100);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
    delay(100);
  }
  MFS.write ("2700");
  delay(1000);
  MFS.write ("WATT");
  delay(1000);
}

void stopHeating(){
  // полное отключение печки
  // включить реле1
  digitalWrite(PIN_RELAY_1, HIGH);
  delay(100);
  // выключить реле1
  digitalWrite(PIN_RELAY_1, LOW);
  delay(100);
}


void loadTimer() {
  // время перезапуска таймера индукционной печки
  time = millis();
}



void checkSetupConditions(byte btn) {
  // отработка нажатия кнопки 1 в режиме SETUP
  MFS.write(RunningMode);
  if (btn == BUTTON_1_PRESSED) {
    //RunningMode
    //countDownMode = WORK; // start the timer
  }
}




void setup() {
  Timer1.initialize();
  MFS.initialize(&Timer1);    // initialize multifunction shield library
  MFS.write(0);
  pinMode(PIN_RELAY_1, OUTPUT); // Инициализируем пин реле как выход.
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT); 
  pinMode(PIN_RELAY_4, OUTPUT); 
  digitalWrite(PIN_RELAY_1, LOW);
  digitalWrite(PIN_RELAY_2, LOW);
  digitalWrite(PIN_RELAY_3, LOW);
  digitalWrite(PIN_RELAY_4, LOW);
  loadTimer();
}

void loop() {
  byte btn = MFS.getButton();
  switch (RunningMode)
  {
    case SETUP:
        checkSetupConditions(btn);
        //MFS.write("____");
        //MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4);
        break;
    /*
        case WORK:
            checkCountDownConditions(btn);
            MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4, OFF);
            sensor.readTemp();
            tempSensor = sensor.getTemp();
            MFS.write(tempSensor, 2);
            break;

        case SETUP:
            checkSetupConditions(btn);
            MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4);
            break;

        case END:
            checkEndConditions(btn);
            MFS.write ("END");
            MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4);
            break;

        case STABILE:
            checkCountDownConditions(btn);
            MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4, OFF);
            sensor.readTemp();
            //tempSensor = sensor.getTemp() * 3;
            tempSensor = sensor.getTemp();
            MFS.write(tempSensor, 2);
            checkStabileConditions(btn);
            break;

        void checkSetupConditions(byte btn) {
      // отработка нажатия кнопки 1 "Старт" в режиме "Setup"
      if (btn == BUTTON_1_SHORT_RELEASE) {
        countDownMode = WORK; // изменен режим на Работа
        MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4, OFF);
        MFS.write ("STRT");
        delay(2000);
        MFS.write (int (timerHeat/1000));
        //MFS.write ((minutes * 60) + seconds);
        delay(2000);
        MFS.write ("sec");
        delay(2000);
        startHeating();
        MFS.beep(1, 2, 3);  // beep 1 times, 600 milliseconds on / 200 off
      }
      else if (btn == BUTTON_2_PRESSED) {
        // изменение режима работы на дист
        countingMode = DIST;
        MFS.write ("DIST");
        //delay(1000);
      }
      else if (btn == BUTTON_3_PRESSED) {
        // изменение режима работы на рект 
        countingMode = RECT;
        MFS.write ("RECT");
        //delay(1000);
      }
    }

    void checkStabileConditions (byte btn) {
      // отработка нажатия кнопки 2 "Фиксация температуры" в режиме "STABILE"
      if (btn == BUTTON_2_PRESSED) {
        //countDownMode = COUNTING_STOPPED; // stop the timer
        MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
        tempStabileColumn = tempSensor;
        MFS.write (tempSensor);
        MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4);
        delay(2000);
      }
    }

    void checkCountDownConditions (byte btn) {
      // отработка нажатия кнопки 1 "Стоп" в режиме "WORK"
      if (btn == BUTTON_1_SHORT_RELEASE || btn == BUTTON_1_LONG_RELEASE) {
        //countDownMode = COUNTING_STOPPED; // stop the timer
        MFS.write ("STOP");
        delay(1000);
        MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
        
      }
      else { 
        //тело цикла в режиме "WORK"
        if (countingMode == DIST && tempSensor > tempStopDistillers){
          MFS.write ("END");
          delay(5000);
          MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
          stopHeating();
          countDownMode = END;
        }
        
        //тело цикла в режиме "RECT"
        else if (countingMode == RECT && tempSensor > tempStabileColumn){
          //снижение мощности до 1300 для стабилизации колонны
          MFS.write ("1300");
          delay(5000);
          powerDownStabile();
          MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
          countingMode = STABILE;
          MFS.write ("STAB");
          delay(5000);
        }
        
        //тело цикла в режиме "STABILE"
        else if (countingMode == STABILE && tempSensor > tempStabileColumn){
          //стабилизация колонны
          //MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
        }

        // чтение температуры с датчика
        sensor.requestTemp();
        if (millis() - time >= timerHeat) {
          MFS.beep(50, 50, 3);  // beep 3 times, 500 milliseconds on / 500 off
          MFS.write ("RST"); delay(1000);
          restartTimerHeating(); //перезапуск таймера печки
          loadTimer();
          }

        delay(100);

      }

    }

    void checkEndConditions(byte btn) {
      // отработка нажатия кнопки 1 "Старт" в режиме "Setup"
      if (btn == BUTTON_1_SHORT_RELEASE) {
        MFS.write(
      }
    }
        */
  }
}
