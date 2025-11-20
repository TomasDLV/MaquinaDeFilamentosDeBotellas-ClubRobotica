// FileName: ExtrusionModule.h

#ifndef EXTRUSION_MODULE_H
#define EXTRUSION_MODULE_H

#include <AccelStepper.h>

class ExtrusionModule {
  private:
    AccelStepper stepper;
    float currentSpeed; // Velocidad actual en pasos/segundo
    

  public:
    ExtrusionModule();
    void init();
    void setSpeed(float speed);
    float getSpeed();
    void start();
    void stop();
    void update();

    // Funciones específicas para la EEPROM
    void saveSpeedToEEPROM();
    void loadSpeedFromEEPROM();
};

#endif