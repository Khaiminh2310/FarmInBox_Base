/*!
 *  @file RS485.cpp
 *
 *  @author khaiminh2310
 */

#include "RS485.h"
#include "ModbusCRC.h"
#include <common.h>

/* ------------------------------ Constructor ------------------------------ */

RS485::RS485(uint8_t rxPin, uint8_t txPin, int8_t dePin, int8_t rePin, uint8_t addr)
{
  SoftwareSerial *ss = new SoftwareSerial(rxPin, txPin);
  port = ss;
  typeSerial = SOFT_SERIAL;

  _dePin = dePin;
  _rePin = rePin;
  _addr = addr;
  if (_dePin != -1) pinMode(_dePin, OUTPUT);
  if (_rePin != -1) pinMode(_rePin, OUTPUT);
}

/* ----------------------------- Initialization ---------------------------- */
void RS485::begin(uint32_t baud)
{
  if (typeSerial == HARD_SERIAL)
  {
    HardwareSerial *hs = (HardwareSerial *)port;
    hs->begin(baud);
  }
  else if (typeSerial == SOFT_SERIAL)
  {
    SoftwareSerial *ss = (SoftwareSerial *)port;
    ss->begin(baud);
  }
}

/* --------------------------------- Common -------------------------------- */

bool RS485::readHoldingRegisters(uint16_t reg, uint16_t len, uint16_t *result)
{
  return readRegister(0x03, reg, result, len);
}

bool RS485::readInputRegisters(uint16_t reg, uint16_t len, uint16_t *result)
{
  return readRegister(0x04, reg, result, len);
}

bool RS485::writeSingleHoldingRegister(uint16_t reg, uint16_t *values, uint16_t len)
{
  return writeRegister(0x06, reg, values, 1);
}

bool RS485::writeMultipleHoldingRegisters(uint16_t reg, uint16_t *values, uint16_t len)
{
  return writeRegister(0x10, reg, values, len);
}

/* --------------------------------- Struct -------------------------------- */

bool RS485::readRegister(uint8_t cmd, uint16_t reg, uint16_t *result, uint16_t len=1)
{
  if (!port || len == 0 || len > 125) return false;

  /* Remove all previous junk data (if have) */
  while (port->available()) {
    port->read();
  }

  if (_dePin != -1) digitalWrite(_dePin, RS485Transmit);
  if (_rePin != -1) digitalWrite(_rePin, RS485Transmit);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  uint8_t frame[8];
  frame[0] = _addr;
  frame[1] = cmd;
  frame[2] = reg >> 8;
  frame[3] = reg & 0xFF;
  frame[4] = len >> 8;
  frame[5] = len & 0xFF;
  uint16_t crc = checkModbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;
  port->write(frame, 8);
#if defined(ENABLE_RS485_DEBUG)
  DEBUG_PRINT("SEND: ");
  for (int i = 0; i < 8; i++) {
    DEBUG_PRINTF("%02X ", frame[i]);
  }
  DEBUG_PRINTLN("");
#endif

  vTaskDelay(50 / portTICK_PERIOD_MS);

  /* Number of bytes to respond
  **
  ** Addr : 1 Byte
  ** Cmd  : 1 Byte (0x03)
  ** Resp : 1 Byte (0x02)
  ** Val  : 2 Byte
  ** Crc  : 2 Byte [L_CRC]  [H_CRC]
  */
  if (_dePin != -1 || _rePin != -1) {
    if (_dePin != -1) digitalWrite(_dePin, RS485Receive);
    if (_rePin != -1) digitalWrite(_rePin, RS485Receive);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  int expectedLen = 5 + len * 2;
  uint8_t myBuf[256];
  int index = 0;
  unsigned long start = millis();

  while ((millis() - start) < _timeOut && index < expectedLen) {
    if (port->available()) {
      myBuf[index++] = port->read();
    }
  }

  if (index == expectedLen) {
#if defined(ENABLE_RS485_DEBUG)
    DEBUG_PRINT("RECEIVED: ");
    for (int i = 0; i < index; i++) {
      DEBUG_PRINTF("%02X ", myBuf[i]);
    }
    DEBUG_PRINTLN("");
#endif
    /* Check CRC */
    if (checkModbusCRC(myBuf, expectedLen) == 0) {
      for (int i = 0; i < len; i++) {
        result[i] = (myBuf[3 + i * 2] << 8) | myBuf[4 + i * 2];
      }
      return true;
    } else {
      DEBUG_PRINTLN(F("Error CRC when readRegister"));
    }
  } else {
    DEBUG_PRINTLN(F("Timeout when readRegister"));
  }
  return false;
}

bool RS485::writeRegister(uint8_t cmd, uint16_t reg, uint16_t *values, uint16_t len)
{
  if (!port) return false;

  /* Remove all previous junk data (if have) */
  while (port->available()) port->read();

  if (_dePin != -1) digitalWrite(_dePin, RS485Transmit);
  if (_rePin != -1) digitalWrite(_rePin, RS485Transmit);
  vTaskDelay(2 / portTICK_PERIOD_MS);

  uint8_t frame[256];
  int frameLen = 0;

  if (cmd == 0x06 && len == 1) {
    // Write Single Holding Register
    frame[0] = _addr;
    frame[1] = 0x06;
    frame[2] = reg >> 8;
    frame[3] = reg & 0xFF;
    frame[4] = values[0] >> 8;
    frame[5] = values[0] & 0xFF;

    uint16_t crc = checkModbusCRC(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    frameLen = 8;
  }
  else if (cmd == 0x10 && len >= 1 && len <= 123) {
    // Write Multiple Holding Registers
    uint8_t byteCount = len * 2;
    frame[0] = _addr;
    frame[1] = 0x10;
    frame[2] = reg >> 8;
    frame[3] = reg & 0xFF;
    frame[4] = len >> 8;
    frame[5] = len & 0xFF;
    frame[6] = byteCount;

    for (int i = 0; i < len; i++) {
      frame[7 + i * 2] = values[i] >> 8;
      frame[8 + i * 2] = values[i] & 0xFF;
    }

    uint16_t crc = checkModbusCRC(frame, 7 + byteCount);
    frame[7 + byteCount] = crc & 0xFF;
    frame[8 + byteCount] = crc >> 8;

    frameLen = 9 + byteCount;
  }
  else {
    DEBUG_PRINTLN("Unsupported command or invalid length");
    return false;
  }

  port->write(frame, frameLen);

#if defined(ENABLE_RS485_DEBUG)
  DEBUG_PRINT("SEND: ");
  for (int i = 0; i < frameLen; i++) {
    DEBUG_PRINTF("%02X ", frame[i]);
  }
  DEBUG_PRINTLN("");
#endif

  vTaskDelay(50 / portTICK_PERIOD_MS);

  if (_dePin != -1) digitalWrite(_dePin, RS485Receive);
  if (_rePin != -1) digitalWrite(_rePin, RS485Receive);
  vTaskDelay(2 / portTICK_PERIOD_MS);

  uint8_t response[8];
  int index = 0;
  unsigned long start = millis();

  while ((millis() - start) < _timeOut && index < sizeof(response)) {
    if (port->available()) {
      response[index++] = port->read();
    }
  }

  if (index == 8) {
#if defined(ENABLE_RS485_DEBUG)
    DEBUG_PRINT("RECEIVED: ");
    for (int i = 0; i < 8; i++) {
      DEBUG_PRINTF("%02X ", response[i]);
    }
    DEBUG_PRINTLN("");
#endif
    if (checkModbusCRC(response, 8) == 0) {
      return true;
    } else {
      DEBUG_PRINTLN(F("CRC error in writeRegister"));
    }
  } else {
    DEBUG_PRINTLN(F("Timeout in writeRegister"));
  }

  return false;
}


RS485 shtc3(MODBUS_RX_PIN, MODBUS_TX_PIN, MAX485_DE, MAX485_RE, 0x01);
RS485 pzem(MODBUS_RX_PIN, MODBUS_TX_PIN, MAX485_DE, MAX485_RE, 0xF8);