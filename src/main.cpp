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
  // 1. Actualizar los módulos con los valores del menú
  extruder.setSpeed(motorSpeed);
  tempController.setTargetTemp(targetTemp);
  tempController.setTunings(kp, ki, kd);

  // 2. Guardar en EEPROM
  extruder.saveSpeedToEEPROM();
  tempController.saveSettingsToEEPROM();

  // 3. --- FEEDBACK VISUAL Y SONORO ---
  Serial.println("¡Guardado con exito!");
  
  // Hacemos un BEEP de confirmación (2000Hz por 200ms)
  // Nota: tone() funciona en pines PWM o digitales en la mayoría de Arduinos
  tone(LCD_BEEPER_PIN, 2000, 200); 
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

  // 1. Iniciamos los módulos (esto carga valores por defecto o internos)
  tempController.init();
  extruder.init();
  ui.init(); 

  // 2. FORZAMOS la carga desde la EEPROM
  // (Asegúrate de haber implementado estas funciones como vimos antes)
  tempController.loadSettingsFromEEPROM();
  extruder.loadSpeedFromEEPROM();

  // 3. --- CORRECCIÓN CRÍTICA ---
  // Actualizamos las variables globales del menú con lo que se leyó de la memoria.
  // Si no haces esto, el valor fijo de 'motorSpeed' sobrescribirá la memoria.
  motorSpeed = extruder.getSpeed();
  targetTemp = tempController.getTargetTemp();
  
  // También el PID
  kp = tempController.getKp();
  ki = tempController.getKi();
  kd = tempController.getKd();

  // 4. Configuramos Timer y resto
  Timer1.initialize(1000); 
  Timer1.attachInterrupt(poll_inputs_isr); 

  Serial.println("Sistema iniciado. Datos cargados de EEPROM.");
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