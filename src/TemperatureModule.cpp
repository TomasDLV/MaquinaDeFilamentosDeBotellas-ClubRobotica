// FileName: TemperatureModule.cpp

#include "TemperatureModule.h"
#include "Config.h"
#include <Arduino.h>

// El constructor inicializa el objeto PID usando una lista de inicialización
TemperatureModule::TemperatureModule() : 
  myPID(&pidInput, &pidOutput, &pidSetpoint, TEMP_KP, TEMP_KI, TEMP_KD, DIRECT)
{
  // Guardamos las constantes por si necesitamos reajustarlas en el futuro
  kp = TEMP_KP;
  ki = TEMP_KI;
  kd = TEMP_KD;
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
  // Si el objetivo es 0 o menos, mantenemos el calentador apagado
  if (pidSetpoint <= 0) {
    analogWrite(TEMP_HEATER_PIN, 0);
    return;
  }
  
  pidInput = getTemp();      // 1. Leemos la temperatura actual
  myPID.Compute();           // 2. La librería decide si es hora de calcular y lo hace
  analogWrite(TEMP_HEATER_PIN, (int)pidOutput); // 3. Aplicamos la salida PWM
}

// Función para ajustar las constantes del PID "en caliente"
void TemperatureModule::setTunings(double Kp, double Ki, double Kd) {
    myPID.SetTunings(Kp, Ki, Kd);
}