// FileName: ActionMenuItem.h

#ifndef ACTIONMENUITEM_H
#define ACTIONMENUITEM_H

#include "MenuItem.h"

class ActionMenuItem : public MenuItem {
private:
    // Declarados primero
    bool* stateRef;         
    const char* titleOn;    

public:
    // Declarado después
    void (*action)(); 

    // --- CONSTRUCTOR 1: MODO CLÁSICO ---
    // CORRECCIÓN: Ordenamos la lista: primero variables padres, luego privadas, luego públicas
    ActionMenuItem(const char* title, void (*action)(), MenuItem* parent = nullptr) 
      : MenuItem(title, parent), stateRef(nullptr), titleOn(nullptr), action(action) {}

    // --- CONSTRUCTOR 2: MODO DINÁMICO ---
    // CORRECCIÓN: Mismo orden que la declaración arriba
    ActionMenuItem(bool* state, const char* titleOff, const char* titleOn, void (*action)(), MenuItem* parent = nullptr)
      : MenuItem(titleOff, parent), stateRef(state), titleOn(titleOn), action(action) {}


    void draw(U8G2 &u8g2, int x, int y, bool selected) override {
        const char* textToDraw = title;
        if (stateRef != nullptr && *stateRef == true) {
            textToDraw = titleOn; 
        }

        if (selected) { 
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        } else { 
            u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x + 2, y, textToDraw);
        u8g2.setDrawColor(1);
    }

    MenuItem* handleInput(MenuInput input) override {
        if (input == INPUT_SELECT && action != nullptr) {
            action(); 
        }
        return this; 
    }
};

#endif