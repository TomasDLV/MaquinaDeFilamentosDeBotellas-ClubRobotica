// FileName: SubMenu.h

#ifndef SUBMENU_H
#define SUBMENU_H

#include "MenuItem.h"

class SubMenu : public MenuItem {
public: 
    MenuItem** items;
    int itemCount;
    int selectedIndex;
    int topItemIndex; // Para controlar el scrolling

public:
    SubMenu(const char* title, MenuItem* parent, MenuItem** items, int itemCount) 
      : MenuItem(title, parent), items(items), itemCount(itemCount), selectedIndex(0), topItemIndex(0) 
    {
        if (items != nullptr) { 
            for (int i = 0; i < itemCount; i++) {
                if (items[i] != nullptr) {
                   // Opcional: items[i]->parent = this; 
                }
            }
        }
    }

    virtual bool isSubMenu() override { return true; }

    void draw(U8G2 &u8g2, int, int, bool) override {
        if (items == nullptr || itemCount == 0) return;

        u8g2.setFont(u8g2_font_6x10_tf);
        const int fontHeight = 12; 
        const int maxItemsOnScreen = u8g2.getDisplayHeight() / fontHeight;

        for (int i = 0; i < maxItemsOnScreen; ++i) {
            int currentItemIndex = topItemIndex + i;
            if (currentItemIndex >= itemCount) break; 

            // Dibujamos cada item
            items[currentItemIndex]->draw(u8g2, 0, fontHeight + i * fontHeight, currentItemIndex == selectedIndex);
        }
    }

    MenuItem* handleInput(MenuInput input) override {
        if (items == nullptr || itemCount == 0) { return this; } 

        MenuItem* selectedItem = items[selectedIndex];

        // --- 1. MODO EDICIÓN (Bloqueo) ---
        if (selectedItem->isEditing()) {
            selectedItem->handleInput(input);
            return this; 
        }

        // Definimos cuántos items caben en pantalla (Fijo: 5 líneas para fuente 6x10)
        const int MAX_ITEMS_VISIBLE = 5; 

        // --- 2. NAVEGACIÓN ---
        switch (input) {
            case INPUT_NEXT:
                { // <--- LLAVES AGREGADAS PARA CORREGIR ERROR "JUMP TO CASE"
                    selectedIndex = (selectedIndex + 1) % itemCount;
                    // Scroll Abajo
                    if (selectedIndex == 0) topItemIndex = 0;
                    else if (selectedIndex >= topItemIndex + MAX_ITEMS_VISIBLE) {
                        topItemIndex = selectedIndex - MAX_ITEMS_VISIBLE + 1;
                    }
                }
                break;

            case INPUT_PREV:
                { // <--- LLAVES AGREGADAS
                    selectedIndex = (selectedIndex - 1 + itemCount) % itemCount;
                    // Scroll Arriba
                    if (selectedIndex < topItemIndex) {
                        topItemIndex = selectedIndex;
                    }
                }
                break;

            case INPUT_SELECT:
                {
                    MenuItem* result = selectedItem->handleInput(input);

                    if (result == selectedItem) {
                        return this; 
                    }
                    return result;
                }
                break;

            case INPUT_BACK:
                if (parent != nullptr) return parent;
                break;
            
            default:
                break;
        }
        
        return this; 
    }
};

#endif