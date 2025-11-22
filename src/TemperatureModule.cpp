// FileName: TemperatureModule.cpp

#include "TemperatureModule.h"
#include "Config.h"
#include <Arduino.h>
#include <EEPROM.h>

TemperatureModule::TemperatureModule() : 
  myPID(&pidInput, &pidOutput, &pidSetpoint, TEMP_KP, TEMP_KI, TEMP_KD, DIRECT)
{
  kp = TEMP_KP;
  ki = TEMP_KI;
  kd = TEMP_KD;
  currentTemp = 0.0;
}

void TemperatureModule::init() {
  pinMode(TEMP_HEATER_PIN, OUTPUT);
  // LÓGICA NORMAL: LOW es APAGADO
  digitalWrite(TEMP_HEATER_PIN, LOW); 

  pidSetpoint = 0; 

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 255); 
  myPID.SetSampleTime(PID_SAMPLE_TIME);
}

void TemperatureModule::setTargetTemp(int sp) {
  pidSetpoint = sp;
}

int TemperatureModule::getTargetTemp() {
  return (int)pidSetpoint;
}

// En TemperatureModule.cpp

double TemperatureModule::getTemp() {
  long sum = 0;
  
  // Leemos varias veces RÁPIDO (sin delay) para promediar ruido eléctrico
  // Reducimos las muestras a 10 o 20 para no bloquear
  int muestras = 20; 
  
  for (int i = 0; i < muestras; i++) {
    sum += analogRead(TEMP_THERMISTOR_PIN);
    // ¡ELIMINAMOS EL delay(5)!
  }
  
  float lecturaProm = sum / (float)muestras;

  // ... (el resto de la matemática sigue igual) ...
  float Vout = lecturaProm * VCC / 1023.0;
  if (Vout >= VCC) Vout = VCC - 0.001;
  float Rntc = (Vout * R_REF) / (VCC - Vout);
  float tempK = 1.0 / ((1.0 / NTC_T0) + (1.0 / NTC_BETA) * log(Rntc / NTC_R0));
  float tempC = tempK - 273.15 - TEMP_OFFSET;
  
  return tempC;
}

void TemperatureModule::update() {
  // 1. Medimos la temperatura actual
  currentTemp = getTemp(); 

  // 2. SEGURIDAD: Si el objetivo es 0 (o negativo), apagamos a la fuerza.
  // Esto evita que el PID intente "calentar un poquito" si hay ruido.
  if (pidSetpoint <= 0) {
    digitalWrite(TEMP_HEATER_PIN, LOW); // APAGADO (Lógica Normal)
    return; // Salimos de la función, no calculamos PID
  }

  // 3. PID: Alimentamos el algoritmo
  pidInput = currentTemp;
  
  // La librería decide si ya pasó el tiempo (PID_SAMPLE_TIME) para recalcular
  myPID.Compute(); 
  
  // 4. APLICAR POTENCIA
  // Lógica Normal: Mayor valor de salida PID (0-255) = Más voltaje al pin
  analogWrite(TEMP_HEATER_PIN, (int)pidOutput);
}

// --- Funciones de Configuración y EEPROM ---

void TemperatureModule::setTunings(double newKp, double newKi, double newKd) {
    myPID.SetTunings(newKp, newKi, newKd);
    this->kp = newKp;
    this->ki = newKi;
    this->kd = newKd;
}

void TemperatureModule::saveSettingsToEEPROM(int targetToSave) {
    // Guardamos el valor que nos pasan por parámetro (ej. 200)
    // independientemente de si el calentador está prendido o apagado ahora.
    EEPROM.put(EEPROM_ADDR_TARGET_TEMP, targetToSave);

    // Guardamos PID
    EEPROM.put(EEPROM_ADDR_KP, kp);
    EEPROM.put(EEPROM_ADDR_KI, ki);
    EEPROM.put(EEPROM_ADDR_KD, kd);
}
void TemperatureModule::loadSettingsFromEEPROM() {
    int storedTarget;
    EEPROM.get(EEPROM_ADDR_TARGET_TEMP, storedTarget);
    
    if (storedTarget < 0 || storedTarget > 300) {
        pidSetpoint = 0; 
    } else {
        pidSetpoint = storedTarget;
    }

    double tempKp, tempKi, tempKd;
    EEPROM.get(EEPROM_ADDR_KP, tempKp);
    EEPROM.get(EEPROM_ADDR_KI, tempKi);
    EEPROM.get(EEPROM_ADDR_KD, tempKd);

    if (isnan(tempKp) || tempKp < 0) {
        kp = TEMP_KP; 
        ki = TEMP_KI; 
        kd = TEMP_KD;
    } else {
        kp = tempKp;
        ki = tempKi;
        kd = tempKd;
    }
    
    myPID.SetTunings(kp, ki, kd);
}