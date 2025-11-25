// FileName: TemperatureModule.h

#ifndef TEMPERATURE_MODULE_H
#define TEMPERATURE_MODULE_H

#include <PID_v1.h> // Se incluye la librería PID

// --- Estados de seguridad ---
enum TempError {
    TEMP_OK = 0,               // Estado normal, sin errores
    ERROR_HEATING_TIMEOUT,     // No se alcanzó la temperatura en el tiempo esperado
    ERROR_OVERHEAT,            // Temperatura excedió el límite de seguridad
    ERROR_RUNAWAY,             // Temperatura no cambia en mucho tiempo 
    ERROR_SENSOR_FAIL          // Lectura fuera de rango (sensor desconectado o dañado)
};

class TemperatureModule {
  private:
    // Variables que la librería PID necesita para operar
    double pidSetpoint; // El valor que queremos alcanzar (nuestro targetTemp)
    double pidInput;    // El valor que lee el sensor (la temperatura actual) --> Lógica de control
    double pidOutput;   // El valor que calcula el PID (salida para el PWM)

    double currentTemp; // última temperatura calculada y estable para la UI --> Monitoreo e interfaz

    // Variables para guardar las constantes del PID
    double kp, ki, kd;

    // Objeto de la librería PID que hará todo el trabajo
    PID myPID;

    // Seguridad

    unsigned long heatStartTime;       // Marca el tiempo en que comenzo el calentamiento
    double lastTemp;                   // Ultima temperatura registrada para detectar cambios
    unsigned long lastTempChangeTime;  // Marca el ultimo momento en que la temperatura cambio
    TempError errorState;              // Estado actual de error termico

  public:
    TemperatureModule();
    void init();
    void setTargetTemp(int sp);
    int getTargetTemp();
    double getTemp(); // Se usa 'double' para mayor precisión
    void update();
    double getKp() { return kp; }
    double getKi() { return ki; }
    double getKd() { return kd; }
    double getCurrentTemp() const { return currentTemp; }
    // Función para poder ajustar el PID desde el menú en el futuro
    void setTunings(double Kp, double Ki, double Kd);
    void saveSettingsToEEPROM(int targetToSave);
    void loadSettingsFromEEPROM();

    // Funciones seguridad
    void safetyCheck();
    TempError getError(); // Devuelve el estado actual de error
    void resetError(); // Reintentar Calentamiento
};

#endif