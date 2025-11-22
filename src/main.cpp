// FileName: main.cpp

#include <Arduino.h>
#include "Config.h"
#include "UIModule.h"
#include "TemperatureModule.h"
#include "ExtrusionModule.h"
#include <TimerOne.h> 

// === Objetos ===
UIModule ui;
TemperatureModule tempController;
ExtrusionModule extruder;

// === Variables Globales ===
int targetTemp = 0;
float motorSpeed = 2.5; 
bool hotendEnabled = false; 
bool motorEnabled = false;
double currentTemp = 0.0;
bool filamentStatus = true; 

// PID
float kp = TEMP_KP;
float ki = TEMP_KI;
float kd = TEMP_KD;

// Interrupción
volatile int g_encoderDelta = 0;
volatile bool g_buttonClicked = false;

// === Funciones Menu ===
void do_toggleHotend() {
  hotendEnabled = !hotendEnabled;
  if (hotendEnabled) tempController.setTargetTemp(targetTemp);
  else tempController.setTargetTemp(0);
}

void do_toggleMotor() {
  motorEnabled = !motorEnabled;
  if (motorEnabled) extruder.start();
  else extruder.stop();
}

void do_dummy_function() {}

// En main.cpp

void do_saveSettings() {
  // 1. Actualizar Motor y PID (Esto es seguro)
  extruder.setSpeed(motorSpeed);
  tempController.setTunings(kp, ki, kd);

  // 2. --- CORRECCIÓN CRÍTICA ---
  // Solo actualizamos el objetivo del PID si el Hotend está habilitado.
  // Si está APAGADO, nos aseguramos de enviarle 0 al controlador para que siga apagado.
  if (hotendEnabled) {
    tempController.setTargetTemp(targetTemp);
  } else {
    tempController.setTargetTemp(0);
  }

  // 3. Guardar en EEPROM
  extruder.saveSpeedToEEPROM();
  
  // --- CAMBIO AQUÍ ---
  // Le pasamos 'targetTemp' (ej. 200) explícitamente.
  // Así se guarda el 200 en la memoria, aunque el controlador esté en 0.
  tempController.saveSettingsToEEPROM(targetTemp);

  // 4. Feedback
  Serial.println("¡Guardado con exito!");
  tone(LCD_BEEPER_PIN, 2000, 200); 
}

void do_showInfoScreen() {
  ui.showInfoScreen();
}

// ========================================================================
// === ISR DEL TEMPORIZADOR ===
// ========================================================================
void system_isr() {
    // 1. Motor (Prioridad)
    extruder.update(); 

    // 2. Encoder
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
        g_encoderDelta++;
        encoderAccumulator = 0;
    } else if (encoderAccumulator <= -4) {
        g_encoderDelta--;
        encoderAccumulator = 0;
    }

    // 3. Botón
    static int buttonState = HIGH;
    static int lastReadingState = HIGH;
    static uint8_t debounceCounter = 0;
    
    // --- CAMBIO AQUÍ: Ajuste de Debounce ---
    // Como el Timer ahora va a 1ms (1000us), 50 ticks = 50ms.
    // Antes con 200us usábamos 250 ticks. Ahora bajamos a 50.
    const uint8_t debounceTicks = 50; 

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
                    g_buttonClicked = true; 
                }
            }
        }
    }
    lastReadingState = currentReading;
}

// === Setup ===
void setup() {
  Serial.begin(115200);

  // Inicialización de módulos
  tempController.init();
  extruder.init();
  ui.init(); // Esto inicializa la pantalla (u8g2.begin)

  // Carga de EEPROM
  tempController.loadSettingsFromEEPROM();
  extruder.loadSpeedFromEEPROM();

  // Sincronización de variables
  motorSpeed = extruder.getSpeed();
  targetTemp = tempController.getTargetTemp();
  hotendEnabled = false; 
  tempController.setTargetTemp(0); // Inicia apagado por seguridad
  kp = tempController.getKp();
  ki = tempController.getKi();
  kd = tempController.getKd();

  // --- CAMBIO AQUÍ: Timer a 1000us (1ms) ---
  // Esto es suficientemente rápido para el motor, pero deja vivir a la pantalla.
  Timer1.initialize(1000); 
  Timer1.attachInterrupt(system_isr); 

  Serial.println("Sistema listo.");
}

// === Loop ===
void loop() {
  // 1. PID y Temp
  tempController.update();
  currentTemp = tempController.getCurrentTemp(); 

  // 2. Motor (Solo setea velocidad, el movimiento lo hace la ISR)
  noInterrupts();
  extruder.setSpeed(motorSpeed);
  interrupts();

  // 3. Pantalla
  ui.update(); 
}