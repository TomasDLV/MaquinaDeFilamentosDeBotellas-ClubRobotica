// FileName: main.cpp

#include <Arduino.h>
#include "Config.h"
#include "UIModule.h"
#include "TemperatureModule.h"
#include "ExtrusionModule.h"
#include <TimerOne.h> // --- LIBRERÍA DE INTERRUPCIÓN ---
// === Creación de los Módulos (Objetos Globales) ===
UIModule ui;
TemperatureModule tempController;
ExtrusionModule extruder;

// === Variables de Estado Global ===
// Estas son las variables que el menú modificará directamente.
int targetTemp = 0;
float motorSpeed = 2.5; // 2.5mm/s
bool hotendEnabled = false; // ¿Está el calentador encendido?
bool motorEnabled = false;
double currentTemp = 0.0;
bool filamentStatus = true; // Placeholder para el futuro sensor

// --- Variables para el PID (editables desde el menú) ---
// Se inicializan con los valores de Config.h, pero pueden ser modificadas en tiempo real.
float kp = TEMP_KP;
float ki = TEMP_KI;
float kd = TEMP_KD;

// Variables volátiles para la ISR del Temporizador ---
// Estas variables son el "puente" entre la ISR y el UIModule.
volatile int g_encoderDelta = 0;
volatile bool g_buttonClicked = false;
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
  // 1. Asegurarnos que los módulos tengan los valores más recientes del menú
  // (Aunque el loop lo hace constantemente, es bueno asegurar antes de guardar)
  extruder.setSpeed(motorSpeed);
  tempController.setTargetTemp(targetTemp);
  tempController.setTunings(kp, ki, kd);

  // 2. Ordenar a los módulos que escriban en la memoria
  extruder.saveSpeedToEEPROM();
  tempController.saveSettingsToEEPROM();

  // Opcional: Feedback visual (puedes hacer un pequeño parpadeo o mensaje en pantalla si quieres)
  Serial.println("Configuracion guardada en EEPROM.");
}

// Le dice al módulo de UI que vuelva a la pantalla de información
void do_showInfoScreen() {
  ui.showInfoScreen();
}
// ========================================================================
// === ISR DEL TEMPORIZADOR (MÉTODO MARLIN) ===============================
// ========================================================================
// Esta función se llamará automáticamente 1000 veces por segundo
void poll_inputs_isr() {
    // --- Lógica del Encoder ---
    static int8_t lastEncoderState = 0;
    static int encoderAccumulator = 0;
    static const int8_t lookup_table[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    uint8_t a = digitalRead(ENC_A_PIN);
    uint8_t b = digitalRead(ENC_B_PIN);
    uint8_t currentState = (a << 1) | b;
    uint8_t index = (lastEncoderState << 2) | currentState;
    lastEncoderState = currentState;
    encoderAccumulator += lookup_table[index];

    if (encoderAccumulator >= 4) {
        g_encoderDelta++; // Incrementa el contador global
        encoderAccumulator = 0;
    } else if (encoderAccumulator <= -4) {
        g_encoderDelta--; // Decrementa el contador global
        encoderAccumulator = 0;
    }

    // --- Lógica del Botón (Debounce por conteo de ticks) ---
    static int buttonState = HIGH;
    static int lastReadingState = HIGH;
    static uint8_t debounceCounter = 0;
    const uint8_t debounceTicks = 50; // 50 llamadas * 1ms/call = 50ms

    int currentReading = digitalRead(ENC_BTN_PIN);
    
    if (currentReading != lastReadingState) {
        debounceCounter = 0;
    } else {
        if (debounceCounter < debounceTicks) {
            debounceCounter++;
        } else {
            if (currentReading != buttonState) {
                buttonState = currentReading;
                if (buttonState == LOW) {
                    g_buttonClicked = true; // Registra el click
                }
            }
        }
    }
    lastReadingState = currentReading;
}
// === Arduino Setup ===
// Se ejecuta una sola vez al encender la máquina.
void setup() {
  Serial.begin(115200);
  //Wire.begin(); // Importante: Inicia el bus I2C para la pantalla

  // 1. Iniciar Módulos
  tempController.init();
  extruder.init();
  ui.init(); 

  // 2. CARGAR DATOS DE EEPROM A LOS MÓDULOS
  tempController.loadSettingsFromEEPROM();
  extruder.loadSpeedFromEEPROM();

  // 3. SINCRONIZAR VARIABLES GLOBALES (MENU) CON LO CARGADO
  // El módulo ya tiene los datos guardados, ahora actualizamos las variables
  // que usa el menú para mostrarlos.
  motorSpeed = extruder.getSpeed();
  targetTemp = tempController.getTargetTemp();
  
  // También recuperamos el PID guardado para que el menú lo muestre
  kp = tempController.getKp();
  ki = tempController.getKi();
  kd = tempController.getKd();

  Timer1.initialize(1000); // 1000us = 1ms
  Timer1.attachInterrupt(poll_inputs_isr); // Asocia la ISR

  Serial.println("Sistema listo.");
}

// === Arduino Loop ===
// Este bucle se ejecuta continuamente, lo más rápido posible.
void loop() {
  // Primero, actualizamos tunings del PID si han cambiado desde el menú
  tempController.setTunings(kp, ki, kd);

  // Luego, actualizamos el módulo de temperatura (esto mide y guarda currentTemp)
  tempController.update();

  // Ahora sí, leemos la última temperatura estable ya calculada por el módulo. Es importante que update esté antes para que lea la temperatura actualizada.
  currentTemp = tempController.getCurrentTemp(); // Actualiza la variable global para la UI

  // Llama al método update() de cada módulo. Es un bucle no bloqueante.
  extruder.setSpeed(motorSpeed);
  extruder.update();
  ui.update();

}