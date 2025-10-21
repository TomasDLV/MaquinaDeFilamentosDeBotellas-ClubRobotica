// FileName: MenuItem.h

#ifndef MENUITEM_H
#define MENUITEM_H

#include <U8g2lib.h>

enum MenuInput { INPUT_NONE, INPUT_NEXT, INPUT_PREV, INPUT_SELECT, INPUT_BACK };

class MenuItem {
public:
    const char* title;
    MenuItem* parent;

    MenuItem(const char* title, MenuItem* parent = nullptr) : title(title), parent(parent) {}
    virtual ~MenuItem() {}

    virtual void draw(U8G2 &u8g2, int x, int y, bool selected) {
        if (selected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x + 2, y, title);
        u8g2.setDrawColor(1);
    }

    virtual MenuItem* handleInput(MenuInput input) = 0;
};

#endif