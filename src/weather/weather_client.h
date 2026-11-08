#pragma once

#include "weather/weather_data.h"

// Weather from MET Norway Locationforecast (no API key, User-Agent required).
// The HTTPS request runs in its own
// FreeRTOS task on core 0, so loop() and the clock never wait for the network.
namespace weather {

// Creates the worker task. Call once from setup().
void begin();

// Scheduling: fetch right after the network comes up, then every
// kWeatherUpdateMs; on failure retry with backoff. Call every loop().
void loop(bool networkUp);

// Thread-safe copy of the latest data and state.
void snapshot(Data& out);

// Fetch as soon as possible (serial command "weather").
void requestUpdate();

// Prints the current data to Serial.
void printSnapshot();

}  // namespace weather
