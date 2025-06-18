#include "pzem004t.h"

byte getValue_para[8] = {0xf8, 0x04, 0x00, 0x00, 0x00, 0x0a, 0x64, 0x64};
byte resetEnergy_para[4] = {0xf8, 0x42, 0xc2, 0x41};

 Pzem004t::Pzem004t() {
  Serial.begin(9600);
  port = &Serial;
  typeSerial = HARD_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t::Pzem004t(HardwareSerial * serial) {
//  serial->begin(9600);
  port = serial;
  typeSerial = HARD_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t::Pzem004t(SoftwareSerial * serial) {
//  serial->begin(9600);
  port = serial;
  typeSerial = SOFT_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t::Pzem004t(int rxPin, int txPin) {
  SoftwareSerial * yy = new SoftwareSerial(rxPin, txPin);
//  yy->begin(9600);
  port = yy;
  typeSerial = SOFT_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

void Pzem004t::begin(unsigned long _tembaud) {
  if (typeSerial == HARD_SERIAL) {
    HardwareSerial * tt = (HardwareSerial *)port;
    tt->begin(_tembaud);
  } else {
    SoftwareSerial * tt = (SoftwareSerial *) port;
    tt->begin(_tembaud);
  }
}

void Pzem004t::begin() {
  this->begin(9600);
}

void Pzem004t::setTimeout(unsigned int _ui_timeOut) {
  if (_ui_timeOut < MIN_PZEM_TIMEOUT) {
    ui_timeOut = MIN_PZEM_TIMEOUT;
  } else if (_ui_timeOut > MAX_PZEM_TIMEOUT) {
    ui_timeOut = MAX_PZEM_TIMEOUT;
  } else
    ui_timeOut = _ui_timeOut;
}

pzem_info Pzem004t::getData(){
  pzem_info tem_pzem;
  while (port->available()) {
    port->read();
  }
  DB("port->write");
  port->write(getValue_para, sizeof(getValue_para));

  unsigned long temTime = millis();
  bool b_complete = false;
  uint8_t myBuf[RESPONSE_SIZE];

  while ((millis() - temTime) < ui_timeOut) {
    if (port->available()) {
      port->readBytes(myBuf, RESPONSE_SIZE);
      b_complete = true;
      DB_LN("port->available");
      yield();
      break;
    }
  }

  if (b_complete) {
    tem_pzem.volt = PZEM_GET_VALUE(voltage,SCALE_V);
    tem_pzem.ampe = PZEM_GET_VALUE2(ampe, SCALE_A);
    tem_pzem.power = PZEM_GET_VALUE2(power, SCALE_P);
    tem_pzem.energy = PZEM_GET_VALUE2(energy, SCALE_E);
    tem_pzem.freq = PZEM_GET_VALUE(freq, SCALE_H);
    tem_pzem.powerFactor = PZEM_GET_VALUE(powerFactor, SCALE_PF);

    DB(pzemData.address, HEX);    DB(F(" - "));
    DB(pzemData.byteSuccess, HEX);    DB(F(" - "));
    DB(pzemData.numberOfByte, HEX);    DB(F(" -- "));

    DB(pzemData.voltage_int, HEX);    DB(F("V - "));
    DB(pzemData.ampe_int, HEX);    DB(F("A - "));
    DB(pzemData.power_int, HEX);    DB(F("W - "));
    DB(pzemData.energy_int, HEX);    DB(F("Wh - "));
    DB(pzemData.freq_int, HEX);    DB(F("Hz - "));
    DB_LN(pzemData.powerFactor_int, HEX);    DB(F(" - "));

  } else {
    DB_LN(F("Read fail"));
    tem_pzem.volt = 0.0;
    tem_pzem.ampe = 0.0;
    tem_pzem.power = 0.0;
    tem_pzem.energy = 0.0;
    tem_pzem.freq = 0.0;
    tem_pzem.powerFactor = 0.0;
  }
  return tem_pzem;
}

bool Pzem004t::resetEnergy() {
  bool status = false;
  while (port->available()) {
    port->read();
  }
  port->write(resetEnergy_para, sizeof(resetEnergy_para));

  unsigned long temTime = millis(), temSubtime = 0;;
  bool b_complete = true;

  byte testRespone[4];

  while (temSubtime < ui_timeOut) {
    temSubtime = millis() - temTime;
    if (port->available()) {
      port->readBytes(testRespone, sizeof(testRespone));
      for(int cf = 0; cf < 4; cf++){
        if(testRespone[cf] != resetEnergy_para[cf])  b_complete = false;
      }
      DB(F("port->available"));
      yield();
      break;
    }
  }
  if(temSubtime >= ui_timeOut) b_complete = false;

  if (b_complete) {
    if(testRespone[3] == resetEnergy_para[3]){
        status = true;
        DB_LN(F("resetEnergy success!"));
    }else{
        DB_LN(F("resetEnergy invalid"));
    }

  } else {
    DB_LN(F("read resetEnergy fail"));
    
  }
  return status;
}