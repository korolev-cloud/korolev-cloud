/*
 * Описание работы
 * Arduino Uno => HW-262 => Relay module 4
 * При включении инициализируется таймер на 60 минут
 * после окончания работы таймера включается реле на 200мс
 * контактами реле включается нагреватель
 * затем снова стартует таймер на 60 минут
 */
#include <TimerOne.h>
#include <Wire.h>
#include <EEPROMex.h>
#include <MultiFuncShield.h>

#define TIMER_VALUE_MAX 99
#define PIN_RELAY_1  5 // Arduino пин, подключенный через IN5 к реле на кнопку "Power" печки
#define PIN_RELAY_2  6 // Arduino пин, подключенный через IN6 к реле на кнопку "+" печки
#define PIN_RELAY_3  7 // Arduino пин, подключенный через IN6 к реле на кнопку "-" печки
#define PIN_RELAY_4  8 // Arduino пин, подключенный через IN6 к реле на кнопку "Timer" печки

enum CountDownModeValues
{
  COUNTING_STOPPED,
  COUNTING
};

// задаем режим работы при старте
byte countDownMode = COUNTING;


byte tenths = 0;
char seconds = 0;
char minutes = 0;

void saveTimer(char minutes, char seconds) {
  EEPROM.writeByte(0, seconds);
  EEPROM.writeByte(sizeof(byte), minutes);
}

void relayManage(){
  //продление нагрева печки нажатиями кнопок "Timer" и "+"
  // включить реле4 на 50мс
  digitalWrite(PIN_RELAY_4, HIGH);
  delay(50);
  // выключить реле4
  digitalWrite(PIN_RELAY_4, LOW);
  delay(400);
  // включить реле2 36 раз на 50мс
  for(int i = 36; i > 0; i--){
    digitalWrite(PIN_RELAY_2, HIGH);
    delay(50);
    // выключить реле2
    digitalWrite(PIN_RELAY_2, LOW);
    delay(50);
  }

}

void loadTimer() {
  seconds = EEPROM.readByte(0);
  minutes = EEPROM.readByte(sizeof(byte));
  if((seconds+minutes)<0){
    seconds=0;
    minutes=0;
  }
}

void checkStopConditions(byte btn) {

  if (btn == BUTTON_1_SHORT_RELEASE && (minutes + seconds) > 0) {
    countDownMode = COUNTING; // start the timer
    MFS.beep(6, 2, 3);  // beep 3 times, 600 milliseconds on / 200 off
    saveTimer(minutes,seconds);
  }
  else if (btn == BUTTON_1_LONG_PRESSED) {
    tenths  = 0; // reset the timer
    seconds = 0;
    minutes = 0;
    MFS.beep(8, 10, 1);  // beep 1 times, 800 milliseconds on / 1000 off
  }
  else if (btn == BUTTON_2_PRESSED || btn == BUTTON_2_LONG_PRESSED) {
    minutes++;
    if (minutes > TIMER_VALUE_MAX) minutes = 0;
  }
  else if (btn == BUTTON_3_PRESSED || btn == BUTTON_3_LONG_PRESSED) {
    seconds += 1; // continue counting down
    if (seconds >= 60) seconds = 0;
  }

}

void checkCountDownConditions (byte btn) {

  if (btn == BUTTON_1_SHORT_RELEASE || btn == BUTTON_1_LONG_RELEASE) {
    countDownMode = COUNTING_STOPPED; // stop the timer
    MFS.beep(6, 2, 2);  // beep 6 times, 200 milliseconds on / 200 off
  }
  else { 

    tenths++; // continue counting down

    if (tenths == 10) {

      tenths = 0;
      seconds--;

      if (seconds < 0 && minutes > 0) {
        seconds = 59;
        minutes--;
      }

      if (minutes == 0 && seconds == 0) {
        // timer has reached 0, so sound the alarm
        MFS.beep(50, 50, 3);  // beep 3 times, 500 milliseconds on / 500 off
        relayManage(); //управление печкой через контакты реле
        loadTimer();
        // countDownMode = COUNTING_STOPPED;
      }

    }

    delay(100);

  }

}

void display (char min, char sec){
  // display minutes and seconds sepated with a point
  MFS.write((float)((minutes*100 + seconds)/100.0),2);
}

void setup() {
  // put your setup code here, to run once:
  Timer1.initialize();
  MFS.initialize(&Timer1);    // initialize multifunction shield library
  MFS.write(0);
  // Инициализируем пин реле как выход.
  pinMode(PIN_RELAY_1, OUTPUT);
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
        
    case COUNTING:
        checkCountDownConditions(btn);
        MFS.blinkDisplay(DIGIT_1 | DIGIT_2 | DIGIT_3 | DIGIT_4, OFF);
        break;
  }

  display(minutes,seconds);

}
