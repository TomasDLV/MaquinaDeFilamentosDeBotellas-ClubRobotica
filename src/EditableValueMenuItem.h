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

    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        char buffer[32];
        
        if (isEditing) {
            snprintf(buffer, sizeof(buffer), "%s[%d]%s", title, *value_ptr, unit);
        } else {
            snprintf(buffer, sizeof(buffer), "%s%d%s", title, *value_ptr, unit);
        }

        // Usamos el método del padre para el resaltado estándar
        MenuItem::draw(u8g2, x, y, selected);
        // Pero dibujamos nuestro texto formateado encima
        u8g2.drawStr(x + 2, y, buffer);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (!isEditing) {
            if (input == INPUT_SELECT) isEditing = true;
        } else {
            if (input == INPUT_NEXT) (*value_ptr)++;
            if (input == INPUT_PREV) (*value_ptr)--;
            *value_ptr = constrain(*value_ptr, min_val, max_val);
            if (input == INPUT_SELECT || input == INPUT_BACK) isEditing = false;
        }
        return this;
    }
};

#endif