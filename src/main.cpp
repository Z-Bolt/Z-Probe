#include <Arduino.h>
#include <EEPROM.h>
#include "HX711.h"

// Constants
#define DEBUG 1

#define CALIBRATION_STATE 1
#define MESUREMENT_STATE 0

// Configuration
#define EEPROM_ADDRESS 0

#define LED_PIN 13
#define ENDSTOP_PIN 17
#define CALIBRATION_PIN A2

#define THRESHOLD 250
#define THRESHOLD_LOOP 3
#define HORIZON 100
#define CALIBRATION_WEIGHT 50

void init_calibration(void);
void complete_calibration(void);
void calibration(void);

void init_mesurement(void);
void mesurement(void);
void complete_mesurement(void);

float eeprom_get_scale(void);
void eeprom_set_scale(float scale);

void(* resetFunc) (void) = 0;//declare reset function at address 0

int CURRENT_STATE = MESUREMENT_STATE;
int NEW_STATE = -1;

HX711 scale(A1, A0);

// Calibration
struct {
  int indicator_counter = 0;
  double cur_value = 0;
  double av_value = 0;
} c;


// Mesurement
struct {
  int under_presser_loops = 0;
  int counter = 0;
  double diff_value = 0;
  double cur_value = 0;
  double av_value = 0;
} m;


void setup() {
  #if defined (DEBUG)
    Serial.begin(19200);
  #endif

  pinMode(CALIBRATION_PIN, INPUT);
  digitalWrite(LED_PIN, HIGH);

  init_mesurement();
}


void loop() {
  NEW_STATE = digitalRead(CALIBRATION_PIN);

  if (NEW_STATE == CALIBRATION_STATE && NEW_STATE != CURRENT_STATE) {
    // Init calibration process
    #if defined (DEBUG)
      Serial.println("Init calibration process");
      init_calibration();
    #endif

  } else if (NEW_STATE == CALIBRATION_STATE && NEW_STATE == CURRENT_STATE) {
    // Perform calibration process
    calibration();
  } else if (NEW_STATE == MESUREMENT_STATE && NEW_STATE != CURRENT_STATE) {
    // Complete calibration process

    #if defined (DEBUG)
      Serial.println("Complete calibration process");
    #endif

    complete_calibration();

  } else if (NEW_STATE == MESUREMENT_STATE && NEW_STATE == CURRENT_STATE) {
    // Regurlar work
    mesurement();
  }

  CURRENT_STATE = NEW_STATE;
}


void init_calibration() {
  c.av_value = 0;
  scale.set_scale();
  scale.tare();
}


void calibration() {
  c.cur_value = scale.get_units();
  c.av_value = (c.av_value * (HORIZON * 2 - 1) + c.cur_value) / (HORIZON * 2);

  #if defined (DEBUG)
    Serial.print(c.cur_value);
    Serial.print("\t|\t");
    Serial.print(c.av_value);
    Serial.println(" - calibration value");
  #endif

  c.indicator_counter++;

  if (c.indicator_counter > 10){
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    c.indicator_counter = 0;
  }
}


void complete_calibration() {
  float scale = c.av_value / CALIBRATION_WEIGHT;

  #if defined (DEBUG)
    Serial.print(scale);
    Serial.println(" - new calibration value");
  #endif

  eeprom_set_scale(scale);
}


void init_mesurement() {  
  float s = eeprom_get_scale();

  #if defined (DEBUG)
    Serial.print("\n\n\n\n");
    Serial.print(s);
    Serial.println(" - mesure scale value");
  #endif  

  scale.set_scale(s);
  scale.tare();				     
}


void mesurement() {
  m.cur_value = scale.get_units();
  // m.diff_value = m.av_value - m.cur_value;
  m.diff_value = m.cur_value - m.av_value; // if sensor is inverted

  if (m.diff_value > THRESHOLD) {
    m.under_presser_loops += 1;
  } else {
    m.under_presser_loops = 0;
  }

  if (m.under_presser_loops > THRESHOLD_LOOP) {
    digitalWrite(LED_PIN, LOW);
    m.counter = 10;
  }

  if (m.counter > 0){
    if (m.counter == 1){
      digitalWrite(LED_PIN, HIGH);
    }

    #if defined (DEBUG)
      Serial.println("----- Touching detected !!! ------");
    #endif
    
    m.counter--;
  }

  #if defined (DEBUG)
    Serial.print("av: ");
    Serial.print(m.av_value);
    Serial.print("\t|diff: ");
    Serial.print(m.diff_value);
    Serial.print("\t|cur: ");
    Serial.print(m.cur_value);
    Serial.println("");
  #endif

  m.av_value = (m.av_value * (HORIZON - 1) + m.cur_value) / HORIZON;
}


float eeprom_get_scale(){
  float scale;
  return EEPROM.get(EEPROM_ADDRESS, scale);
}


void eeprom_set_scale(float scale){
  EEPROM.put(EEPROM_ADDRESS, scale);
}


