// FileName: UIModule.h

#ifndef UI_MODULE_H
#define UI_MODULE_H

#include <U8g2lib.h>
#include <Wire.h> // Necesario para la pantalla SSD1306 (I2C)

// Pre-declaración de la clase MenuItem para evitar errores de compilación
class MenuItem; 

// Enumeración para definir los posibles estados de la interfaz de usuario.
enum UIState {
    STATE_INFO_SCREEN, // Muestra la pantalla de estado
    STATE_MENU         // Muestra el menú de navegación
};

// --- Declaración de la Clase UIModule ---
class UIModule {
public:
    UIModule(); // Constructor
    void init();   // Método de inicialización
    void update(); // Bucle principal de la UI
    
    // Método público para permitir que 'main.cpp' nos mande a la pantalla de info
    void showInfoScreen(); 

    // Funciones de interrupción (ISRs). Deben ser 'static'
    static void encoder_isr();
    static void button_isr();

private:
    // Objeto de la librería U8g2 para la pantalla SSD1306 128x64 I2C
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

    MenuItem* currentMenu; // Puntero que apunta al menú que se está mostrando.
    UIState currentState;  // Variable que guarda el estado actual de la UI.

    // Variables estáticas y volátiles para las interrupciones
    volatile static int encoderDelta;
    volatile static bool buttonPressed;

    // Métodos privados (solo usados por esta clase)
    void buildMenu();      // Construye la estructura del menú
    void draw();           // Dibuja la pantalla actual
    void drawInfoScreen(); // Dibuja la pantalla de información
};

#endif