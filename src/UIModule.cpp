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
extern float motorSpeed;
extern bool motorEnabled;
extern bool hotendEnabled;
extern double currentTemp;
extern bool filamentStatus;
extern float kp, ki, kd;
// Variables 'extern' para leer los datos de la IS
extern volatile int g_encoderDelta;
extern volatile bool g_buttonClicked;

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
    /*
    VISTA MENU:
    
    <- Ver Info
    Temperatura: * °
    Encender Hotend
    Velocidad: * RPM
    Encender Motor
    Configuracion PID  :
        | <- Volver
        | Kp:
        | Ki:
        | Kd:
    Guardar en Memoria

    
    */
    static ActionMenuItem itemPIDVolver("<- Volver", do_dummy_function);
    static EditableFloatValueMenuItem itemPID_Kp("Kp: ", &kp, "", 0, 100, 0.5);
    static EditableFloatValueMenuItem itemPID_Ki("Ki: ", &ki, "", 0, 100, 0.1);
    static EditableFloatValueMenuItem itemPID_Kd("Kd: ", &kd, "", 0, 200, 1.0);
    static MenuItem* pidItems[] = {&itemPIDVolver, &itemPID_Kp, &itemPID_Ki, &itemPID_Kd};
    static SubMenu menuConfigPID("Configuracion PID", nullptr, pidItems, 4);

    static ActionMenuItem itemPrincipalInfo("<- Ver Info", do_showInfoScreen);
    static EditableValueMenuItem itemPrincipalTemp("Temperatura: ", &targetTemp, " C", 0, 260);
    static EditableFloatValueMenuItem itemPrincipalVel("Velocidad: ", &motorSpeed, " mm/s", 0.0, 15.0 , 0.1);
    static ActionMenuItem itemPrincipalHotend(&hotendEnabled,"Encender Hotend"," Apagar  Hotend", do_toggleHotend);
    static ActionMenuItem itemPrincipalMotor(&motorEnabled,"Encender Motor"," Apagar  Motor", do_toggleMotor);
    static ActionMenuItem itemPrincipalGuardar("Guardar en Memoria", do_saveSettings);
    static MenuItem* mainMenuItems[] = { &itemPrincipalInfo, &itemPrincipalTemp, &itemPrincipalVel/*, &menuConfigPID*/, &itemPrincipalHotend, &itemPrincipalMotor, &itemPrincipalGuardar };
    static SubMenu menuPrincipal("Menu Principal", nullptr, mainMenuItems, 6);

    currentMenu = &menuPrincipal;
    rootMenu = &menuPrincipal;

    menuConfigPID.parent = &menuPrincipal;
}

// Bucle principal de la UI: Lee entradas, actualiza estado y dibuja
void UIModule::update() {
  // 1. Lee los valores de las variables globales que la ISR actualiza
  noInterrupts(); // Desactiva interrupciones brevemente para leer
  int d = g_encoderDelta;
  g_encoderDelta = 0; // Resetea el delta
  bool clicked = g_buttonClicked;
  g_buttonClicked = false; // Resetea el click
  interrupts(); // Reactiva interrupciones
  

  // 2. Convierte los deltas en eventos de menú
  MenuInput input = INPUT_NONE;
  if (d > 0) input = INPUT_NEXT;
  if (d < 0) input = INPUT_PREV;
  if (clicked) input = INPUT_SELECT;

  // 3. La máquina de estados procesa los eventos
  switch (currentState) {
    case STATE_INFO_SCREEN:
      if (input == INPUT_SELECT) {
        currentState = STATE_MENU;
        currentMenu = rootMenu; // Resetea al menú principal
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
  
  // 4. Se redibuja la pantalla (esta es la parte lenta del loop)
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
    int screenWidth = u8g2.getDisplayWidth();
    
    // ============================================
    // SECCIÓN SUPERIOR: TEMPERATURA (Dashboard)
    // ============================================
    
    // 1. Título pequeño
    u8g2.setFont(u8g2_font_profont12_tf); // Fuente pequeña y limpia
    u8g2.drawStr(0, 10, "HOTEND");

    // 2. Temperatura Actual (EN GRANDE)
    u8g2.setFont(u8g2_font_helvB18_tr); // Fuente Helvetica Negrita Grande
    int tempInt = (int)round(currentTemp);
    snprintf(buffer, sizeof(buffer), "%d", tempInt);
    
    // Calculamos ancho para alinear a la derecha o dejarlo fijo
    u8g2.drawStr(0, 35, buffer); 
    
    // Dibujamos el símbolo de grados pequeño al lado
    int tempWidth = u8g2.getStrWidth(buffer);
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.drawStr(tempWidth + 2, 28, "o"); // Simula el símbolo de grados

    // 3. Temperatura Objetivo (Texto pequeño "/ 200")
    u8g2.setFont(u8g2_font_profont12_tf);
    snprintf(buffer, sizeof(buffer), "/ %d C", targetTemp);
    // Lo alineamos a la derecha de la pantalla
    int targetWidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(screenWidth - targetWidth, 35, buffer);

    // 4. Barra de Progreso de Temperatura
    // Dibuja un marco
    u8g2.drawFrame(0, 40, screenWidth, 6); 
    if (targetTemp > 0) {
        // Calcula el ancho de la barra (mapeo simple)
        int barWidth = map(constrain(tempInt, 0, targetTemp), 0, targetTemp, 0, screenWidth - 2);
        // Dibuja el relleno
        u8g2.drawBox(1, 41, barWidth, 4);
    }

    // ============================================
    // SECCIÓN INFERIOR: MOTOR / EXTRUSIÓN
    // ============================================
    
    int yMotorBase = 62; // Base de la línea de texto inferior

    // 1. Estado del Motor (Caja visual)
    u8g2.setFont(u8g2_font_profont12_tf); // Volvemos a fuente normal
    
    if (motorEnabled) {
        // Si está ENCENDIDO: Caja negra con texto blanco (Invertido)
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, yMotorBase - 11, 32, 13); // Caja de fondo
        u8g2.setDrawColor(0); // Texto transparente (color fondo)
        u8g2.drawStr(4, yMotorBase, "RUN");
        u8g2.setDrawColor(1); // Restaurar color normal
    } else {
        // Si está APAGADO: Caja vacía (Marco) con texto normal
        u8g2.drawFrame(0, yMotorBase - 11, 32, 13);
        u8g2.drawStr(2, yMotorBase, "STOP");
    }

    // 2. Velocidad (Alineada a la derecha)
    // Convertimos el float a texto
    char speedStr[10];
    dtostrf(motorSpeed, 4, 1, speedStr); // ej: " 2.5"
    snprintf(buffer, sizeof(buffer), "%s mm/s", speedStr);
    
    // Usamos una fuente negrita media para el número
    u8g2.setFont(u8g2_font_6x12_tr); 
    int speedWidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(screenWidth - speedWidth, yMotorBase, buffer);
}

// Función pública para cambiar al estado de pantalla de información
void UIModule::showInfoScreen() {
    currentState = STATE_INFO_SCREEN;
}