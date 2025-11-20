// FileName: TemperatureModule.cpp

#include "TemperatureModule.h"
#include "Config.h"
#include <Arduino.h>
#include <EEPROM.h>

// El constructor inicializa el objeto PID usando una lista de inicialización
TemperatureModule::TemperatureModule() : 
  myPID(&pidInput, &pidOutput, &pidSetpoint, TEMP_KP, TEMP_KI, TEMP_KD, DIRECT)
{
  // Guardamos las constantes por si necesitamos reajustarlas en el futuro
  kp = TEMP_KP;
  ki = TEMP_KI;
  kd = TEMP_KD;

  currentTemp = 0.0;
}

void TemperatureModule::init() {
  pinMode(TEMP_HEATER_PIN, OUTPUT);
  digitalWrite(TEMP_HEATER_PIN, LOW); // Apagar el calentador al inicio

  pidSetpoint = 0; // Iniciar con el PID desactivado (objetivo 0°C)

  // Configurar la librería PID con los valores de Config.h
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 255); // Salida PWM de 0 a 255
  myPID.SetSampleTime(PID_SAMPLE_TIME);
}

void TemperatureModule::setTargetTemp(int sp) {
  pidSetpoint = sp;
}

int TemperatureModule::getTargetTemp() {
  return (int)pidSetpoint;
}

// Implementación de tu lógica precisa para leer la temperatura
double TemperatureModule::getTemp() {
  long sum = 0;
  for (int i = 0; i < THERMISTOR_SAMPLES; i++) {
    sum += analogRead(TEMP_THERMISTOR_PIN);
    delay(5); // Pequeño delay para estabilizar la lectura
  }
  float lecturaProm = sum / (float)THERMISTOR_SAMPLES;

  float Vout = lecturaProm * VCC / 1023.0;
  
  // Evitar división por cero si Vout es igual o mayor a VCC
  if (Vout >= VCC) {
    Vout = VCC - 0.001;
  }
  
  float Rntc = (Vout * R_REF) / (VCC - Vout);
  
  // Calcular temperatura con la ecuación de Beta
  float tempK = 1.0 / ((1.0 / NTC_T0) + (1.0 / NTC_BETA) * log(Rntc / NTC_R0));
  float tempC = tempK - 273.15 - TEMP_OFFSET;
  
  return tempC;
}

void TemperatureModule::update() {
  // SIEMPRE medimos y guardamos la temperatura actual para la UI
  currentTemp = getTemp(); // esta currentTemp es local al módulo de temperatura

  // Si el objetivo es 0 o menos, mostramos la temp pero mantenemos el calentador apagado
  if (pidSetpoint <= 0) {
    analogWrite(TEMP_HEATER_PIN, 0); // Calentador apagado
    return; // No corremos el PID, pero currentTemp ya quedó actualizado
  }

  // Alimentamos el PID con la última lectura estable
  pidInput = currentTemp;

  // Calculamos y aplicamos el control
  myPID.Compute(); // La librería decide si es hora de calcular y lo hace
  analogWrite(TEMP_HEATER_PIN, (int)pidOutput); //Aplicamos la salida PWM
}


// Función para ajustar las constantes del PID "en caliente"
void TemperatureModule::setTunings(double Kp, double Ki, double Kd) {
    myPID.SetTunings(Kp, Ki, Kd);
}
void TemperatureModule::saveSettingsToEEPROM() {
    // Guardamos Target Temp
    // Nota: pidSetpoint es double, pero targetTemp en main es int. 
    // Guardaremos como int para ahorrar espacio o double si prefieres. Usaremos int para target.
    int targetToSave = (int)pidSetpoint;
    EEPROM.put(EEPROM_ADDR_TARGET_TEMP, targetToSave);

    // Guardamos PID
    EEPROM.put(EEPROM_ADDR_KP, kp);
    EEPROM.put(EEPROM_ADDR_KI, ki);
    EEPROM.put(EEPROM_ADDR_KD, kd);
}

void TemperatureModule::loadSettingsFromEEPROM() {
    // 1. Cargar Temperatura Objetivo
    int storedTarget;
    EEPROM.get(EEPROM_ADDR_TARGET_TEMP, storedTarget);
    
    // Validación: Si es la primera vez (EEPROM virgen = -1 o 65535), ponemos 0
    if (storedTarget < 0 || storedTarget > 300) {
        pidSetpoint = 0; 
    } else {
        pidSetpoint = storedTarget;
    }

    // 2. Cargar PID
    double tempKp, tempKi, tempKd;
    EEPROM.get(EEPROM_ADDR_KP, tempKp);
    EEPROM.get(EEPROM_ADDR_KI, tempKi);
    EEPROM.get(EEPROM_ADDR_KD, tempKd);

    // Validación básica de NaN (Not a Number)
    if (isnan(tempKp) || tempKp < 0) {
        // Si la EEPROM está vacía, cargamos los defaults de Config.h
        kp = TEMP_KP; 
        ki = TEMP_KI; 
        kd = TEMP_KD;
    } else {
        kp = tempKp;
        ki = tempKi;
        kd = tempKd;
    }
    
    // Aplicamos los valores cargados al objeto PID inmediatamente
    myPID.SetTunings(kp, ki, kd);
}