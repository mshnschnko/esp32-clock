#pragma once

#include <stddef.h>
#include <stdint.h>

// User settings, editable on the web page and stored in NVS. Missing values
// fall back to the defaults in config.h. Accessed from loop() (core 1) only;
// the weather task gets its own copy of the coordinates.
// (The clock face is owned by ui: see ui::setFace.)
namespace settings {

struct Values {
  char tz[48];     // POSIX TZ string, e.g. "MSK-3"
  char place[32];  // weather location name
  float lat;
  float lon;
  uint16_t autoReturnS;  // seconds away from the clock before returning to it
};

void begin();

const Values& get();

// Validates and stores. On error returns false and writes a message to err.
bool save(const Values& v, char* err, size_t errSize);

void print();

}  // namespace settings
