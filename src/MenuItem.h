// FileName: MenuItem.h
// Define la idea abstracta de lo que es cualquier parte del item del menú

#ifndef MENUITEM_H
#define MENUITEM_H

#include <U8g2lib.h> // Biblioteca de gráficos que sabe como dibujar pixeles, fromas y texto en pantalla

enum MenuInput { INPUT_NONE, INPUT_NEXT, INPUT_PREV, INPUT_SELECT, INPUT_BACK };
// Representan las posibles entradas del usuario (botones)
//INPUT_NEXT (siguiente), INPUT_PREV (anterior), INPUT_SELECT (seleccionar) y INPUT_BACK (atrás)


class MenuItem {
public:
    const char* title; // Texto a mostrarse en la pantalla
    MenuItem* parent; //Puntero al item padre. Necesario para crear submenus y poder volver atrás.

    MenuItem(const char* title, MenuItem* parent = nullptr) : title(title), parent(parent) {} // Inicializa el título y el padre
    virtual ~MenuItem() {} // Las funciones virtuales son las que se espera que sean sobrescritas (redefinidas) en las clases derivadas

    virtual void draw(U8G2 &u8g2, int x, int y, bool selected) { //Dibuja el item del menú en la pantalla. Recibe el objeto de la pantalla (u8g2), las coordenadas (x, y) y un booleano selected.

        if (selected) { // Si selected es true, dibuja un rectángulo relleno (para resaltarlo) y luego dibuja el texto en color "invertido" (0).
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y - u8g2.getAscent(), u8g2.getDisplayWidth(), u8g2.getAscent() + 2); // (x_inicial, y_inicial, ancho, alto). Dibuja un rectángulo relleno del color actual.
            // La x establece el inicio del rectángulo, la y se ajusta para que el rectángulo cubra el texto.
            /*y es la coordenada Y que recibe la función draw. En U8g2, y no es la parte superior del texto, sino la línea base (la línea donde se "sientan" letras).
            getAscent devuelve la "altura ascendente" de la fuente actual. Es la distancia en píxeles desde la línea base hasta la parte superior del carácter más alto (como 'H' o 'k').
            Por lo tanto, y - u8g2.getAscent() calcula la coordenada Y exacta de la parte superior del texto. Esto hace que la caja de resaltado comience justo encima de las letras.
            El 4to parametro es el alto, se añaden 2 píxeles de padding. Esto es para que la caja sea un poco más alta que el texto.*/

            u8g2.setDrawColor(0);
        } else { //  Si selected es false, simplemente dibuja el texto.
            u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x + 2, y, title); //Esta línea dibuja el texto a mostrar (title) del ítem en la pantalla. El primer parametro es la coordenada X (horizontal), empieza 2 píxeles a la derecha de x para crear un pequeño margen. El segundo parametro la coordenada Y (vertical), que define la línea base del texto.
        u8g2.setDrawColor(1);
    }

    virtual MenuItem* handleInput(MenuInput input) = 0; // Esto establece que MenuItem es una clase abstracta. Las clases derivadas DEBEN implementar esta función.
    //Su trabajo es definir qué hacer cuando el usuario presiona un botón.
};

#endif