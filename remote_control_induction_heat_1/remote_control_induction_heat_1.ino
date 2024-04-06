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

enum CountDownModeValues
{
  COUNTING_STOPPED,
  WORK,
  SETUP,
  END,
  STABILE
};

enum CountingModeValues
{
  DIST,
  RECT
};

// задаем режим работы при старте
byte countDownMode = SETUP;
byte countingMode = DIST;

//char seconds = 0;
//char minutes = 0;
MicroDS18B20<2> sensor;


float tempSensor = 0; //переменная температуры
float tempStopDistillers = 94.0f; //стоп дистилляции
float tempStabileColumn = 70.0f; //стабилизация колонны
unsigned long timerHeat = 3600000; //время перезапуска таймера печки
unsigned long time = 0; //переменная старта таймера


void saveTimer(char minutes, char seconds) {
  EEPROM.writeByte(0, seconds);
  EEPROM.writeByte(sizeof(byte), minutes);
}

void restartTimerHeating(){
  //продление нагрева печки нажатиями кнопок "Timer" и "+"
  //включить реле4 на 50мс
  digitalWrite(PIN_RELAY_4, HIGH);
  delay(50);
  // выключить реле4
  digitalWrite(PIN_RELAY_4, LOW);
  delay(400);
  // включить реле2 45 раз на 100мс
  for(int i = 45; i > 0; i--){
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(100);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
    delay(100);
  }

}

void powerDownStabile(){
  //уменьшение мощности печки для стабилизации колонны при ректификации нажатиями кнопок "-" 6 раз
  delay(2000);
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
  delay(50);
  // выключить реле1
  digitalWrite(PIN_RELAY_1, LOW);
  delay(5000);
  for(int i = 3; i > 0; i--){
    // включить реле2
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(100);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
    delay(100);
  }
  MFS.write ("2700");
  delay(2000);
  MFS.write ("WATT");
  delay(2000);
}

void stopHeating(){
  // полное отключение печки
  // включить реле1
  digitalWrite(PIN_RELAY_1, HIGH);
  delay(50);
  // выключить реле1
  digitalWrite(PIN_RELAY_1, LOW);
  delay(50);
}


void loadTimer() {
  // время перезапуска таймера индукционной печки
  //seconds=60;
  //minutes=0;
  time = millis();
}

void checkStopConditions(byte btn) {
  // отработка нажатия кнопки 1 "СТАРТ" в режиме COUNTING_STOPPED
  if (btn == BUTTON_1_SHORT_RELEASE) {
    countDownMode = WORK; // start the timer
    MFS.beep(6, 2, 3);  // beep 3 times, 600 milliseconds on / 200 off
    //saveTimer(minutes,seconds);
  }
  

}

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
    countDownMode = SETUP;
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
  switch (countDownMode)
  {
    case COUNTING_STOPPED:
        checkStopConditions(btn);
        MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4);
        break;
        
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
        break;
  }
  

}
