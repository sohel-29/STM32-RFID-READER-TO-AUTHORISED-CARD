# STM32 RFID Access Control System
RFID-based access control** using STM32 microcontroller, MFRC522 RFID reader, LED, and buzzer.

This project reads RFID card UIDs. **Authorized card** (`69 EA 70 06`) turns **LED ON**. **Unauthorized cards** trigger the **buzzer**.

## Features

- MFRC522 RFID reader via SPI
- UART debugging output
- **LED indication for authorized access** (PC10)
- **Buzzer alert for unauthorized access** (PC11)
- STM32 HAL drivers + STM32CubeIDE

## Hardware Connections

MFRC522 RFID Module:
├── SDA/SS → PA4 (NSS)
├── SCK → PA5 (SPI1_SCK)
├── MOSI → PA7 (SPI1_MOSI)
├── MISO → PA6 (SPI1_MISO)
├── RST → PB0 (GPIO)
├── VCC → 3.3V
└── GND → GND

LED:
├── PC10 ──[220Ω]── LED(+), LED(-) ── GND

Buzzer (Active):
├── PC11 ── Buzzer(+), Buzzer(-) ── GND


## Working Logic

1. **Card tapped** → UID read via SPI → Printed on UART
2. **UID = `69 EA 70 06`** → **LED ON** (PC10) for 2 seconds ✅
3. **Any other UID** → **Buzzer ON** (PC11) for 2 seconds ❌
4. **No card** → Both LED + Buzzer OFF

## Example Output
Card UID: 69 EA 70 06 ← LED GLOWS ✅
Card UID: 97 C1 EF 05 ← BUZZER SOUNDS ❌

## Project Files
├── Core/ # Main application
├── Drivers/ # STM32 HAL drivers
├── .ioc # CubeMX configuration
├── mfrc522.h/c # RFID library
└── README.md # This file


## How to Build & Run

1. Open project in **STM32CubeIDE**
2. Verify **SPI1**, **UART2**, **PC10/PC11 GPIO** configuration
3. **Build** → **Flash** to STM32
4. Open **serial terminal** (9600 baud)
5. **Test** with authorized card `69 EA 70 06` and other cards

## GPIO Configuration
PC10 → GPIO_Output (LED)
PC11 → GPIO_Output (Buzzer)
PA4 → GPIO_Output (RFID NSS/CS)
PB0 → GPIO_Output (RFID RST)

## Notes

- **MFRC522 VCC = 3.3V only** (not 5V)
- Some modules show `Version: 0xB2` (still works fine)
- **Active buzzer** works with simple HIGH/LOW
- **220Ω resistor** required for LED current limiting

## Author

**Mohammed Sohel**  
Embedded Systems Engineer  
[LinkedIn]:- (https://www.linkedin.com/in/sohel29)

## Future Enhancements

- Multiple authorized cards
- EEPROM UID storage
- Relay for door lock
- LCD/OLED display
- Web dashboard via ESP32
