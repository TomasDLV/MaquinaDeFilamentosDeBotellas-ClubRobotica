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
  digitalWrite(MOTOR_ENABLE_PIN, HIGH);

  stepper.setMaxSpeed(MOTOR_MAX_SPEED);
  
  // 1. Cargamos la velocidad guardada al iniciar
  this->loadSpeedFromEEPROM();
  // 2. Le decimos al motor que use esa velocidad
  stepper.setSpeed(currentSpeed);
  
  this->stop(); 
}

void ExtrusionModule::loadSpeedFromEEPROM() {
  float storedSpeed;
  EEPROM.get(eepromAddr, storedSpeed);

  // Verificamos que el valor leído sea lógico. Si no, usamos un valor por defecto.
  if (isnan(storedSpeed) || storedSpeed < 0 || storedSpeed > MOTOR_MAX_SPEED) {
    currentSpeed = 200.0; // Velocidad por defecto si la EEPROM está vacía o corrupta
  } else {
    currentSpeed = storedSpeed;
  }
}

void ExtrusionModule::saveSpeedToEEPROM() {
  // Guardamos el valor actual de la velocidad en la EEPROM
  EEPROM.put(eepromAddr, currentSpeed);
}

void ExtrusionModule::setSpeed(float newSpeed) {
  // Actualiza la velocidad en la variable y en el motor
  currentSpeed = constrain(newSpeed, 0, MOTOR_MAX_SPEED);
  stepper.setSpeed(currentSpeed);
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