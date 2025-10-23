// FileName: UIModule.cpp

#include "UIModule.h"
#include "Config.h"
#include "MenuItem.h"
#include "ActionMenuItem.h"
#include "SubMenu.h"
#include "EditableValueMenuItem.h"
#include "EditableFloatValueMenuItem.h" // Incluimos la nueva clase para valores flotantes
#include <Arduino.h>

// --- Declaraciones 'extern' ---
// Le decimos a este archivo que estas variables y funciones existen en 'main.cpp'
extern int targetTemp;
extern int motorRPM;
extern bool motorEnabled;
extern double currentTemp;
extern bool filamentStatus;
extern float kp, ki, kd; // Variables PID

extern void do_toggleMotor();
extern void do_toggleHotend();
extern void do_dummy_function();
extern void do_saveSettings();
extern void do_showInfoScreen();

// --- Inicialización de variables estáticas ---
volatile int UIModule::encoderDelta = 0;
volatile bool UIModule::buttonPressed = false;

// --- ISRs ---
void UIModule::encoder_isr() {
  static int8_t lastState = 0;
  static const int8_t table[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  lastState = (lastState << 2) | (a << 1) | b;
  UIModule::encoderDelta += table[lastState & 0x0F];
}

void UIModule::button_isr() {
  UIModule::buttonPressed = true;
}

// --- Implementación de los Métodos de la Clase ---
UIModule::UIModule() : u8g2(U8G2_R0) {
    currentMenu = nullptr;
    currentState = STATE_INFO_SCREEN;
}

void UIModule::init() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), UIModule::encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), UIModule::encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_BTN_PIN), UIModule::button_isr, FALLING);

  u8g2.begin();
  buildMenu();
}

// ========================================================================
// === buildMenu() con la NUEVA ESTRUCTURA ================================
// ========================================================================
void UIModule::buildMenu() {
    // La palabra 'static' es crucial. Asegura que los objetos del menú
    // se creen una sola vez y no se destruyan al salir de esta función.

    // --- 1. Definir Submenú de Configuración PID (el más interno) ---
    static ActionMenuItem itemPIDVolver("<- Volver", do_dummy_function);
    static EditableFloatValueMenuItem itemPID_Kp("Kp: ", &kp, "", 0, 100, 0.5);
    static EditableFloatValueMenuItem itemPID_Ki("Ki: ", &ki, "", 0, 100, 0.1);
    static EditableFloatValueMenuItem itemPID_Kd("Kd: ", &kd, "", 0, 200, 1.0);
    
    static MenuItem* pidItems[] = {&itemPIDVolver, &itemPID_Kp, &itemPID_Ki, &itemPID_Kd};
    static SubMenu menuConfigPID("Configuracion PID", nullptr, pidItems, 4);

    // --- 2. Definir Menú Principal ---
    static ActionMenuItem itemPrincipalInfo("<- Ver Info", do_showInfoScreen);
    static EditableValueMenuItem itemPrincipalTemp("Temperatura: ", &targetTemp, " C", 0, 260);
    static EditableValueMenuItem itemPrincipalVel("Velocidad: ", &motorRPM, " RPM", 0, 120);
    // 'menuConfigPID' se añade aquí como un item más
    static ActionMenuItem itemPrincipalHotend("Encender Hotend", do_toggleHotend);
    static ActionMenuItem itemPrincipalMotor("Encender Motor", do_toggleMotor);
    static ActionMenuItem itemPrincipalGuardar("Guardar Config", do_saveSettings);

    static MenuItem* mainMenuItems[] = {
        &itemPrincipalInfo,
        &itemPrincipalTemp,
        &itemPrincipalVel,
        &menuConfigPID, // El submenú es un item dentro del menú principal
        &itemPrincipalHotend,
        &itemPrincipalMotor,
        &itemPrincipalGuardar
    };

    static SubMenu menuPrincipal("Menu Principal", nullptr, mainMenuItems, 7);

    // Asignamos el menú actual para empezar
    currentMenu = &menuPrincipal;

    // Asignamos el padre al submenú para que sepa cómo volver
    menuConfigPID.parent = &menuPrincipal;
}
// ========================================================================

void UIModule::update() {
  noInterrupts();
  int d = encoderDelta / 4;
  encoderDelta = 0;
  bool clicked = buttonPressed;
  buttonPressed = false;
  interrupts();

  MenuInput input = INPUT_NONE;
  if (d > 0) input = INPUT_NEXT;
  if (d < 0) input = INPUT_PREV;
  if (clicked) input = INPUT_SELECT;

  switch (currentState) {
    case STATE_INFO_SCREEN:
      if (input == INPUT_SELECT) {
        currentState = STATE_MENU;
      }
      break;
    case STATE_MENU:
      if (input != INPUT_NONE && currentMenu != nullptr) {
          MenuItem* nextMenu = currentMenu->handleInput(input);
          if (nextMenu != nullptr) {
             currentMenu = nextMenu;
          }
      }
      break;
  }
  
  draw();
}

void UIModule::draw() {
    u8g2.firstPage();
    do {
      switch (currentState) {
        case STATE_INFO_SCREEN:
          drawInfoScreen();
          break;
        case STATE_MENU:
          if (currentMenu != nullptr) {
              currentMenu->draw(u8g2, 0, 0, false);
          }
          break;
      }
    } while (u8g2.nextPage());
}

void UIModule::drawInfoScreen() {
    char buffer[32];
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 12, "Temperatura:");
    snprintf(buffer, sizeof(buffer), "%.1f C / %d C", currentTemp, targetTemp);
    u8g2.drawStr(0, 26, buffer);
    u8g2.drawStr(0, 42, "Motor:");
    const char* motorStateStr = motorEnabled ? "ON" : "OFF";
    snprintf(buffer, sizeof(buffer), "%d RPM (%s)", motorRPM, motorStateStr);
    u8g2.drawStr(0, 56, buffer);
}

void UIModule::showInfoScreen() {
    currentState = STATE_INFO_SCREEN;
}