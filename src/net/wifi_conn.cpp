#include "net/wifi_conn.h"

#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"

namespace net {

namespace {

WiFiManager wm;
State current = State::Connecting;
uint32_t stateSinceMs = 0;
uint32_t lastRetryMs = 0;

void enter(State s) {
  if (s == current) return;
  Serial.printf("[wifi] %s -> %s\n", stateName(current), stateName(s));
  current = s;
  stateSinceMs = millis();
}

void startPortal() {
  Serial.printf("[wifi] starting setup portal: AP \"%s\", password \"%s\"\n", config::kApName,
                config::kApPassword);
  wm.setConfigPortalTimeout(config::kPortalTimeoutS);
  wm.startConfigPortal(config::kApName, config::kApPassword);
  lastRetryMs = millis();
  enter(State::Portal);
}

void retryConnect() {
  lastRetryMs = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin();  // saved credentials
}

}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config::kHostname);
  WiFi.setAutoReconnect(true);

  wm.setDebugOutput(false);
  wm.setConfigPortalBlocking(false);
  wm.setHostname(config::kHostname);

  stateSinceMs = millis();
  if (!wm.getWiFiIsSaved()) {
    Serial.println("[wifi] no saved network");
    startPortal();
    return;
  }
  Serial.printf("[wifi] connecting to saved network \"%s\"\n", wm.getWiFiSSID(true).c_str());
  retryConnect();
}

void loop() {
  const bool linkUp = WiFi.status() == WL_CONNECTED;

  switch (current) {
    case State::Connecting:
      if (linkUp) {
        enter(State::Connected);
      } else if (millis() - stateSinceMs > config::kWifiConnectTimeoutMs) {
        Serial.println("[wifi] connect timeout");
        startPortal();
      }
      break;

    case State::Portal:
      wm.process();
      if (linkUp) {
        if (wm.getConfigPortalActive()) wm.stopConfigPortal();
        WiFi.mode(WIFI_STA);
        enter(State::Connected);
      } else if (!wm.getConfigPortalActive()) {
        Serial.println("[wifi] portal closed, will keep retrying");
        retryConnect();
        enter(State::Offline);
      } else if (wm.getWiFiIsSaved() && WiFi.softAPgetStationNum() == 0 &&
                 millis() - lastRetryMs > config::kWifiRetryIntervalMs) {
        // The saved network may have come back (e.g. router booted slower than
        // the clock). A connect attempt makes the radio scan and briefly
        // disturbs the access point, so only try while nobody uses the portal.
        lastRetryMs = millis();
        WiFi.begin();
      }
      break;

    case State::Connected:
      if (!linkUp) {
        lastRetryMs = millis();  // auto-reconnect is already trying
        enter(State::Offline);
      }
      break;

    case State::Offline:
      if (linkUp) {
        enter(State::Connected);
      } else if (millis() - lastRetryMs > config::kWifiRetryIntervalMs) {
        retryConnect();
      }
      break;
  }
}

State state() { return current; }

const char* stateName(State s) {
  switch (s) {
    case State::Connecting: return "connecting";
    case State::Connected: return "connected";
    case State::Portal: return "portal";
    case State::Offline: return "offline";
  }
  return "?";
}

bool isConnected() { return current == State::Connected; }

IPAddress localIp() { return WiFi.localIP(); }

int rssi() { return WiFi.RSSI(); }

uint32_t portalSecondsLeft() {
  if (current != State::Portal) return 0;
  const uint32_t elapsed = (millis() - stateSinceMs) / 1000;
  return elapsed >= config::kPortalTimeoutS ? 0 : config::kPortalTimeoutS - elapsed;
}

void resetCredentials() {
  Serial.println("[wifi] forgetting saved network");
  wm.resetSettings();
}

}  // namespace net
