# LCC ESP32 2 Servo & 8 I/O Basic Controller

An OpenLCB / LCC (Layout Command Control) firmware implementation optimized for model railway automation. Developed for the **ESP32 DevKit 1** combined with the **SN65HVD230** transceiver module, this project uses the OpenLCB Single Thread Library to control two turnout servos and 8 multi-configurable Input/Output pins.

---

## ⚠️ Disclaimer and Limitation of Liability

This software has been developed specifically for the **ESP32 Devkit 1** and the **SN65HVD230** CAN transceiver module. It has only been tested on the author’s personal model railway layout.

**The software is provided “AS IS” and “AS AVAILABLE”**, without any warranties or guarantees of any kind. The author explicitly disclaims all warranties, whether express, implied, or statutory, including but not limited to any warranties of merchantability, fitness for a particular purpose, accuracy, reliability, or non-infringement.

The author accepts **no responsibility or liability** for:
- Any malfunction, failure, or unexpected behaviour of the sketch.
- Damage to hardware, loss of data, or disruption to your model railway layout.
- Incompatibility caused by updates to third-party libraries, Arduino core, JMRI, or other software.
- Any direct, indirect, incidental, consequential, or punitive damages arising from the use or inability to use this sketch.

This code is offered strictly for **educational and hobbyist purposes** to help railway modellers learn how to use the OpenLCB Single Thread Library. It is not intended for commercial use, safety-critical applications, or any situation where failure could cause damage or injury.

By downloading, using, or modifying this sketch, you acknowledge that you assume **all risk** and full responsibility for any outcomes resulting from its use.

---

## 🚀 Features

* **LCC/OpenLCB Integrated:** Fully configurable via Configuration Description Information (CDI) using software like JMRI.
* **Dual Turnout Servo Support:** Precise control of 2 servos (Pins 32 & 33) with smooth movement driven via a background FreeRTOS execution thread on Core 0.
* **Dynamic Servo Speeds:** Adjustable travel speed (5–50 steps/delay) alongside a configurable auto-save routine to protect EEPROM health.
* **8 Multi-Function I/O Pins:** Pins 14, 27, 26, 25, 16, 17, 18, and 19 can be individually mapped to 11 different modes (Inputs, Outputs, Toggles, Pull-ups, Phase outputs, and inverted alternatives).
* **Auto-Detach System:** Servos automatically detach after 4 seconds of inactivity to eliminate buzzing and reduce power consumption.

---

## 🛠️ Hardware Requirements & Pinout

### Core Hardware
* **Microcontroller:** ESP32 Dvekit 1 
* **CAN Transceiver:** SN65VHD230

> 🛑 **CRITICAL NOTE:** Always power your turnouts/servos from an independent, external 5V power supply. **Do not** draw servo power directly from the ESP32 shield to avoid brownouts or hardware damage.

### Pin Allocation
| Component / Function | ESP32 Pin Assignment |
| :--- | :--- |
| **CAN RX** | Pin 15 |
| **CAN TX** | Pin 2 |
| **Servo 1** | Pin 32 |
| **Servo 2** | Pin 33 |
| **Configurable I/O** | Pins 14, 27, 26, 25, 16, 17, 18, 19 |

---

## ⚙️ Configuration Mappings (CDI Options)

The module exposes structured definitions editable via LCC configuration tools (e.g., JMRI):

### 1. Servos
* **Positions:** Supports 3 state targets per servo (`Closed`, `Midpoint`, `Thrown`).
* **Range:** Range mapping from `0` to `180` degrees. Small adjustments should be inputted via text box instead of sliders for accuracy.
* **Save Period:** Saved automatically every $X \times X \times 100$ seconds (e.g., a value of `190` translates to roughly 60 minutes between saves if the servo has moved).

### 2. Input/Output Channel Modes
Each of the 8 available general I/O pins can be configured to one of the following channel types:

| ID | Type Description |
| :--- | :--- |
| **0** | None (Disabled) |
| **1** | Input |
| **2** | Input, Inverted |
| **3** | Input with pull-up |
| **4** | Input with pull-up, Inverted |
| **5** | Toggle |
| **6** | Toggle with pull-up |
| **7** | Output Phase A |
| **8** | Output Phase A, Inverted |
| **9** | Output Phase B |
| **10**| Output Phase B, Inverted |

* *Note: Ensure appropriate current-limiting resistors are used when pins are utilized as Outputs.*

---

## 📦 Software Dependencies

To compile this sketch, ensure the following are included in your development ecosystem:
* Arduino IDE (with EspressifESP32 board support installed) tested with 3.3.10
* OpenLCB / LCC Single Thread Library version 0.1.19
* Local header profiles: `Config.h`, `Boards.h`, `mdebugging.h`, `OpenLCBHeader.h`, `OpenLCBMid.h`
* Requires the installation of ESP32Servo from the library manager

---

## 🤝 Credits & History

* **Original Architecture:** Copyright © 2024 David P Harris (Derived from baseline protocols by Bob Jacobson Alex Shepherd & David Harris).
* **Updates:** * 2024.11.14 – DPH
    * 2026.06.24 – John Holmes. (Added using the core 0 for servo movement timing.)
