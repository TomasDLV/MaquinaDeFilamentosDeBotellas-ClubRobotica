// FileName: EditableValueMenuItem.h

#ifndef EDITABLEVALUEMENUITEM_H
#define EDITABLEVALUEMENUITEM_H

#include "MenuItem.h"
#include <Arduino.h> // Necesario para millis()

class EditableValueMenuItem : public MenuItem {
private:
    int* value_ptr;
    const char* unit;
    int min_val, max_val;
    bool m_isEditing;

    // --- VARIABLES DE ACELERACIÓN AJUSTADAS ---
    unsigned long lastInputTime;
    
    // CAMBIO 1: Aumentamos el tiempo de tolerancia a 200ms.
    // Como tu loop() es lento por los delays de temperatura, necesitamos ser más pacientes.
    const unsigned long fastThreshold = 200; 

    // CAMBIO 2: Salto de 5 en 5 (puedes cambiarlo a 10 si quieres más velocidad)
    const int fastStep = 5;    

public:
    EditableValueMenuItem(const char* title, int* value_ptr, const char* unit, int min, int max, MenuItem* parent = nullptr)
        : MenuItem(title, parent), value_ptr(value_ptr), unit(unit), min_val(min), max_val(max), m_isEditing(false), lastInputTime(0) {}

    virtual bool isEditing() override { return m_isEditing; }

    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        char buffer[32];
        
        if (m_isEditing) {
            snprintf(buffer, sizeof(buffer), "%s[%d]%s", title, *value_ptr, unit);
        } else {
            snprintf(buffer, sizeof(buffer), "%s%d%s", title, *value_ptr, unit);
        }

        if (selected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }

        u8g2.drawStr(x + 2, y, buffer);
        u8g2.setDrawColor(1);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (!m_isEditing) {
            if (input == INPUT_SELECT) {
                m_isEditing = true;
                return this; 
            }
        } else {
            // --- LÓGICA DE ACELERACIÓN ---
            int currentStep = 1; // Paso normal
            unsigned long now = millis();

            // Si el tiempo entre este giro y el anterior es menor a 200ms, activamos TURBO
            if (now - lastInputTime < fastThreshold) {
                currentStep = fastStep; 
            }
            lastInputTime = now;

            // Aplicamos
            if (input == INPUT_NEXT) (*value_ptr) += currentStep;
            if (input == INPUT_PREV) (*value_ptr) -= currentStep;
            
            *value_ptr = constrain(*value_ptr, min_val, max_val);

            if (input == INPUT_SELECT || input == INPUT_BACK) {
                m_isEditing = false;
            }
        }
        return this; 
    }
};

#endif