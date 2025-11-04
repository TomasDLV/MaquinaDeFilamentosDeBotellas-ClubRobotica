// FileName: UIModule.cpp

#include "UIModule.h"
#include "Config.h" // Necesario para los pines LCD_CLK_PIN, etc.
#include "MenuItem.h"
#include "ActionMenuItem.h"
#include "SubMenu.h"
#include "EditableValueMenuItem.h"
#include "EditableFloatValueMenuItem.h"
#include <Arduino.h>

// --- Declaraciones 'extern' ---
// Le decimos al compilador que estas variables y funciones globales existen en main.cpp
extern int targetTemp;
extern int motorRPM;
extern bool motorEnabled;
extern double currentTemp;
extern bool filamentStatus;
extern float kp, ki, kd;

extern void do_toggleMotor();
extern void do_toggleHotend();
extern void do_dummy_function();
extern void do_saveSettings();
extern void do_showInfoScreen();

// --- Implementación de los Métodos de la Clase ---

// Constructor: Inicializa el objeto u8g2 con los pines SPI y las variables de polling
UIModule::UIModule() :
    u8g2(U8G2_R0, LCD_CLK_PIN, LCD_MOSI_PIN, LCD_CS_PIN)
{
    currentMenu = nullptr;
    currentState = STATE_INFO_SCREEN;
    lastEncoderState = 0;
    // Inicializamos las nuevas variables del botón
    lastReadingState = HIGH; // Asumimos INPUT_PULLUP, no presionado al inicio
    buttonState = HIGH;      // El estado estable inicial también es no presionado
    lastDebounceTime = 0;
}

// Inicialización: Configura pines, lee estado inicial y arranca la pantalla
void UIModule::init() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);

  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  lastEncoderState = (a << 1) | b;
  // Leemos el estado inicial real y lo asignamos a AMBAS variables de estado
  lastReadingState = digitalRead(ENC_BTN_PIN);
  buttonState = lastReadingState; 

  u8g2.begin();
  buildMenu();
}

// Construye la estructura del menú usando objetos estáticos y arreglos
void UIModule::buildMenu() {
    static ActionMenuItem itemPIDVolver("<- Volver", do_dummy_function);
    static EditableFloatValueMenuItem itemPID_Kp("Kp: ", &kp, "", 0, 100, 0.5);
    static EditableFloatValueMenuItem itemPID_Ki("Ki: ", &ki, "", 0, 100, 0.1);
    static EditableFloatValueMenuItem itemPID_Kd("Kd: ", &kd, "", 0, 200, 1.0);
    static MenuItem* pidItems[] = {&itemPIDVolver, &itemPID_Kp, &itemPID_Ki, &itemPID_Kd};
    static SubMenu menuConfigPID("Configuracion PID", nullptr, pidItems, 4);

    static ActionMenuItem itemPrincipalInfo("<- Ver Info", do_showInfoScreen);
    static EditableValueMenuItem itemPrincipalTemp("Temperatura: ", &targetTemp, " C", 0, 260);
    static EditableValueMenuItem itemPrincipalVel("Velocidad: ", &motorRPM, " RPM", 0, 120);
    static ActionMenuItem itemPrincipalHotend("Encender Hotend", do_toggleHotend);
    static ActionMenuItem itemPrincipalMotor("Encender Motor", do_toggleMotor);
    static ActionMenuItem itemPrincipalGuardar("Guardar Config", do_saveSettings);
    static MenuItem* mainMenuItems[] = { &itemPrincipalInfo, &itemPrincipalTemp, &itemPrincipalVel, &menuConfigPID, &itemPrincipalHotend, &itemPrincipalMotor, &itemPrincipalGuardar };
    static SubMenu menuPrincipal("Menu Principal", nullptr, mainMenuItems, 7);

    currentMenu = &menuPrincipal;
    menuConfigPID.parent = &menuPrincipal;
}

// --- Lectura del Encoder por Polling (Robusta) ---
int UIModule::readEncoder() {
  static const int8_t lookup_table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  uint8_t currentState = (a << 1) | b; // Estado actual como 0bAB (0, 1, 2, o 3)
  uint8_t index = (lastEncoderState << 2) | currentState; // Índice combinado (0-15)
  lastEncoderState = currentState; // Actualiza para la próxima vez

  // Acumulador para contar micro-pasos hasta un "click" completo
  static int encoderAccumulator = 0;
  encoderAccumulator += lookup_table[index];

  int delta = 0;
  // Si acumulamos 2 micro-pasos, es un click horario
  if (encoderAccumulator >= 2) {
      delta = 1;
      encoderAccumulator = 0;
  // Si acumulamos -2 micro-pasos, es un click antihorario
  } else if (encoderAccumulator <= -2) {
      delta = -1;
      encoderAccumulator = 0;
  }
  return delta;
}

// --- Lectura del Botón por Polling con Debounce ---
// --- FUNCIÓN readButton() CON LÓGICA DEBOUNCE ESTÁNDAR ---
bool UIModule::readButton() {
  bool triggered = false; // Indica si se detectó una pulsación VÁLIDA en este ciclo

  // 1. Lee el estado actual del pin
  int currentReading = digitalRead(ENC_BTN_PIN);

  // 2. Comprueba si la lectura actual es DIFERENTE a la lectura del ciclo ANTERIOR
  //    Si es diferente, significa que el estado del pin está cambiando (puede ser ruido o una pulsación real)
  if (currentReading != lastReadingState) {
    // Resetea el temporizador de debounce CADA VEZ que detecta un cambio
    lastDebounceTime = millis();
    // Descomenta para depurar:
    // Serial.print("Button changing... Reading: "); Serial.println(currentReading);
  }

  // 3. Comprueba si ha pasado suficiente tiempo desde el ÚLTIMO cambio detectado
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Si el tiempo ha pasado, significa que la lectura actual se ha mantenido ESTABLE
    // Ahora, comprobamos si este estado estable es DIFERENTE al último estado ESTABLE que registramos
    if (currentReading != buttonState) {
      // Si es diferente, actualizamos el estado estable
      buttonState = currentReading;
      // Descomenta para depurar:
      // Serial.print("Button stable state changed to: "); Serial.println(buttonState);

      // Si el NUEVO estado estable es PRESIONADO (LOW)
      if (buttonState == LOW) {
        triggered = true; // ¡Registramos la pulsación válida!
        // Descomenta para depurar:
        // Serial.println("---> Button Press Triggered <---");
      }
    }
  }

  // 4. Guarda la lectura actual para compararla en el próximo ciclo
  lastReadingState = currentReading;

  // 5. Devuelve si se detectó una pulsación válida en este ciclo
  return triggered;
}

// Bucle principal de la UI: Lee entradas, actualiza estado y dibuja
void UIModule::update() {
  // Lee las entradas usando las funciones de polling
  int d = readEncoder();
  bool clicked = readButton();
  Serial.print("Boton Presionado: ");
  Serial.println(clicked);
  // Convierte las lecturas en eventos de menú
  MenuInput input = INPUT_NONE;
  if (d > 0) input = INPUT_NEXT;
  if (d < 0) input = INPUT_PREV;
  if (clicked) input = INPUT_SELECT;

  // Actualiza la máquina de estados de la UI
  switch (currentState) {
    case STATE_INFO_SCREEN:
      // Si estamos en la pantalla de info, un click nos lleva al menú
      if (input == INPUT_SELECT) {
        currentState = STATE_MENU;
      }
      break;
    case STATE_MENU:
      // Si estamos en el menú, procesamos la navegación
      if (input != INPUT_NONE && currentMenu != nullptr) {
          MenuItem* nextMenu = currentMenu->handleInput(input);
          // Actualizamos el menú actual si la acción resultó en un cambio
          if (nextMenu != nullptr) {
             currentMenu = nextMenu;
          }
      }
      break;
  }
  
  // Redibuja la pantalla al final de cada ciclo
  draw();
}

// Función principal de dibujo: llama a la función específica según el estado
void UIModule::draw() {
    u8g2.firstPage(); // Inicia el ciclo de dibujado de U8g2
    do {
      switch (currentState) {
        case STATE_INFO_SCREEN:
          drawInfoScreen(); // Dibuja la pantalla de estado
          break;
        case STATE_MENU:
          if (currentMenu != nullptr) {
              currentMenu->draw(u8g2, 0, 0, false); // Dibuja el menú actual
          }
          break;
      }
    } while (u8g2.nextPage()); // Termina el ciclo de dibujado
}

// Dibuja la pantalla de información con los datos actuales
void UIModule::drawInfoScreen() {
    char buffer[32];
    u8g2.setFont(u8g2_font_ncenB10_tr); // Fuente grande para info principal

    // Muestra Temperatura Actual / Objetivo
    u8g2.drawStr(0, 12, "Temperatura:");
    snprintf(buffer, sizeof(buffer), "%.1f C / %d C", currentTemp, targetTemp);
    u8g2.drawStr(0, 26, buffer);

    // Muestra Velocidad y Estado del Motor
    u8g2.drawStr(0, 42, "Motor:");
    const char* motorStateStr = motorEnabled ? "ON" : "OFF";
    snprintf(buffer, sizeof(buffer), "%d RPM (%s)", motorRPM, motorStateStr);
    u8g2.drawStr(0, 56, buffer);

    // Placeholder para Sensor de Filamento
    // const char* filamentStateStr = filamentStatus ? "OK" : "NO";
    // u8g2.drawStr(100, 56, filamentStateStr);
}

// Función pública para cambiar al estado de pantalla de información
void UIModule::showInfoScreen() {
    currentState = STATE_INFO_SCREEN;
}