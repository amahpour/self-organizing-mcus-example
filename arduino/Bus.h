#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Proto.h"

class Bus {
 public:
  Bus(uint8_t rxPin, uint8_t txPin) : serial(rxPin, txPin, true) {}

  void begin(unsigned long baud) {
    serial.begin(baud);
  }

  bool send(const Frame& f) {
    Frame c = f;
    proto_finalize(c);
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&c);
    size_t len = 5 + c.payload_len + 1; // sof..payload..checksum
    return serial.write(raw, len) == (int)len;
  }

  bool recv(Frame& out, uint16_t timeout_ms) {
    unsigned long start = millis();
    // Find SOF
    while ((millis() - start) < timeout_ms) {
      if (serial.available()) {
        uint8_t b = (uint8_t)serial.read();
        if (b == SOF) {
          out.sof = b;
          // read fixed header
          if (!readByte(out.type, timeout_ms)) return false;
          if (!readByte(out.source, timeout_ms)) return false;
          if (!readByte(out.payload_len, timeout_ms)) return false;
          if (out.payload_len > MAX_PAYLOAD_SIZE) return false;
          for (uint8_t i = 0; i < out.payload_len; ++i) {
            if (!readByte(out.payload[i], timeout_ms)) return false;
          }
          if (!readByte(out.checksum, timeout_ms)) return false;
          return proto_is_valid(out);
        }
      }
      yield();
    }
    return false;
  }

 private:
  SoftwareSerial serial;

  bool readByte(uint8_t& b, uint16_t timeout_ms) {
    unsigned long start = millis();
    while ((millis() - start) < timeout_ms) {
      if (serial.available()) { b = (uint8_t)serial.read(); return true; }
      yield();
    }
    return false;
  }
};
