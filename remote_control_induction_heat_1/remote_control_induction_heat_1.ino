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
  END
};

enum CountingModeValues
{
  DIST,
  RECT
};

// задаем режим работы при старте
byte countDownMode = SETUP;
byte countingMode = DIST;

byte tenths = 0;
char seconds = 0;
char minutes = 0;
MicroDS18B20<2> sensor;


float tempSensor = 0; //переменная температуры
float tempStopDistillers = 98.0f; //стоп дистилляции

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
  // включить реле2 36 раз на 50мс
  for(int i = 45; i > 0; i--){
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(100);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
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
  seconds=60;
  minutes=0;
}

void checkStopConditions(byte btn) {
  // отработка нажатия кнопки 1 "СТАРТ" в режиме COUNTING_STOPPED
  if (btn == BUTTON_1_SHORT_RELEASE && (minutes + seconds) > 0) {
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
    MFS.write ("HEAT");
    delay(2000);
    MFS.write ((minutes * 60) + seconds);
    delay(2000);
    MFS.write ("sec");
    delay(2000);
    startHeating();
    MFS.beep(1, 2, 3);  // beep 1 times, 600 milliseconds on / 200 off
  }
  else if (btn == BUTTON_2_PRESSED && countingMode == DIST) {
    // изменение режима работы с дист на рект
    countingMode = RECT;
  }
  else if (btn == BUTTON_2_PRESSED && countingMode == RECT) {
    // изменение режима работы с рект на дист 
    countingMode = DIST;
  }
  if (countingMode == DIST) 
  {
    MFS.write ("DIST");
  }
  else 
  {
    MFS.write ("RECT");
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
    // выполнение обратного отсчета таймера
    tenths++; // continue counting down
    // чтение температуры с датчика
    sensor.requestTemp();
    if (tenths == 10) {

      tenths = 0;
      seconds--;

      if (seconds < 0 && minutes > 0) {
        seconds = 59;
        minutes--;
      }

      if (minutes == 0 && seconds == 0) {
        MFS.beep(50, 50, 3);  // beep 3 times, 500 milliseconds on / 500 off
        MFS.write ("REST"); delay(300);
        MFS.write ("ESTA"); delay(300);
        MFS.write ("STAR"); delay(300);
        MFS.write ("TART"); delay(300);
        MFS.write ("ART "); delay(300);
        MFS.write ("RT H"); delay(300);
        MFS.write ("T HE"); delay(300);
        MFS.write (" HEA"); delay(300);
        MFS.write ("HEAT"); delay(300);
        restartTimerHeating(); //перезапуск таймера печки
        loadTimer();
      }

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

void display (char min, char sec){
  MFS.write((float)((minutes*100 + seconds)/100.0),2); // отображение времени работы таймера
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
        MFS.write(sensor.getTemp(), 2);
        // display(minutes,seconds);
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
  }
  

}
