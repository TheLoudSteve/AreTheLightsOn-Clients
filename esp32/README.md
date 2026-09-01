# ESP32 client — M5Stack Atom Lite

The first ESP32 target is the M5Stack Atom Lite (ESP32-PICO-D4, 4 MB flash, 2.4 GHz Wi-Fi).

The browser installer flashes generic firmware and passes a short-lived setup claim over USB serial. The device redeems that claim once over HTTPS for its permanent heartbeat credential; no permanent secret is in the firmware or installer URL.

LED states: blue = ready for setup, yellow = joining Wi-Fi, green = heartbeat accepted, red = configuration or connection error.

## Build locally

```sh
pip install platformio
pio run -d esp32
```

The installer requires Chrome or Edge over HTTPS. It only supports 2.4 GHz Wi-Fi because that is what the Atom Lite radio supports.
