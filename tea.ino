//  Tea Alarm - t.trevethan 2016
//  Arduino Uno or Pro Mini
//
//  MLX9015 IR thermometor connected to pins 2 (SCL) and 3 (SDA)
//  ESP8266-01 connected to pins 10 (RX) and 11 (TX)
//  Red LED connected to pin 5
//  Blue LED connected to pin 6
//  Piezo buzzer connected to pin 8
//  Status LED connected to pin 9

//define the pins for software I2C
#define SDA_PORT PORTD
#define SDA_PIN 3   //define the SDA pin         
#define SCL_PORT PORTD
#define SCL_PIN 2   //define the SCL pin

//include libraries for IR thermometer, buzzer tones and EEPROM read and write
#include "MLX90615.h"
#include "pitches.h"
#include <EEPROM.h>

MLX90615 mlx90615;

// melodies for acknowledgements:
int melody1[] = {
  NOTE_A6, NOTE_E6};

int melody2[] = {
  NOTE_D7,NOTE_D7,NOTE_D7};

int melody3[] = {
  NOTE_E6, NOTE_A6};

//pins
int ledb = 6; // blue led
int ledr = 5; // red led
int buzz = 8; // buzzer
int statled = 9; // status

float otemp = 0.0; // object temperature
float oldtemp = 0.0; // previous object temperatures
float oldtemp2 = 0.0;
float bb = 0.0;  // blue temperature scale variable
float br = 0.0;  // red temperature scale variable
int brightnessb = 0;  // blue LED PWM intensity for analogueWrite()
int brightnessr = 0;  // red LED PWM intensity for analogueWrite()
int tcool = 0;  // time elapsed since tea placed on device (ms)

// the threshold temperature 
// retrieve from EEPROM address 1 and convert to float

int address = 1;
byte value;
float ttemp;

float tcold = 35.0; // the 'tea gone cold' threshold

// the device mode:
// 0: initialise - no cup
// 1: hot cup present
// 2: hot cup cooling
// 3: hot cup cooled past threshold - alarm
// 4: tea gone cold - send tweet
// 5: temperature setting mode

int tmode = 0; 

void setup()
{
  value = EEPROM.read(address);   // retrieve from EEPROM address 1
  ttemp = value;                  // convert to float
  Serial.begin(9600);             // UART serial for debugging
  Serial.println("Setup...");
  Serial.print("ttemp: ");
  Serial.println(ttemp);
  mlx90615.init();                // initialize soft i2c wires
  
  pinMode(ledb, OUTPUT);
  pinMode(ledr, OUTPUT);
  pinMode(statled, OUTPUT);
}

void loop()
{ 
    unsigned long currentMillis = millis(); // get the current time (in ms)

    oldtemp2 = oldtemp;  // save previous values of object temperature
    oldtemp = otemp;
    
    Serial.print("Object temperature: ");
    otemp = mlx90615.printTemperature(MLX90615_OBJECT_TEMPERATURE);  // get the object temperature
    Serial.println(otemp);

//  detect hot cup (either at startup or after being placed on detector) - this is done by detecting an increase in object temperature above the threshold
//  and place into mode 1
    if(otemp > ttemp && tmode == 0) {
      tmode = 1;
      Serial.println("Entering mode 1");
      //play acnowledgement
      for (int thisNote = 0; thisNote < 2; thisNote++) {
        tone(buzz, melody3[thisNote], 200);
        delay(200);
        noTone(buzz);
      }
    }

//  detect steady temperature from hot cup - temperature remaining above threshold for three cycles
//  and place into mode 2
    if(otemp > ttemp && oldtemp > ttemp && oldtemp2 > ttemp && tmode == 1) {
      tmode = 2;
      Serial.println("Entering mode 2");
      //play acnowledgement
      for (int thisNote = 0; thisNote < 2; thisNote++) {
        tone(buzz, melody1[thisNote], 200);
        delay(200);
        noTone(buzz);
      }
    tcool = currentMillis;  //  start the timer
    }

// detect cup removed from detector: sharp drop in temperature (> 10 degrees) over 1 cycle. 
// and place into mode 0
    if(otemp < (ttemp - 8.0) && oldtemp > ttemp && tmode == 2) {
      tmode = 0;
      Serial.println("Cup removed, entering mode 0");
      tcool = 0; // reset the timer
    }
      
//  temperature dropped below threshold 
//  and place into mode 3 and sound alarm
    if(otemp < ttemp && otemp > (ttemp - 8.0) && tmode == 2) {
      tmode = 3;
      Serial.println("Temperature alarm: entering mode 3");
      int timepassed = (currentMillis - tcool)/1000; //  time since mode 2 entered (s)
      int mins = timepassed/60;  // convert to minutes
      int secs = timepassed%60;  // and seconds
      Serial.print("Time cooling: ");
      Serial.print(mins);
      Serial.print(" minutes ");
      Serial.print(secs);
      Serial.print(" seconds ");
      
//    send SMS message via ThingHTTP

      //sound alarm (40 cycles)
      for (int thisNote = 0; thisNote < 40; thisNote++) {
        tone(buzz, melody2[0], 200);
        delay(200);
        noTone(buzz);
        delay(300);
        Serial.println("ALARM!");
        //if the cup is picked up, stop the alarm and enter mode 0
        // get the object temperature
        otemp = mlx90615.printTemperature(MLX90615_OBJECT_TEMPERATURE);
        if(otemp < 42.0) {
          thisNote = 40;
          tmode = 0;
          Serial.println("Cup picked up: alarm cancelled");
          tcool = 0;
        }
         
      }
    }      

//  temperature dropped below cold threshold 
//  and place into mode 0 and send tweet
    if(otemp < tcold && otemp > 30.0 && tmode == 3) {
      tmode = 0;
      Serial.println("Tea gone cold: entering mode 0");
      int timepassed = (currentMillis - tcool)/1000; //  time since mode 2 entered (s)
      int mins = timepassed/60;  // convert to minutes
      int secs = timepassed%60;  // and seconds

// send tweet


// make sad sound

      tcool = 0; //reset timer
    }      

// cup removed when in alarm mode
    if(otemp < 30.0 && tmode == 3) {
      tmode = 0;
      Serial.println("Cup removed: entering mode 0");
      tcool = 0; // reset timer
    }          
    
// light the RGB led according the object temperature in mode 1,2,3 - red for hot and blue for cold
    if(tmode == 1 || tmode == 2 || tmode == 3) {
      br = constrain((otemp - 20.0)*6.375,0,255);  // constrain the temperature range 20C - 60C to 0 - 255
      brightnessr = (int)br;  // convert to integers
      brightnessb = 255-br;
    }
    else { 
      brightnessr = 0;  //  or turn off in any other mode
      brightnessb = 0; 
    }
    
    analogWrite(ledr, brightnessr);
    analogWrite(ledb, brightnessb);

// enter threshold temperature programming mode: sub-zero (frozen object temperature for 3 cycles)
// mode 5
    if(otemp < 0.0 && oldtemp < 0.0 && oldtemp2 < 0.0 && tmode == 0) {
      tmode = 5;
      Serial.println("Programming mode 5");
      //play acnowledgement amd blink blue led three times
      for (int thisNote = 0; thisNote < 3; thisNote++) {
        tone(buzz, melody2[thisNote], 200);
        brightnessb = 255;
        analogWrite(ledb, brightnessb);
        delay(200);
        brightnessb = 0;
        analogWrite(ledb, brightnessb);
        noTone(buzz);
        delay(400);
      }
      delay(2000);
// loop 12 times to record program:
// object temperature is sampled every 2 seconds. If it is below zero, ttemp is reduced by 1. If it is above 40C, ttemp is increased by 1.  
      for(int pcount = 0; pcount < 12; pcount++) {
          otemp = mlx90615.printTemperature(MLX90615_OBJECT_TEMPERATURE);
          Serial.print("Object temp: ");
          Serial.println(otemp);
          if(otemp < 0.0) {
              ttemp = ttemp - 1;  // reduce the threshold temperature by 1 degree
              brightnessb = 255;
              analogWrite(ledb, brightnessb); // flash cold (blue) acknowedgement
              tone(buzz, NOTE_C7, 200); // beep once
              delay(200);
              brightnessb = 0;
              analogWrite(ledb, brightnessb);
              noTone(buzz);
              Serial.print("Reduce ttemp: ");
              Serial.println(ttemp);
          }
          else if(otemp > 40.0) {
              ttemp = ttemp + 1;  // increase the threshold temperature by 1 degree
              brightnessr = 255;
              analogWrite(ledr, brightnessr); // flash red (hot) acknowedgement
              tone(buzz, NOTE_C7, 200);  // beep once
              delay(200);
              brightnessr = 0;
              analogWrite(ledr, brightnessr);
              noTone(buzz);
              Serial.print("Increase ttemp: ");
              Serial.println(ttemp);
          }
          // if sampled object temperature between 0 and 40C, exit.  
          else {
            pcount = 12; // exit program mode
          }
      delay(2000);
      }
    Serial.print("Exiting program mode");
      //play acnowledgement and flash blue LED
      for (int thisNote = 0; thisNote < 3; thisNote++) {
        tone(buzz, melody2[thisNote], 200);
        brightnessb = 255;
        analogWrite(ledb, brightnessb);
        delay(100);
        brightnessb = 0;
        analogWrite(ledb, brightnessb);
        noTone(buzz);
        delay(100);
      }    
      tmode = 0;
      value = ttemp;
      EEPROM.write(address, value);  // write the new ttemp value to the EEPROM
    }          

    Serial.print("Mode: ");
    Serial.println(tmode);

// post to data thingspeak every 60 seconds if in mode 1,2 or 3

    delay(1000);  
}