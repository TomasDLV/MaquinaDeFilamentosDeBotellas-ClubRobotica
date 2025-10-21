// FileName: TemperatureModule.h

#ifndef TEMPERATURE_MODULE_H
#define TEMPERATURE_MODULE_H

#include <PID_v1.h> // Se incluye la librería PID

class TemperatureModule {
  private:
    // Variables que la librería PID necesita para operar
    double pidSetpoint; // El valor que queremos alcanzar (nuestro targetTemp)
    double pidInput;    // El valor que lee el sensor (la temperatura actual)
    double pidOutput;   // El valor que calcula el PID (salida para el PWM)
    
    // Variables para guardar las constantes del PID
    double kp, ki, kd;

    // Objeto de la librería PID que hará todo el trabajo
    PID myPID;

  public:
    TemperatureModule();
    void init();
    void setTargetTemp(int sp);
    int getTargetTemp();
    double getTemp(); // Se usa 'double' para mayor precisión
    void update();
    // Función para poder ajustar el PID desde el menú en el futuro
    void setTunings(double Kp, double Ki, double Kd);
};

#endif