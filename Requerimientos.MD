Proyecto: Máquina Extrusora de Filamento PET
Versión 1.0 Fecha: 11 de Octubre de 2025 Autor: [Tu Nombre/Equipo]

Índice
Resumen del Proyecto

Características Principales

Arquitectura del Software

Diagrama de Componentes

Módulos Principales

Sistema de Menús

Requisitos de Hardware y Software

Guía de Configuración y Puesta en Marcha

Guía de Uso

Futuras Mejoras

1. Resumen del Proyecto
Este proyecto consiste en el diseño y la implementación del firmware para una máquina extrusora de filamento de código abierto. El objetivo principal es reciclar botellas de plástico (PET) para convertirlas en filamento utilizable en impresoras 3D FDM.

El sistema se controla mediante una placa Arduino Mega 2560 con un shield RAMPS 1.4 y se opera a través de una pantalla LCD con un menú interactivo, permitiendo un control preciso de la temperatura de extrusión y la velocidad del motor. El software ha sido diseñado con un enfoque en la modularidad, la eficiencia y la operación no bloqueante para garantizar la máxima estabilidad y capacidad de respuesta.

2. Características Principales
Control de Temperatura Preciso: Implementación de un controlador PID robusto para mantener una temperatura de extrusión estable, utilizando la librería PID_v1.

Lectura de Temperatura Calibrada: Uso de la ecuación Beta para una conversión precisa de la resistencia del termistor NTC a grados Celsius.

Control de Motor Avanzado: Manejo del motor de extrusión mediante la librería AccelStepper, permitiendo un control fino de la velocidad (en RPM) y la aceleración.

Interfaz de Usuario Intuitiva: Menú navegable en una pantalla LCD gráfica, construido con un sistema de clases orientado a objetos que facilita la modificación y expansión.

Persistencia de Configuración: Almacenamiento de la velocidad del motor en la memoria EEPROM para retener los ajustes entre reinicios.

Arquitectura Modular: El código está separado en módulos lógicos (TemperatureModule, ExtrusionModule, UIModule), lo que facilita el mantenimiento, la depuración y la adición de nuevas funcionalidades.

Operación No Bloqueante: Todo el sistema opera sin usar delay(), asegurando que la interfaz de usuario sea siempre responsiva mientras los procesos de control (PID y motor) se ejecutan en segundo plano.

3. Arquitectura del Software
El firmware sigue un patrón de diseño que se asemeja a Model-View-Controller (MVC), separando la lógica de control, los datos y la interfaz de usuario.

3.1. Diagrama de Componentes
+--------------------------+       +-------------------------+
|   FilamentMaker.ino      |------>|       UIModule          |
|   (Controlador Principal)|       | (Vista / Controlador UI)|
+--------------------------+       +-------------------------+
             |                                |
             |                                V
             |                +------------------------------+
             |                |      Sistema de Menús        |
             |                | (MenuItem, SubMenu, etc.)    |
             |                +------------------------------+
             |
             +--------------->+-------------------------+
             |                |   TemperatureModule     |
             |                |        (Modelo)         |
             |                +-------------------------+
             |
             +--------------->+-------------------------+
                              |    ExtrusionModule      |
                              |        (Modelo)         |
                              +-------------------------+
3.2. Módulos Principales
FilamentMaker.ino: Es el punto de entrada y el orquestador principal. Su responsabilidad es inicializar todos los módulos en setup() y llamar a sus respectivos métodos update() en un bucle cerrado en loop(). Contiene las variables de estado globales (ej: motorRPM) y las funciones de acción que el menú ejecuta.

Config.h: Archivo de cabecera centralizado que contiene todas las constantes de configuración del hardware (pines, parámetros del termistor, ajustes del motor) y del software (ganancias PID, tiempo de muestreo). Permite una fácil calibración y adaptación a diferentes montajes mecánicos.

TemperatureModule: Encapsula toda la lógica relacionada con el control de temperatura.

Lee el termistor y calcula la temperatura real.

Gestiona el objeto de la librería PID_v1.

Controla la salida PWM del calentador.

ExtrusionModule: Encapsula toda la lógica de control del motor.

Gestiona el objeto de la librería AccelStepper.

Convierte las RPM deseadas a pasos/segundo.

Maneja el guardado y la carga de la velocidad en la EEPROM.

UIModule: Gestiona toda la interacción con el usuario.

Dibuja el menú y la información en la pantalla LCD.

Captura las entradas del encoder rotativo y el botón mediante interrupciones (ISRs) para no perder ninguna acción del usuario.

Construye la estructura del menú y gestiona la navegación.

3.3. Sistema de Menús
El menú es una estructura de datos dinámica construida con clases.

MenuItem: Clase base abstracta que define la interfaz común para todos los elementos del menú (debe poder dibujarse y manejar entradas).

ActionMenuItem: Un MenuItem que ejecuta una función específica (mediante un puntero a función) cuando se selecciona.

SubMenu: Un MenuItem que contiene una lista (std::vector) de otros MenuItems, permitiendo la navegación anidada.

EditableValueMenuItem: Un MenuItem con estado interno que permite modificar una variable numérica directamente desde el menú.

4. Requisitos de Hardware y Software
Hardware
Placa de control: Arduino Mega 2560

Shield: RAMPS 1.4

Pantalla: LCD ST7920 Full Graphic (128x64)

Sensor de temperatura: Termistor NTC 100kΩ (con su correspondiente divisor de voltaje)

Motor de extrusión: Motor a pasos (ej: NEMA 17) con un driver compatible (ej: A4988, DRV8825)

Entrada de usuario: Encoder rotativo con pulsador

Software
Entorno de desarrollo: Visual Studio Code con la extensión PlatformIO.

Librerías (gestionadas automáticamente por platformio.ini):

U8g2 (para la pantalla LCD)

PID (para el control de temperatura)

AccelStepper (para el control del motor)

5. Guía de Configuración y Puesta en Marcha
Clonar/Descargar el Proyecto: Obtén todos los archivos del proyecto y guárdalos en una carpeta.

Abrir en PlatformIO: Abre la carpeta del proyecto en Visual Studio Code. PlatformIO debería reconocer el archivo platformio.ini e instalar todas las dependencias (lib_deps) automáticamente.

Calibrar Config.h: Este es el paso más importante. Abre el archivo Config.h y ajusta los siguientes parámetros para que coincidan con tu hardware específico:

VCC y R_REF: Mide con un multímetro el voltaje real de 5V de tu placa y el valor real de tu resistencia fija.

NTC_BETA: Revisa la hoja de datos de tu termistor para obtener el valor Beta correcto.

MOTOR_STEPS_PER_REV: Ajusta los pasos por revolución de tu motor (usualmente 200 o 400).

Ajusta los pines (*_PIN) si tu cableado es diferente al estándar de RAMPS 1.4.

Compilar y Subir:

Haz clic en el ícono Build (tilde/check) en la barra de tareas de PlatformIO para compilar el proyecto.

Si la compilación es exitosa, conecta tu Arduino Mega y haz clic en Upload (flecha) para cargar el firmware.

6. Guía de Uso
Pantalla Principal: Al encender, el sistema mostrará el menú principal.

Navegación: Gira el encoder rotativo para desplazarte arriba y abajo por las opciones del menú. La opción seleccionada se resaltará.

Selección: Presiona el botón del encoder para seleccionar una opción.

Si es una acción (ej: "Encender Motor"), esta se ejecutará.

Si es un submenú (ej: "Configuracion"), entrarás a una nueva lista de opciones.

Editar un Valor:

Navega hasta un valor editable (ej: "Velocidad: 60 RPM").

Presiona el botón una vez para entrar en modo edición. El valor aparecerá entre corchetes [60].

Gira el encoder para aumentar o disminuir el valor.

Presiona el botón de nuevo para confirmar y salir del modo edición.

Guardar Configuración: Para que la velocidad del motor se guarde permanentemente, navega a Configuracion -> Guardar Config y presiona el botón.

7. Futuras Mejoras
Implementar un menú para el ajuste de las ganancias PID en tiempo real.

Añadir perfiles de materiales (PET, PLA, etc.) con configuraciones preguardadas.

Integrar un sensor de diámetro de filamento con un bucle de control para ajustar la velocidad del motor automáticamente.

Implementar una parada de emergencia.