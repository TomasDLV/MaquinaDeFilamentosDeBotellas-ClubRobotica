// FileName: Config.h

#ifndef CONFIG_H
#define CONFIG_H

// --------- Módulo de UI (Pantalla y Encoder) ---------
// Pantalla ST7920 en modo SPI
#define LCD_CLK_PIN 23
#define LCD_CS_PIN  16
#define LCD_MOSI_PIN 17

// Encoder Rotativo (con interrupciones)
#define ENC_A_PIN   33
#define ENC_B_PIN   31
#define ENC_BTN_PIN 35

// --------- Módulo de Temperatura ---------
// --- Parámetros del Termistor NTC ---
#define VCC 5.065        // Voltaje de referencia real de tu placa (V)
#define R_REF 4661.0     // Resistencia fija en el divisor de voltaje (Ohms)
#define NTC_R0 100000.0  // Resistencia del NTC a 25°C (100kΩ)
#define NTC_T0 298.15    // 25°C en Kelvin
#define NTC_BETA 4200.0  // Coeficiente Beta del NTC
#define TEMP_OFFSET 3.0  // Offset de calibración en °C

// --- Ajustes de Lectura ---
#define THERMISTOR_SAMPLES 10  // Número de muestras para promediar

// --- Parámetros del PID ---
// Usamos los valores que ya has probado
#define TEMP_KP 20.0
#define TEMP_KI 5.0
#define TEMP_KD 100.0

// El tiempo de muestreo del PID. Tu código usaba un delay(800), así que usamos un valor similar.
#define PID_SAMPLE_TIME 800

// --- Pines Físicos ---
#define TEMP_HEATER_PIN 10
#define TEMP_THERMISTOR_PIN A15

// --------- Módulo de Extrusión ---------
// Pines para el driver del motor (ej: A4988, DRV8825)
#define MOTOR_ENABLE_PIN 24 // Opcional, pero recomendado
#define MOTOR_STEP_PIN   26
#define MOTOR_DIR_PIN    28
#define MOTOR_MAX_SPEED 1000

#endif