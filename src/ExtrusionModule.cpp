// FileName: ExtrusionModule.cpp

#include "ExtrusionModule.h"
#include "Config.h"
#include <EEPROM.h> // Necesitamos incluir la librería EEPROM

ExtrusionModule::ExtrusionModule() :
  stepper(AccelStepper::DRIVER, MOTOR_STEP_PIN, MOTOR_DIR_PIN)
{
  currentSpeed = 0.0; // Inicia detenido
}

void ExtrusionModule::init() {
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, HIGH); //Inicia apagado

  stepper.setMaxSpeed(MOTOR_MAX_SPEED);
  
  // 1. Cargamos la velocidad guardada al iniciar
  this->loadSpeedFromEEPROM();
  // 2. Le decimos al motor que use esa velocidad
  stepper.setSpeed(currentSpeed);
  
  this->stop(); 
}

void ExtrusionModule::loadSpeedFromEEPROM() {
  float storedVal; // Leemos como float
  EEPROM.get(eepromAddr, storedVal);

  // Validación: Si es NaN (basura) o negativo o muy alto, ponemos un valor seguro
  if (isnan(storedVal) || storedVal < 0 || storedVal > MAX_EXTRUSION_SPEED_MMS) {
    currentSpeed = 2.0; // Velocidad segura por defecto (2 mm/s)
    // Nota: Para PET, el inicio suele ser lento (entre 1.5 y 3 mm/s)
  } else {
    currentSpeed = storedVal;
  }
}

void ExtrusionModule::saveSpeedToEEPROM() {
  // Guardamos el valor actual de la velocidad en la EEPROM
  EEPROM.put(eepromAddr, currentSpeed);
}

void ExtrusionModule::setSpeed(float newSpeed) {
  // Actualiza la velocidad en la variable y en el motor
  // 1. Limitamos el rango (seguridad)
  currentSpeed = constrain(newSpeed, 0.0, MAX_EXTRUSION_SPEED_MMS);
  
  // 2. Convertimos mm/s a pasos/segundo
  // Fórmula: (mm/s) * (pasos/mm) = pasos/s
  float stepsPerSec = currentSpeed * E_STEPS_PER_MM;
  
  // 3. Aplicamos al motor
  stepper.setSpeed(stepsPerSec);
}

float ExtrusionModule::getSpeed() { 
  return currentSpeed;
}

void ExtrusionModule::start() {
    digitalWrite(MOTOR_ENABLE_PIN, LOW);
}

void ExtrusionModule::stop() {
    stepper.stop();
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
}

void ExtrusionModule::update() {
  stepper.runSpeed();
}