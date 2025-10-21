// FileName: UIModule.cpp

#include "UIModule.h"
#include "Config.h"
#include "MenuItem.h"
#include "ActionMenuItem.h"
#include "SubMenu.h"
#include "EditableValueMenuItem.h"
#include <Arduino.h>

// --- Variables globales externas ---
extern int targetTemp;
extern int motorRPM;
extern void do_toggleMotor();
extern void do_toggleHotend();
extern void do_dummy_function();
extern void do_saveSettings();

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
  // Se debe usar el nombre de la clase para acceder a miembros estáticos
  UIModule::encoderDelta += table[lastState & 0x0F];
}

void UIModule::button_isr() {
  // Se debe usar el nombre de la clase para acceder a miembros estáticos
  UIModule::buttonPressed = true;
}

// --- Implementación de la clase ---
UIModule::UIModule() : u8g2(U8G2_R0, LCD_CLK_PIN, LCD_MOSI_PIN, LCD_CS_PIN) {
    currentMenu = nullptr;
}

void UIModule::init() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);

  // Al adjuntar la interrupción, se usa la función estática
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), UIModule::encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), UIModule::encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_BTN_PIN), UIModule::button_isr, FALLING);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);

  buildMenu();
}

void UIModule::buildMenu() {
    static ActionMenuItem itemConfigVolver("<- Volver", do_dummy_function);
    static EditableValueMenuItem itemConfigTemp("Temp. Target: ", &targetTemp, "C", 0, 260);
    static EditableValueMenuItem itemConfigVel("Velocidad: ", &motorRPM, " RPM", 0, 120);
    static ActionMenuItem itemConfigGuardar("Guardar Config", do_saveSettings);

    static MenuItem* configItems[] = {&itemConfigVolver, &itemConfigTemp, &itemConfigVel, &itemConfigGuardar};
    static SubMenu menuConfig("Configuracion", nullptr, configItems, 4);

    static ActionMenuItem itemPrincipalHotend("Encender Hotend", do_toggleHotend);
    static ActionMenuItem itemPrincipalMotor("Encender Motor", do_toggleMotor);

    static MenuItem* mainMenuItems[] = {&itemPrincipalHotend, &itemPrincipalMotor, &menuConfig};
    static SubMenu menuPrincipal("Menu Principal", nullptr, mainMenuItems, 3);

    currentMenu = &menuPrincipal;
}

void UIModule::update() {
  noInterrupts();
  // Aquí no se usa el prefijo porque estamos en una función normal (no estática)
  int d = encoderDelta / 4;
  encoderDelta = 0; // Reseteamos a 0 para no acumular movimientos
  bool clicked = buttonPressed;
  buttonPressed = false;
  interrupts();

  MenuInput input = INPUT_NONE;
  if (d > 0) input = INPUT_NEXT;
  if (d < 0) input = INPUT_PREV;
  if (clicked) input = INPUT_SELECT;

  if (input != INPUT_NONE && currentMenu != nullptr) {
      MenuItem* nextMenu = currentMenu->handleInput(input);
      if (nextMenu != nullptr) { // Simplificamos la lógica de navegación
         currentMenu = nextMenu;
      }
  }
  
  draw();
}

void UIModule::draw() {
    u8g2.firstPage();
    do {
        if (currentMenu != nullptr) {
            currentMenu->draw(u8g2, 0, 0, false);
        }
    } while (u8g2.nextPage());
}