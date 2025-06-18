#ifndef _H_PZEM004T_V2_H__
#define _H_PZEM004T_V2_H__

#include "pzem004t_define.h"
#include "SoftwareSerial.h"

enum {
  HARD_SERIAL,
  SOFT_SERIAL
};

#define writeToSerial(...)      { port->write(__VA_ARGS__); }

//#define EN_DEBUG_PZEM004TV2

#if defined(EN_DEBUG_PZEM004TV2)
  #define debug                   Serial
  #define DB(...)                 debug.print(__VA_ARGS__);
  #define DB_LN(...)              debug.println(__VA_ARGS__);
#else
  #define debug                   Serial
  #define DB(...)                 
  #define DB_LN(...)              
#endif

extern byte getValue_para[8];
extern byte resetEnergy_para[4];

class Pzem004t{
  private:
    uint8_t typeSerial;
    Stream * port;
    unsigned int ui_timeOut;
    void begin(unsigned long _tembaud);
    
  public:
    Pzem004t();

    virtual ~Pzem004t(){delete port;}

    Pzem004t(HardwareSerial * serial);

    Pzem004t(SoftwareSerial * serial);

    Pzem004t(int rxPin, int txPin);
    
    void begin();

    void setTimeout(unsigned int _ui_timeOut);

    pzem_info getData();

    bool resetEnergy(); // return true when success
};

#endif