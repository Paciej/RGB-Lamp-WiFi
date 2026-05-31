# Smart RGB LED Lamp — WiFi + MQTT (ESP32)

A WiFi-connected RGB LED lamp prototype built on the ESP32 microcontroller.
The firmware is being developed to support MQTT-based control for real-time
color, brightness, lighting modes, and temperature monitoring.
The project is currently under active development and BLE support will be removed.

---

## PCB Design (Eagle)

> Schematic and board layout designed in Eagle CAD.
<p align="center">
<img src="https://github.com/Paciej/RGB-Lamp/blob/main/hardware/Pictures/RGBLampSchematic.png" alt="Eagle Schematic" width="500">
</p>
<p align="center">
<img src="https://github.com/Paciej/RGB-Lamp/blob/main/hardware/Pictures/RGBLampBoard.png" alt="Eagle PCB Layout" width="500">
</p>
---

## Printed & Assembled PCB

> Physical PCB after printing and soldering.
<p align="center">
<img src="https://github.com/Paciej/RGB-Lamp/blob/main/hardware/Pictures/BoardSoldered.jpg" alt="Soldered PCB" width="200" hspace="50"> <img src="https://github.com/Paciej/RGB-Lamp/blob/main/hardware/Pictures/EmptyBoard.jpg" alt="Printed PCB" width="200" hspace="10">
</p>
---

## Control via WiFi + MQTT

> The lamp will be controlled using MQTT messages over WiFi.
> Temperature, RGB color, brightness, and lighting mode data are exposed via MQTT topics.
---

##  Lighting Modes

### Random Color — Physical Button
> Press the physical button to apply a random color using ESP32's hardware RNG.

<p align="center">
  <img src="https://github.com/user-attachments/assets/1ca8d6ae-3017-4212-8292-e95a26128fe6" alt="Random Color Button Demo" width="300">
</p>

---

### Candle Mode
> Simulates a realistic flickering candle effect.

<p align="center">
  <img src="https://github.com/user-attachments/assets/360fdc14-0a77-4107-a589-a153b81df355" alt="Candle Mode Demo" width="300">
</p>

---

### Pulse Mode
> Smooth brightness breathing effect.

<p align="center">
  <img src="https://github.com/user-attachments/assets/5d3f25d7-855c-486d-9b48-8a42fe558902" alt="Pulse Mode Demo" width="300">
</p>

---

### Wave Mode
> Seamless smooth transitions across the full RGB spectrum.

<p align="center">
  <img src="https://github.com/user-attachments/assets/1e8b393f-a74e-49be-bfe6-844565e4a7bd" alt="Wave Mode Demo" width="300">
</p>

---

##  Features

-  **Full RGB color control** — set any color via MQTT payloads (values 0–255 per channel)
-  **Brightness control** via MOSFET PWM (low-side configuration)
-  **Temperature monitoring** — DS18B20 sensor data published over MQTT
-  **Random color button** — applies a random color using ESP32's hardware RNG
-  **Lighting modes:**
  - Candle (flicker simulation)
  - Pulse (breathing effect)
  - Wave (smooth RGB transitions)
-  **WiFi + MQTT communication** — planned MQTT-based control interface

---

##  Pin Configuration (ESP32 DevKit)

| Pin | Function | Component |
|-----|----------|-----------|
| 23 | Red channel (PWM) | RGB LED — Red |
| 24 | Green channel (PWM) | RGB LED — Green |
| 25 | Blue channel (PWM) | RGB LED — Blue |
| 9 | 1-Wire data | DS18B20 Temperature Sensor |
| 11 | Button input | Random color trigger |

---

##  Hardware Components

| Component | Description |
|-----------|-------------|
| **ESP32 DevKit** | Main microcontroller with WiFi and MQTT support |
| **RGB LED 3W (Star)** | High-brightness point light source |
| **MOSFET (×3)** | Low-side PWM drivers for each RGB channel |
| **LM2596 Buck Converter Module** | Steps down 12V supply to 3.3V for ESP32 |
| **DS18B20** | Digital 1-Wire temperature sensor |
| **Resistor 33Ω 5W** | Custom current-limiting resistor |
| **Push Button** | Physical control for random color |

>  The repository includes pre-built external libraries for ESP32 DevKit, DS18B20,
> and the custom 33Ω 5W resistor component (Eagle library).

---

##  Libraries

The project uses the following external libraries bundled in the repository:

- `WiFi` — ESP32 WiFi connectivity
- `PubSubClient` or equivalent MQTT client library — MQTT messaging support
- `DallasTemperature` + `OneWire` — DS18B20 sensor support
- Custom Eagle component library — 33Ω 5W resistor footprint

---

## Setup

1. Open the project in **PlatformIO**
2. Select board: `ESP32 Dev Module`
3. Install libraries
4. Configure WiFi SSID, password, and MQTT broker settings in the firmware
5. Flash the firmware to your ESP32
6. Connect the lamp to your WiFi network
7. Control RGB color, lighting mode, and temperature monitoring via MQTT topics

---

##  MQTT Control

The lamp is intended to be controlled using MQTT topics over the local WiFi network:

- Publish RGB payloads to set the lamp color
- Publish mode commands to switch lighting modes
- Subscribe to temperature updates from the lamp
- Subscribe to status topics for connection and state information

---

##  Project Info

- **University:** Silesian University of Technology
- **Faculty:** Faculty of Automatic Control, Electronics and Computer Science
- **Subject:** Microprocessor and embedded systems
- **Author:** Maciej Porębski
