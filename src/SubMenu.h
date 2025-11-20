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
        for (int i = 0; i < itemCount; i++) {
            if (items[i] != nullptr) {
                items[i]->parent = this;
            }
        }
    }

    virtual bool isSubMenu() override { return true; }

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
        if (itemCount == 0) { return this; } // Menú vacío, no hacer nada

        // Obtenemos el ítem que está seleccionado actualmente
        MenuItem* selectedItem = items[selectedIndex];

        // --- INICIO DE LA NUEVA LÓGICA DE BLOQUEO ---
        // 1. PRIMERO, preguntamos si el ítem actual está en modo edición
        if (selectedItem->isEditing()) {
            
            // Si SÍ está editando, le pasamos TODOS los comandos (giro y clic)
            // a ese ítem. El ítem se encargará de cambiar el valor o de salir
            // del modo edición si se presiona SELECT.
            selectedItem->handleInput(input);
            
            // No hacemos nada más. Retornamos 'this' para que la UI
            // no cambie de menú. El selector NO se moverá.
            return this;
        }
        // --- FIN DE LA NUEVA LÓGICA DE BLOQUEO ---

        // 2. Si NO está editando, procesamos la navegación normal
        switch (input) {
            case INPUT_NEXT:
                selectedIndex = (selectedIndex + 1) % itemCount;
                // Lógica de scrolling
                if (selectedIndex == 0) topItemIndex = 0;
                else if (selectedIndex >= topItemIndex + maxItemsOnScreen) {
                    topItemIndex = selectedIndex - maxItemsOnScreen + 1;
                }
                break;

            case INPUT_PREV:
                selectedIndex = (selectedIndex - 1 + itemCount) % itemCount;
                // Lógica de scrolling
                if (selectedIndex < topItemIndex) {
                    topItemIndex = selectedIndex;
                }
                break;

            case INPUT_SELECT:
                // El primer ítem (índice 0) siempre es "Volver"
                if (selectedIndex == 0 && parent != nullptr) {
                    return parent; // Volver al menú padre
                }
                
                // Le pasamos el clic al ítem
                selectedItem->handleInput(input);
                
                // Si el ítem es OTRO SubMenu, navegamos hacia él
                if (selectedItem->isSubMenu()) {
                    return selectedItem;
                }
                // Si no, era un ítem de Acción o Editable, nos quedamos aquí
                break;

            case INPUT_BACK:
                if (parent != nullptr) return parent;
                break;
            
            case INPUT_NONE:
            default:
                break; // No hacer nada
        }
        
        return this; // Quedarse en este menú
    }
};

#endif