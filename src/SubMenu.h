// FileName: SubMenu.h

#ifndef SUBMENU_H
#define SUBMENU_H

#include "MenuItem.h"

class SubMenu : public MenuItem {
private:
    MenuItem** items;
    const int itemCount;
    int selectedIndex;
    int topItemIndex; // Para controlar el scrolling

public:
    SubMenu(const char* title, MenuItem* parent, MenuItem** items, int itemCount) 
      : MenuItem(title, parent), items(items), itemCount(itemCount), selectedIndex(0), topItemIndex(0) 
    {
        for (int i = 0; i < itemCount; i++) {
            if (items[i] != nullptr) {
                items[i]->parent = this;
            }
        }
    }

    void draw(U8G2 &u8g2, int, int, bool) override {
        u8g2.setFont(u8g2_font_6x10_tf);
        const int fontHeight = 12; // Altura de línea para nuestra fuente
        const int maxItemsOnScreen = u8g2.getDisplayHeight() / fontHeight;

        for (int i = 0; i < maxItemsOnScreen; ++i) {
            int currentItemIndex = topItemIndex + i;
            if (currentItemIndex >= itemCount) break; // No dibujar más allá de la lista

            items[currentItemIndex]->draw(u8g2, 0, fontHeight + i * fontHeight, currentItemIndex == selectedIndex);
        }
    }

    MenuItem* handleInput(MenuInput input) override {
        const int maxItemsOnScreen = 5; // Asumimos 5 items visibles

        if (input == INPUT_NEXT) {
            selectedIndex = (selectedIndex + 1) % itemCount;
            // Lógica de scrolling hacia abajo
            if (selectedIndex > topItemIndex + maxItemsOnScreen - 1) {
                topItemIndex = selectedIndex - maxItemsOnScreen + 1;
            }
            if (selectedIndex == 0) topItemIndex = 0; // Volver al inicio
        } else if (input == INPUT_PREV) {
            selectedIndex = (selectedIndex - 1 + itemCount) % itemCount;
            // Lógica de scrolling hacia arriba
            if (selectedIndex < topItemIndex) {
                topItemIndex = selectedIndex;
            }
        } else if (input == INPUT_SELECT) {
            // El primer ítem (índice 0) siempre es "Volver"
            if (selectedIndex == 0 && parent != nullptr) {
                return parent;
            }
            return items[selectedIndex]->handleInput(input);
        } else if (input == INPUT_BACK) {
            if (parent != nullptr) return parent;
        }
        return this;
    }
};

#endif