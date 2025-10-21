// FileName: UIModule.h

#ifndef UI_MODULE_H
#define UI_MODULE_H

#include <U8g2lib.h>

// Adelantamos la declaración de la clase para evitar dependencias circulares
class MenuItem; 

class UIModule {
public:
    UIModule();
    void init();
    void update();

    // ISRs deben ser estáticas
    static void encoder_isr();
    static void button_isr();

private:
    U8G2_ST7920_128X64_F_SW_SPI u8g2;
    MenuItem* currentMenu;

    // Variables para manejar las interrupciones
    volatile static int encoderDelta;
    volatile static bool buttonPressed;

    void buildMenu();
    void draw();
};

#endif