// ORIGINAL https://github.com/TakehikoShimojima/Arduino-Current-Logger

#ifndef INA266_H
#define INA266_H

#include <Wire.h>

class INA226
{
public:
    INA226(void);

    virtual ~INA226();
  
    void begin(int sda, int sck);

    int readId(void);

    float readCurrent(void);
  
    float readVoltage(void);

    short readCurrentReg(void);
  
    short readVoltageReg(void);

private:
};

#endif // INA226_H

