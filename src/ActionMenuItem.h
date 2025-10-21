// FileName: ActionMenuItem.h

#ifndef ACTIONMENUITEM_H
#define ACTIONMENUITEM_H

#include "MenuItem.h"

class ActionMenuItem : public MenuItem {
public:
    void (*action)(); 

    ActionMenuItem(const char* title, void (*action)(), MenuItem* parent = nullptr) 
      : MenuItem(title, parent), action(action) {}

    MenuItem* handleInput(MenuInput input) override {
        if (input == INPUT_SELECT && action != nullptr) {
            action(); 
        }
        return this; 
    }
};

#endif