// FileName: main.cpp

#include <Arduino.h>
#include "Config.h"
#include "UIModule.h"
#include "TemperatureModule.h"
#include "ExtrusionModule.h"

// === Creación de los Módulos (Objetos Globales) ===
UIModule ui;
TemperatureModule tempController;
ExtrusionModule extruder;

// === Variables de Estado Global ===
// Estas son las variables que el menú modificará directamente.
int targetTemp = 0;
int motorRPM = 60;
bool hotendEnabled = false;
bool motorEnabled = false;
double currentTemp = 0.0;
bool filamentStatus = true; // Placeholder para el futuro sensor

// --- Variables para el PID (editables desde el menú) ---
// Se inicializan con los valores de Config.h, pero pueden ser modificadas en tiempo real.
float kp = TEMP_KP;
float ki = TEMP_KI;
float kd = TEMP_KD;

// === Funciones de Acción para el Menú ===
// Estas son las "acciones" que se conectan a los botones del menú.

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
    extruder.start();
  } else {
    extruder.stop();
  }
}

// Función vacía usada por el botón "Volver"
void do_dummy_function() {}

// Guarda la configuración actual en la memoria no volátil
void do_saveSettings() {
  extruder.saveSpeedToEEPROM();
  // En el futuro, aquí se podría añadir:
  // tempController.saveTuningsToEEPROM(kp, ki, kd);
}

// Le dice al módulo de UI que vuelva a la pantalla de información
void do_showInfoScreen() {
  ui.showInfoScreen();
}

// === Arduino Setup ===
// Se ejecuta una sola vez al encender la máquina.
void setup() {
  Serial.begin(115200);
  Wire.begin(); // Importante: Inicia el bus I2C para la pantalla

  // Llama al método de inicialización de cada módulo.
  tempController.init();
  extruder.init();
  ui.init(); 
  Serial.println("Sistema listo.");
}

// === Arduino Loop ===
// Este bucle se ejecuta continuamente, lo más rápido posible.
void loop() {
  // --- Sincronización de Datos ---
  // Se asegura de que los módulos siempre tengan los valores más recientes.
  currentTemp = tempController.getTemp();
  extruder.setSpeed(motorRPM);
  tempController.setTunings(kp, ki, kd); // Actualiza las constantes del PID en tiempo real

  // --- Actualización de Módulos ---
  // Llama al método update() de cada módulo. Es un bucle no bloqueante.
  tempController.update();
  extruder.update();
  ui.update();
}