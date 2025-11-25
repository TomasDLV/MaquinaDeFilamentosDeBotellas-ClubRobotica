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

  // Inicializando Seguridad
  heatStartTime = millis();            // Inicia el contador de tiempo de calentamiento
  lastTemp = 0;                        // Inicializa la temperatura anterior
  lastTempChangeTime = millis();      // Marca el tiempo de último cambio de temperatura
  errorState = TEMP_OK;               // Estado inicial sin errores
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

  // Aplicando seguridad de Temperatura
  // Aplicar salida PWM solo si no hay error
  if (errorState == TEMP_OK) {
       // 4. APLICAR POTENCIA
      // Lógica Normal: Mayor valor de salida PID (0-255) = Más voltaje al pin
      analogWrite(TEMP_HEATER_PIN, (int)pidOutput);
  }

  // Revisar seguridad en cada ciclo
  safetyCheck();
  
 
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

// Funciones de Seguridad Temperatura
// Función principal de seguridad
void TemperatureModule::safetyCheck() {
  double currentTemp = pidInput;       // Temperatura actual leída del sensor
  double target = pidSetpoint;         // Temperatura objetivo configurada

  // --- 1. FALLO DE SENSOR ---
  if (currentTemp < -5 || currentTemp > 260) {
    errorState = ERROR_SENSOR_FAIL; // Lectura fuera de rango razonable

  }

  // --- 2. SOBRECALENTAMIENTO ---
  if (currentTemp > target + 20) { 
    errorState = ERROR_OVERHEAT; // Temperatura excede el objetivo por más de 20°C

  }

  // --- 3. TIMEOUT DE CALENTAMIENTO ---
  if (target > 0) {
    if (millis() - heatStartTime > 120000) { // Si pasaron mas de 120 segundos
      if (currentTemp < target - 30) { // Y aun esta muy lejos del objetivo
        errorState = ERROR_HEATING_TIMEOUT;
      }
    }
  }

  // --- 4. RUNAWAY (no sube la temperatura en mucho tiempo) ---
  if (abs(currentTemp - lastTemp) > 1.0) {
    lastTemp = currentTemp; // Actualiza si hubo cambio significativo
    lastTempChangeTime = millis(); // Marca el tiempo del cambio

  }

  if (millis() - lastTempChangeTime > 30000) { // 30 s sin cambios
    if (currentTemp < target - 10) {
      errorState = ERROR_RUNAWAY;
    }
  }

  // Si hay error  APAGAR TODO
  if (errorState != TEMP_OK) {
    analogWrite(TEMP_HEATER_PIN, 0);
  }
}

//Funciones para obtener y resetear error
TempError TemperatureModule::getError() {
    return errorState; // Devuelve el estado actual de error

}

void TemperatureModule::resetError() {
    errorState = TEMP_OK;                  // Limpia estado de error
    heatStartTime = millis();              // Reinicia tiempo de calentamiento
    lastTemp = pidInput;                   // Actualiza la temperatura base
    lastTempChangeTime = millis();         // Reinicia tiempo de cambio
}