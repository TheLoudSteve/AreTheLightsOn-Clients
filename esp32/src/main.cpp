#include <Adafruit_NeoPixel.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/base64.h>
#include <time.h>

#ifndef ATLO_VERSION
#define ATLO_VERSION "dev"
#endif

constexpr int LED_PIN = 27;
constexpr int LED_COUNT = 1;
constexpr int BUTTON_PIN = 39;
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 60000;
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Preferences prefs;
unsigned long startedAt;
unsigned long lastHeartbeat;
bool wifiConnectedAnnounced;

const char AMAZON_ROOT_CA_1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

void status(uint32_t color) { led.setPixelColor(0, color); led.show(); }
void secureClient(WiFiClientSecure& client) { client.setCACert(AMAZON_ROOT_CA_1); }
bool ensureTime() { if (time(nullptr) > 1700000000) return true; configTime(0, 0, "time.aws.com", "pool.ntp.org"); for (int attempt = 0; attempt < 20 && time(nullptr) <= 1700000000; attempt++) delay(250); const bool ready = time(nullptr) > 1700000000; if (!ready) Serial.println("ATLO_CLOCK_NOT_SET"); return ready; }

String decode64(const String& text) {
  size_t outputLength = 0;
  const int sizeResult = mbedtls_base64_decode(nullptr, 0, &outputLength, reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
  if (sizeResult != 0 && sizeResult != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) return "";
  if (outputLength == 0) return "";
  auto* output = new unsigned char[outputLength + 1];
  if (mbedtls_base64_decode(output, outputLength, &outputLength, reinterpret_cast<const unsigned char*>(text.c_str()), text.length()) != 0) { delete[] output; return ""; }
  output[outputLength] = 0; String result(reinterpret_cast<char*>(output)); delete[] output; return result;
}

String jsonValue(const String& json, const String& key) {
  const String marker = "\"" + key + "\":\""; const int start = json.indexOf(marker);
  if (start < 0) return ""; const int valueStart = start + marker.length(); const int end = json.indexOf('"', valueStart);
  return end < 0 ? "" : json.substring(valueStart, end);
}

bool claimDevice() {
  const String claimUrl = prefs.getString("claim_url"); const String deviceId = prefs.getString("device_id"); const String claim = prefs.getString("claim");
  if (claimUrl.isEmpty() || deviceId.isEmpty() || claim.isEmpty()) { Serial.println("ATLO_CLAIM_ERROR missing_setup"); return false; }
  if (!ensureTime()) return false; WiFiClientSecure client; secureClient(client);
  HTTPClient http; if (!http.begin(client, claimUrl)) { Serial.println("ATLO_CLAIM_ERROR begin"); return false; }
  http.addHeader("Content-Type", "application/json"); const int code = http.POST("{\"device_id\":\"" + deviceId + "\",\"claim_code\":\"" + claim + "\"}");
  const String response = http.getString(); http.end(); Serial.printf("ATLO_CLAIM_HTTP %d\n", code); if (code != 200) return false;
  const String secret = jsonValue(response, "secret"); const String heartbeatUrl = jsonValue(response, "heartbeat_url");
  if (secret.isEmpty() || heartbeatUrl.isEmpty()) { Serial.println("ATLO_CLAIM_ERROR response"); return false; }
  prefs.putString("secret", secret); prefs.putString("heartbeat_url", heartbeatUrl); prefs.remove("claim"); prefs.remove("claim_url"); Serial.println("ATLO_CLAIMED"); return true;
}

bool heartbeat() {
  const String url = prefs.getString("heartbeat_url"); const String secret = prefs.getString("secret"); const String deviceId = prefs.getString("device_id");
  if (url.isEmpty() || secret.isEmpty() || deviceId.isEmpty()) return false;
  if (!ensureTime()) return false; WiFiClientSecure client; secureClient(client); HTTPClient http; if (!http.begin(client, url)) { Serial.println("ATLO_HEARTBEAT_ERROR begin"); return false; }
  http.addHeader("Authorization", "Bearer " + secret); http.addHeader("Content-Type", "application/json");
  const String body = "{\"device_id\":\"" + deviceId + "\",\"uptime_seconds\":" + String((millis() - startedAt) / 1000) + ",\"wifi_rssi\":" + String(WiFi.RSSI()) + ",\"firmware_version\":\"esp32/atom-lite-" ATLO_VERSION "\"}";
  const int code = http.POST(body); http.end(); Serial.printf("ATLO_HEARTBEAT_HTTP %d\n", code); return code >= 200 && code < 300;
}

void configureFromSerial() {
  if (!Serial.available()) return;
  const String line = Serial.readStringUntil('\n'); if (!line.startsWith("ATLO_CONFIG|")) return;
  String fields[5]; int position = 12;
  for (int index = 0; index < 5; index++) { const int separator = line.indexOf('|', position); fields[index] = line.substring(position, separator < 0 ? line.length() : separator); position = separator < 0 ? line.length() : separator + 1; }
  const String claimUrl = decode64(fields[0]); const String deviceId = decode64(fields[1]); const String claim = decode64(fields[2]); const String ssid = decode64(fields[3]); const String password = decode64(fields[4]);
  if (claimUrl.isEmpty() || deviceId.isEmpty() || claim.isEmpty() || ssid.isEmpty()) { Serial.println("ATLO_ERROR invalid configuration"); status(led.Color(255, 0, 0)); return; }
  prefs.putString("claim_url", claimUrl); prefs.putString("device_id", deviceId); prefs.putString("claim", claim); prefs.putString("ssid", ssid); prefs.putString("password", password); Serial.println("ATLO_CONFIGURED"); delay(100); ESP.restart();
}

void setup() {
  led.begin(); status(led.Color(0, 0, 255)); pinMode(BUTTON_PIN, INPUT); Serial.begin(115200); Serial.setTimeout(1000); prefs.begin("atlo", false); startedAt = millis(); Serial.println("ATLO_BOOT");
  const String ssid = prefs.getString("ssid"); if (ssid.isEmpty()) { Serial.println("ATLO_READY"); return; }
  status(led.Color(255, 160, 0)); Serial.println("ATLO_WIFI_CONNECTING"); WiFi.mode(WIFI_STA); WiFi.begin(ssid.c_str(), prefs.getString("password").c_str());
}

void loop() {
  configureFromSerial();
  if (prefs.getString("ssid").isEmpty()) return;
  if (WiFi.status() != WL_CONNECTED) { status(led.Color(255, 160, 0)); return; }
  if (!wifiConnectedAnnounced) { wifiConnectedAnnounced = true; Serial.println("ATLO_WIFI_CONNECTED"); }
  if (prefs.getString("secret").isEmpty() && !claimDevice()) { status(led.Color(255, 0, 0)); delay(5000); return; }
  if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS || lastHeartbeat == 0) { lastHeartbeat = millis(); if (heartbeat()) status(led.Color(0, 255, 0)); else status(led.Color(255, 0, 0)); }
}
