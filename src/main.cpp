// FileName: main.cpp

#include <Arduino.h>
#include "Config.h"
#include "UIModule.h"
#include "TemperatureModule.h"
#include "ExtrusionModule.h"

// === Creación de los Módulos ===
UIModule ui;
TemperatureModule tempController;
ExtrusionModule extruder;

// === Variables de Estado Global ===
int targetTemp = 0;   // Inicia en 0 (apagado)
int motorRPM = 60; // <--- CAMBIO: Renombrado de 'motorSpeed' a 'motorRPM'
bool hotendEnabled = false;
bool motorEnabled = false;

// === Funciones de Acción para el Menú ===

void do_toggleHotend() {
  hotendEnabled = !hotendEnabled;
  if (hotendEnabled) {
    tempController.setTargetTemp(targetTemp);
  } else {
    tempController.setTargetTemp(0);
  }
}

void do_toggleMotor() {
  motorEnabled = !motorEnabled;
  if (motorEnabled) {
    extruder.setSpeed(motorRPM); // <-- CAMBIO: Se usa la nueva función setRPM
    extruder.start();
  } else {
    extruder.stop();
  }
}

void do_dummy_function() {
  // Función vacía para el botón "Volver"
}

// <--- DEFINICIÓN AÑADIDA: Esta es la función que faltaba
void do_saveSettings() {
  extruder.saveSpeedToEEPROM();
  // Aquí puedes añadir el guardado de la temperatura en el futuro
}

// === Arduino Setup ===
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Maquina de Filamento...");

  tempController.init();
  extruder.init();
  ui.init(); 

  Serial.println("Sistema listo.");
}

// === Arduino Loop ===
void loop() {
  // Sincroniza el valor del menú con el motor
  extruder.setSpeed(motorRPM); 

  tempController.update();
  extruder.update();
  ui.update();
}