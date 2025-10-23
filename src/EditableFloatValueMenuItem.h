// FileName: EditableFloatValueMenuItem.h

#ifndef EDITABLEFLOATVALUEMENUITEM_H
#define EDITABLEFLOATVALUEMENUITEM_H

#include "MenuItem.h"

class EditableFloatValueMenuItem : public MenuItem {
private:
    float* value_ptr;
    const char* unit;
    float min_val, max_val;
    float step; // Cuánto aumenta/disminuye en cada giro del encoder
    bool isEditing;

public:
    EditableFloatValueMenuItem(const char* title, float* value_ptr, const char* unit, float min, float max, float step, MenuItem* parent = nullptr)
        : MenuItem(title, parent), value_ptr(value_ptr), unit(unit), min_val(min), max_val(max), step(step), isEditing(false) {}

    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        char buffer[32];
        
        // Usamos dtostrf para convertir float a string en Arduino
        char float_str[10];
        dtostrf(*value_ptr, 4, 2, float_str); // Formato: 4 caracteres, 2 decimales

        if (isEditing) {
            snprintf(buffer, sizeof(buffer), "%s[%s]%s", title, float_str, unit);
        } else {
            snprintf(buffer, sizeof(buffer), "%s%s%s", title, float_str, unit);
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
        if (!isEditing) {
            if (input == INPUT_SELECT) isEditing = true;
        } else {
            if (input == INPUT_NEXT) *value_ptr += step;
            if (input == INPUT_PREV) *value_ptr -= step;
            *value_ptr = constrain(*value_ptr, min_val, max_val);

            if (input == INPUT_SELECT || input == INPUT_BACK) isEditing = false;
        }
        return this;
    }
};

#endif