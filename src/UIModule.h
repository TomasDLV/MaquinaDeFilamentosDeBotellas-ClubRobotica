// FileName: UIModule.h

#ifndef UI_MODULE_H
#define UI_MODULE_H

#include <U8g2lib.h>
// #include <Wire.h> // No es necesario para SPI

// Pre-declaración de la clase MenuItem
class MenuItem;

// Enumeración para los estados de la UI
enum UIState {
    STATE_INFO_SCREEN,
    STATE_MENU
};

// --- Declaración de la Clase UIModule ---
class UIModule {
public:
    UIModule(); // Constructor
    void init();   // Método de inicialización
    void update(); // Bucle principal de la UI
    void showInfoScreen(); // Cambia al estado de pantalla de información
    
    // --- ELIMINADO: Ya no usamos ISRs ---
    // static void encoder_isr();
    // static void button_isr();

private:
    // Objeto U8g2 para la pantalla ST7920 con SPI por software
    U8G2_ST7920_128X64_F_SW_SPI u8g2;

    MenuItem* currentMenu; // Puntero al menú activo
    MenuItem* rootMenu; // Puntero al menu principal
    UIState currentState;  // Estado actual de la UI

    // --- Variables para Polling (no bloqueante) ---
    int8_t lastEncoderState; // Guarda el estado combinado anterior de los pines A y B

    // --- Variables CORREGIDAS para Debounce del Botón ---
    int lastReadingState;       // Guarda la lectura del pin en el ciclo ANTERIOR
    int buttonState;            // Guarda el estado ESTABLE y DEBOUNCED actual (HIGH o LOW)
    unsigned long lastDebounceTime; // Guarda el tiempo del último cambio detectado
    static const unsigned long debounceDelay = 7; // Tiempo (ms) para estabilizar

    // Métodos privados
    void buildMenu();      // Construye la estructura del menú
    void draw();           // Dibuja la pantalla actual
    void drawInfoScreen(); // Dibuja la pantalla de información

    //Eliminado
    // // Funciones privadas para leer las entradas por polling
    // int readEncoder();     // Lee el giro del encoder
    // bool readButton();     // Lee la pulsación del botón (con debounce)
};

#endif // UI_MODULE_H