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

// --- CLASE ESPECIAL PARA EL BOTÓN "VOLVER" ---
class BackMenuItem : public MenuItem
{
private:
    MenuItem *targetMenu; // Guardamos explícitamente a dónde queremos ir

public:
    // El constructor recibe el menú de destino (target)
    BackMenuItem(const char *title, MenuItem *target)
        : MenuItem(title, nullptr), targetMenu(target) {}
    // Pasamos nullptr al padre base porque usaremos nuestro propio 'targetMenu'

    void draw(U8G2 &u8g2, int x, int y, bool selected) override
    {
        if (selected)
        {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        }
        else
        {
            u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x + 2, y, title);
        u8g2.setDrawColor(1);
    }

    MenuItem *handleInput(MenuInput input) override
    {
        // Al hacer click, devolvemos el menú objetivo que guardamos
        if (input == INPUT_SELECT)
            return targetMenu;
        return this;
    }
};
// --- CLASE PARA ENLAZAR SUBMENÚS (MenuLink) ---
// Esta clase dibuja un botón normal, pero al hacer click te lleva al submenú destino.
class MenuLink : public MenuItem
{
private:
    MenuItem *targetMenu; // A qué menú nos lleva

public:
    MenuLink(const char *title, MenuItem *target)
        : MenuItem(title, nullptr), targetMenu(target) {}

    void draw(U8G2 &u8g2, int x, int y, bool selected) override
    {
        // Dibujamos solo el TÍTULO (como un botón normal)
        if (selected)
        {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        }
        else
        {
            u8g2.setDrawColor(1);
        }
        // Agregamos una flechita ">" para indicar que es un submenú
        u8g2.drawStr(x + 2, y, title);
        u8g2.drawStr(u8g2.getDisplayWidth() - 10, y, ">");
        u8g2.setDrawColor(1);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (input == INPUT_SELECT) return targetMenu;
        return this;
    }
};
// --- CLASE ESPECIAL: EJECUTAR Y VOLVER (ActionAndBackMenuItem) ---
// Sirve para el botón "SI": Guarda y regresa al menú anterior.
class ActionAndBackMenuItem : public MenuItem {
private:
    void (*action)();   // La función a ejecutar (do_saveSettings)
    MenuItem* target;   // A dónde volver (menuPrincipal)

public:
    ActionAndBackMenuItem(const char* title, void (*act)(), MenuItem* targetMenu)
        : MenuItem(title, nullptr), action(act), target(targetMenu) {}

    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        // Dibujado estándar
        if (selected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x + 2, y, title);
        u8g2.setDrawColor(1);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (input == INPUT_SELECT) {
            if (action) action(); // 1. Ejecuta el Guardado + Beep
            return target;        // 2. Devuelve el menú principal (Vuelve atrás)
        }
        return this;
    }
};
// --- Implementación de los Métodos de la Clase ---

// Constructor: Inicializa el objeto u8g2 con los pines SPI y las variables de polling
UIModule::UIModule() : u8g2(U8G2_R0, LCD_CLK_PIN, LCD_MOSI_PIN, LCD_CS_PIN)
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
void UIModule::init()
{
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
    // 1. Menú Principal (Contenedor)
    static SubMenu menuPrincipal("Menu Principal", nullptr, nullptr, 0); 

    // ==========================================
    // SUBMENÚ DE CONFIRMACIÓN (GUARDAR)
    // ==========================================
    
    // Botón NO: Simplemente vuelve al menú principal (usamos BackMenuItem)
    static BackMenuItem itemGuardarNO("No", &menuPrincipal);
    
    // Botón SI: Guarda, hace Beep y vuelve al menú principal
    static ActionAndBackMenuItem itemGuardarSI("Si", do_saveSettings, &menuPrincipal);

    // Lista de items del submenú
    static MenuItem* confirmarItems[] = { &itemGuardarNO, &itemGuardarSI };
    
    // El contenedor del submenú "¿Guardar?"
    // Nota: El padre es menuPrincipal
    static SubMenu menuConfirmar("¿Guardar?", &menuPrincipal, confirmarItems, 2);

    // ENLACE: Botón en el menú principal que lleva a la confirmación
    static MenuLink linkToGuardar("Guardar en Memoria", &menuConfirmar);


    // ==========================================
    // SUBMENÚ PID (El que ya tenías)
    // ==========================================
    static BackMenuItem itemPIDVolver("<- Volver", &menuPrincipal);
    static EditableFloatValueMenuItem itemPID_Kp("Kp: ", &kp, "", 0, 100, 0.5);
    static EditableFloatValueMenuItem itemPID_Ki("Ki: ", &ki, "", 0, 100, 0.1);
    static EditableFloatValueMenuItem itemPID_Kd("Kd: ", &kd, "", 0, 200, 1.0);
    
    static MenuItem* pidItems[] = {&itemPIDVolver, &itemPID_Kp, &itemPID_Ki, &itemPID_Kd};
    static SubMenu menuConfigPID("Configuracion PID", &menuPrincipal, pidItems, 4);
    static MenuLink linkToPID("Configuracion PID", &menuConfigPID);

    // ==========================================
    // MENÚ PRINCIPAL (ITEMS)
    // ==========================================
    static ActionMenuItem itemPrincipalInfo("<- Ver Info", do_showInfoScreen);
    static EditableValueMenuItem itemPrincipalTemp("Temperatura: ", &targetTemp, " C", 0, 260);
    static EditableFloatValueMenuItem itemPrincipalVel("Velocidad: ", &motorSpeed, " mm/s", 0.0, 15.0 , 0.1);
    static ActionMenuItem itemPrincipalHotend(&hotendEnabled,"Encender Hotend"," Apagar  Hotend", do_toggleHotend);
    static ActionMenuItem itemPrincipalMotor(&motorEnabled,"Encender Motor"," Apagar  Motor", do_toggleMotor);
    
    // YA NO USAMOS 'ActionMenuItem' DIRECTO, USAMOS EL LINK AL SUBMENÚ
    // static ActionMenuItem itemPrincipalGuardar("Guardar en Memoria", do_saveSettings); <--- BORRAR ESTO

    static MenuItem* mainMenuItems[] = { 
        &itemPrincipalInfo, 
        &itemPrincipalTemp, 
        &itemPrincipalVel, 
        &linkToPID,         
        &itemPrincipalHotend, 
        &itemPrincipalMotor, 
        &linkToGuardar       
    };

    menuPrincipal.items = mainMenuItems;
    menuPrincipal.itemCount = 7; 
    
    currentMenu = &menuPrincipal;
    rootMenu = &menuPrincipal;
}

// Bucle principal de la UI: Lee entradas, actualiza estado y dibuja
// En UIModule.cpp

void UIModule::update() {
  
  // 1. LEER DATOS DE LA ISR (Sección Crítica)
  // Leemos los contadores que la interrupción modificó en segundo plano.
  // Usamos noInterrupts() para asegurar que no cambien mientras los leemos.
  noInterrupts();
  int d = g_encoderDelta;       // Cuánto giró el encoder
  g_encoderDelta = 0;           // Reseteamos para la próxima
  bool clicked = g_buttonClicked; // Si se pulsó el botón
  g_buttonClicked = false;      // Reseteamos
  interrupts(); 
  
  // 2. TRADUCIR A EVENTOS DE MENÚ
  MenuInput input = INPUT_NONE;
  if (d > 0) input = INPUT_NEXT;
  if (d < 0) input = INPUT_PREV;
  if (clicked) input = INPUT_SELECT;

  // 3. MÁQUINA DE ESTADOS (Lógica de Navegación)
  switch (currentState) {
    case STATE_INFO_SCREEN:
      // Si estamos en la pantalla de información y se pulsa el botón, entramos al menú
      if (input == INPUT_SELECT) {
        currentState = STATE_MENU;
        currentMenu = rootMenu; // Volvemos a la raíz del menú
      }
      break;

    case STATE_MENU:
      // Si estamos en el menú y hay entrada, se la pasamos al ítem actual
      if (input != INPUT_NONE && currentMenu != nullptr) {
          MenuItem* nextMenu = currentMenu->handleInput(input);
          
          // Si handleInput devuelve un puntero diferente, cambiamos de menú
          // (Sirve para entrar a submenús o volver atrás)
          if (nextMenu != nullptr) {
             currentMenu = nextMenu;
          }
      }
      break;
  }
  
  // 4. DIBUJAR PANTALLA
  // Llamamos a draw() siempre. 
  // Aunque esto tarde 50ms, el motor NO se frenará porque ahora lo mueve la interrupción.
  draw();
}

// Función principal de dibujo: llama a la función específica según el estado
void UIModule::draw()
{
    u8g2.firstPage(); // Inicia el ciclo de dibujado de U8g2
    do
    {
        switch (currentState)
        {
        case STATE_INFO_SCREEN:
            drawInfoScreen(); // Dibuja la pantalla de estado
            break;
        case STATE_MENU:
            if (currentMenu != nullptr)
            {
                currentMenu->draw(u8g2, 0, 0, false); // Dibuja el menú actual
            }
            break;
        }
    } while (u8g2.nextPage()); // Termina el ciclo de dibujado
}

// Dibuja la pantalla de información con los datos actuales
void UIModule::drawInfoScreen()
{
    char buffer[32];
    int screenWidth = u8g2.getDisplayWidth();

    // ============================================
    // SECCIÓN SUPERIOR: HOTEND Y ESTADO
    // ============================================

    u8g2.setFont(u8g2_font_profont12_tf); // Fuente pequeña
    u8g2.drawStr(0, 10, "HOTEND");

    // --- NUEVO TESTIGO (INDICATOR) ---
    // Dibujamos el estado justo al lado del título "HOTEND"
    // Posición X calculada para que quede prolijo (aprox px 45)
    int statusX = 48;
    int statusY = 0; // Y inicial de la caja

    if (hotendEnabled)
    {
        // SI ESTÁ PRENDIDO: Caja negra con texto blanco (Invertido)
        u8g2.setDrawColor(1);
        u8g2.drawBox(statusX, statusY, 22, 11); // Caja de fondo
        u8g2.setDrawColor(0);                   // Texto en "blanco" (hueco)
        u8g2.drawStr(statusX + 3, statusY + 9, "ON");
    }
    else
    {
        // SI ESTÁ APAGADO: Marco vacío
        u8g2.setDrawColor(1);
        u8g2.drawFrame(statusX, statusY, 26, 11); // Marco un poco más ancho
        u8g2.drawStr(statusX + 3, statusY + 9, "OFF");
    }
    u8g2.setDrawColor(1); // Restauramos color normal siempre

    // ============================================
    // DATOS DE TEMPERATURA
    // ============================================

    // Temperatura Actual (EN GRANDE)
    u8g2.setFont(u8g2_font_helvB18_tr); // Fuente Grande
    int tempInt = (int)round(currentTemp);
    snprintf(buffer, sizeof(buffer), "%d", tempInt);
    u8g2.drawStr(0, 35, buffer);

    // Símbolo de grados pequeño
    int tempWidth = u8g2.getStrWidth(buffer);
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.drawStr(tempWidth + 2, 28, "o");

    // Temperatura Objetivo ("/ 200 C")
    snprintf(buffer, sizeof(buffer), "/ %d C", targetTemp);
    int targetWidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(screenWidth - targetWidth, 35, buffer);

    // Barra de Progreso
    u8g2.drawFrame(0, 40, screenWidth, 5);
    if (targetTemp > 0 && hotendEnabled)
    { // Solo muestra barra si está habilitado
        int barWidth = map(constrain(tempInt, 0, targetTemp), 0, targetTemp, 0, screenWidth - 2);
        u8g2.drawBox(1, 41, barWidth, 3);
    }

    // ============================================
    // SECCIÓN INFERIOR: MOTOR / EXTRUSIÓN
    // ============================================

    int yMotorBase = 62;

    // Estado del Motor (Igual estilo que arriba)
    u8g2.setFont(u8g2_font_profont12_tf);

    if (motorEnabled)
    {
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, yMotorBase - 11, 32, 13);
        u8g2.setDrawColor(0);
        u8g2.drawStr(4, yMotorBase, "RUN");
        u8g2.setDrawColor(1);
    }
    else
    {
        u8g2.drawFrame(0, yMotorBase - 11, 32, 13);
        u8g2.drawStr(2, yMotorBase, "STOP");
    }

    // Velocidad (mm/s)
    char speedStr[10];
    dtostrf(motorSpeed, 4, 1, speedStr);
    snprintf(buffer, sizeof(buffer), "%s mm/s", speedStr);

    u8g2.setFont(u8g2_font_6x12_tr);
    int speedWidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(screenWidth - speedWidth, yMotorBase, buffer);
}

// Función pública para cambiar al estado de pantalla de información
void UIModule::showInfoScreen()
{
    currentState = STATE_INFO_SCREEN;
}