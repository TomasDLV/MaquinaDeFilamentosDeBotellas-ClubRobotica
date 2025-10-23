// FileName: EditableValueMenuItem.h

#ifndef EDITABLEVALUEMENUITEM_H
#define EDITABLEVALUEMENUITEM_H

#include "MenuItem.h"

class EditableValueMenuItem : public MenuItem {
private:
    int* value_ptr;
    const char* unit;
    int min_val, max_val;
    bool isEditing;

public:
    EditableValueMenuItem(const char* title, int* value_ptr, const char* unit, int min, int max, MenuItem* parent = nullptr)
        : MenuItem(title, parent), value_ptr(value_ptr), unit(unit), min_val(min), max_val(max), isEditing(false) {}

    // --- FUNCIÓN DE DIBUJADO CORREGIDA ---
    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        char buffer[32];
        
        // Primero, preparamos el texto que vamos a mostrar
        if (isEditing) {
            snprintf(buffer, sizeof(buffer), "%s[%d]%s", title, *value_ptr, unit);
        } else {
            snprintf(buffer, sizeof(buffer), "%s%d%s", title, *value_ptr, unit);
        }

        // Ahora, manejamos el dibujado nosotros mismos
        if (selected) {
            // Si el ítem está seleccionado, dibujamos el fondo y el texto invertido
            u8g2.setDrawColor(1); // Color del pincel a blanco
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0); // Color del pincel a negro para el texto
        } else {
            // Si no está seleccionado, solo fijamos el color del pincel a blanco
            u8g2.setDrawColor(1);
        }

        // Finalmente, dibujamos nuestro texto formateado
        u8g2.drawStr(x + 2, y, buffer);

        // MUY IMPORTANTE: Dejamos el pincel en blanco para el resto de dibujos
        u8g2.setDrawColor(1);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (!isEditing) {
            if (input == INPUT_SELECT) {
                isEditing = true;
                return this; // Nos quedamos en este item para editar
            }
        } else {
            if (input == INPUT_NEXT) (*value_ptr)++;
            if (input == INPUT_PREV) (*value_ptr)--;
            *value_ptr = constrain(*value_ptr, min_val, max_val);

            // Si se presiona SELECT de nuevo, salimos del modo edición
            if (input == INPUT_SELECT || input == INPUT_BACK) {
                isEditing = false;
                // No retornamos 'parent' aquí para quedarnos en el menú actual,
                // simplemente salimos del modo edición.
            }
        }
        return this; // El control siempre se queda aquí hasta que se navega fuera
    }
};

#endif