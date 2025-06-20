/*!
 *  @file RS485.h
 *
 *  @author khaiminh2310
 */

#ifndef __RS485_MODBUS_H__
#define __RS485_MODBUS_H__

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "pzem_define.h"

/* -------------- DEBUG (uncomment to open the Debug function) ------------- */

#if defined(ENABLE_RS485_DEBUG)
#define Debug Serial
#define DEBUG_PRINT(...) Debug.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Debug.println(__VA_ARGS__)
#define DEBUG_PRINTF(...) Debug.printf(__VA_ARGS__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINTF(...)
#endif

#define RS485Transmit                    HIGH
#define RS485Receive                     LOW
#define BAUDRATE_DEFAULT                 9600
#define ADDRESS_DEFAULT                  100

enum
{
  HARD_SERIAL,
  SOFT_SERIAL
};

typedef struct dataSHTC3
{
  float temperature;
  float humidity;
};

/* ------------------------------------------------------------------------- */

class RS485
{
private:
  Stream *port;
  uint8_t typeSerial;

  int8_t _dePin;
  int8_t _rePin;
  uint8_t _addr;
  uint16_t _timeOut = 2000;

  bool readRegister(uint8_t cmd, uint16_t reg, uint16_t *result, uint16_t len);
  bool writeRegister(uint8_t cmd, uint16_t reg, uint16_t *values, uint16_t len);

public:
  RS485(uint8_t rxPin, uint8_t txPin, int8_t dePin, int8_t rePin, uint8_t addr = ADDRESS_DEFAULT);
  virtual ~RS485() { delete port; }

  /* Initialization */
  void begin(uint32_t baud);

  bool readHoldingRegisters(uint16_t reg, uint16_t len, uint16_t *result);
  bool readInputRegisters(uint16_t reg, uint16_t len, uint16_t *result);
  bool writeSingleHoldingRegister(uint16_t reg, uint16_t *values, uint16_t len);
  bool writeMultipleHoldingRegisters(uint16_t reg, uint16_t *values, uint16_t len);

  int Running;
};



/* ------------------------------------------------------------------------- */

extern RS485 shtc3;
extern RS485 pzem;

#endif
