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
int targetTemp = 0; // Inicia en 0 (apagado)
int motorRPM = 60;  // <--- CAMBIO: Renombrado de 'motorSpeed' a 'motorRPM'
bool hotendEnabled = false;
bool motorEnabled = false;
bool filamentRunout = false; // <-- Estado de parada de emergencia

// === Funciones de Acción para el Menú ===

void do_toggleHotend()
{
  hotendEnabled = !hotendEnabled;
  if (hotendEnabled)
  {
    tempController.setTargetTemp(targetTemp);
  }
  else
  {
    tempController.setTargetTemp(0);
  }
}

void do_toggleMotor()
{
  motorEnabled = !motorEnabled;
  if (motorEnabled)
  {
    extruder.setSpeed(motorRPM); // <-- CAMBIO: Se usa la nueva función setRPM
    extruder.start();
  }
  else
  {
    extruder.stop();
  }
}

void do_dummy_function()
{
  // Función vacía para el botón "Volver"
}

// <--- DEFINICIÓN AÑADIDA: Esta es la función que faltaba
void do_saveSettings()
{
  extruder.saveSpeedToEEPROM();
  // Aquí puedes añadir el guardado de la temperatura en el futuro
}

// === Arduino Setup ===
void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando Maquina de Filamento...");

  pinMode(FILAMENT_SENSOR_PIN, INPUT_PULLUP); // Inicializar el sensor de filamento
  tempController.init();
  extruder.init();
  ui.init();

  Serial.println("Sistema listo.");
}

// === Arduino Loop ===
void loop()
{
  // --- Comprobación de sensor de filamento ---
  if (digitalRead(FILAMENT_SENSOR_PIN) == HIGH)
  {
    filamentRunout = true;
  }
  // Si no hay emergencia, operamos normalmente
  if (!filamentRunout)
  {
    extruder.setSpeed(motorRPM);
    tempController.update();
    extruder.update();
    ui.update();
  }
  else
  {
    // 1. Apagar el motor
    if (motorEnabled)
    {
      extruder.stop();
      motorEnabled = false;
    }
    // 2. Apagar el calentador
    if (hotendEnabled)
    {
      tempController.setTargetTemp(0);
      targetTemp = 0;
      hotendEnabled = false;
    }
    // 3. Mostrar error en la pantalla.
    
    // 4. Actualizar módulos por última vez
    tempController.update();
    ui.update();
  }
}