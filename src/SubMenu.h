// FileName: SubMenu.h

#ifndef SUBMENU_H
#define SUBMENU_H

// #include <vector> // <-- Ya no necesitamos esta librería

#include "MenuItem.h"

class SubMenu : public MenuItem {
private:
    MenuItem** items;     // <-- Ahora es un puntero a un arreglo de punteros
    const int itemCount;  // <-- Guardamos el número de items
    int selectedIndex;

public:
    // El constructor ahora recibe el arreglo de items y su tamaño
    SubMenu(const char* title, MenuItem* parent, MenuItem** items, int itemCount) 
      : MenuItem(title, parent), items(items), itemCount(itemCount), selectedIndex(0) 
    {
        // Asignamos este submenú como el padre de todos sus items hijos
        for (int i = 0; i < itemCount; i++) {
            if (items[i] != nullptr) {
                items[i]->parent = this;
            }
        }
    }

    void draw(U8G2 &u8g2, int, int, bool) override {
        u8g2.setFont(u8g2_font_6x10_tf);
        for (int i = 0; i < itemCount; ++i) {
            items[i]->draw(u8g2, 0, 12 + i * 12, i == selectedIndex);
        }
    }

    MenuItem* handleInput(MenuInput input) override {
        if (itemCount == 0) { // Prevenir error si el menú está vacío
             if (input == INPUT_BACK && parent != nullptr) return parent;
             return this;
        }
        if (input == INPUT_NEXT) {
            selectedIndex = (selectedIndex + 1) % itemCount;
        } else if (input == INPUT_PREV) {
            selectedIndex = (selectedIndex - 1 + itemCount) % itemCount;
        } else if (input == INPUT_SELECT) {
            return items[selectedIndex]->handleInput(input);
        } else if (input == INPUT_BACK) {
            if (parent != nullptr) return parent;
        }
        return this;
    }
};

#endif