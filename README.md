# I2C Driver – TMP117 High-Precision Temperature Sensor
 
A bare-metal embedded C driver for the **Texas Instruments TMP117** digital temperature sensor, implemented on a **Silicon Labs EFM32/EFR32** microcontroller using the hardware I2C peripheral.
 
This project demonstrates register-level I2C peripheral configuration, datasheet-driven driver development, and hardware verification using a Saleae Logic Analyser.
 
---
 
## Hardware
 
| Component | Description |
|-----------|-------------|
| MCU | Silicon Labs Wireless Gecko BGM2305C22 (ARM Cortex-M) |
| Sensor | TMP117NAIDRVR — ±0.1°C accuracy, 16-bit resolution |
| EEPROM | M24128X-FCU6T (128Kb, I2C) on same bus |
| PCB | INNIRION UC-Baseboard (1094) + UC-Companionboard (1095) |
| Interface | I2C @ ~38 kHz, address 0x48 |
| Tools | Saleae Logic Analyser, SEGGER J-Link debugger, bench power supply |
 
---
 
## Lab Setup
 
![Setup overview](images/IMG_4173.JPG)
*Silicon Labs Wireless Gecko MCU (left) connected to INNIRION custom PCB (centre) with Saleae Logic Analyser (right) probing the I2C bus*
 
![Setup powered](images/IMG_4174.JPG)
*Full test bench powered up — JTAG connected, Logic Analyser probes on I2C test points*
 
![ADC bring-up](images/IMG_4176.JPG)
*Hardware bring-up session: bench power supply, Wireless Gecko, INNIRION baseboard with ADC companion board. Green LEDs confirm successful power-on.*
 
---
 
## What it does
 
- Initialises the I2C0 peripheral on **PB0 (SCL)** and **PB1 (SDA)** with pull-up configuration
- Reads the **Device ID register (0x0F)** on startup to verify sensor presence — expected: `0x0117`
- Writes a configuration byte to the **Configuration register (0x01)**
- Continuously reads the **Temperature register (0x00)** and converts the raw 16-bit two's complement value to °C using the TMP117 LSB resolution of **0.0078125 °C/LSB**
- Verified with Logic Analyser captures and in-circuit debugger
---
 
## Code Structure
 
```
main.c
├── initCMU()            — Enable I2C0 and GPIO clocks
├── initGPIO()           — Configure interrupt (ALERT pin)
├── initI2C()            — Route I2C to PB0/PB1, init peripheral
├── I2C_LeaderRead()     — Generic I2C read: write register address, repeated START, read N bytes
├── I2C_LeaderWrite()    — Generic I2C write: address + data bytes
├── ReadDeviceID()       — Read 0x0F → verify 0x0117
├── ReadTemperatureRaw() — Read 0x00 → raw 16-bit value
├── GetTemperatureC()    — Convert raw → float °C
└── main()               — Init, verify ID, write config, poll temperature
```
 
---
 
## Key Implementation Details
 
### I2C Peripheral Routing (Silicon Labs ROUTE system)
```c
GPIO->I2CROUTE[0].SDAROUTE = (gpioPortB << _GPIO_I2C_SDAROUTE_PORT_SHIFT)
                             | (1 << _GPIO_I2C_SDAROUTE_PIN_SHIFT);
GPIO->I2CROUTE[0].SCLROUTE = (gpioPortB << _GPIO_I2C_SCLROUTE_PORT_SHIFT)
                             | (0 << _GPIO_I2C_SCLROUTE_PIN_SHIFT);
GPIO->I2CROUTE[0].ROUTEEN  = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
```
 
### Temperature Conversion (from TMP117 datasheet, Table 7-1)
```c
float GetTemperatureC(void) {
    uint16_t raw = ReadTemperatureRaw();
    int16_t temp_signed = (int16_t)raw;   // Two's complement signed value
    return temp_signed * 0.0078125f;       // 1 LSB = 7.8125 m°C
}
```
 
### Write-Read Sequence (I2C_FLAG_WRITE_READ)
The TMP117 requires a **write** of the register pointer address followed by a **repeated START** and read — implemented using the Silicon Labs `I2C_FLAG_WRITE_READ` transfer flag.
 
---
 
## Verification
 
### Logic Analyser — Configuration Write
Captured on Saleae Logic: `Setup Write to [0x48] + ACK` → `0xAB + ACK`
 
![Config Write](images/LA-Configdata.png)
 
SCL frequency: **~38 kHz**, duty cycle: **29%**
 
### Logic Analyser — Device ID Read
Write pointer `0x0F`, repeated START, read 2 bytes → `0x01 0x17` = **Device ID 0x0117** ✓
 
![Device ID Read](images/LA-Device_ID.png)
 
### Logic Analyser — Temperature Read
Write pointer `0x00`, repeated START, read 2 bytes → raw temperature data
 
![Temperature Read](images/LA-Temp.png)
 
### Debugger — Verified Values
 
| Variable | Value | Meaning |
|----------|-------|---------|
| `currentDeviceID` | `0x0117` (279) | TMP117 confirmed present |
| `lastWrittenConfig` | `0xAB` (171) | Config register write confirmed |
| `currentTemperature` | `23.671875` | 23.67°C room temperature |
 
---
 
## Schematic
 
I2C bus with TMP117 (address `0x48`, ADD0 → GND) and M24128X EEPROM (address `0xA0`), 10kΩ pull-up on SDA, pull-up on ALERT pin.
 
![Schematic](images/Pin_configuration.png)
 
---
 
## What I learned
 
- Register-level I2C peripheral configuration on Silicon Labs EFM32 (ROUTE system, wired-AND pull-up mode)
- Two's complement signed 16-bit to float conversion as specified in the TMP117 datasheet
- Using the Saleae Logic Analyser to verify ACK/NAK, timing parameters, and data frames against datasheet specifications
- Debugging embedded C with SEGGER J-Link — watching variables update in real time
---
 
## Tools & Environment
 
- **IDE:** Simplicity Studio (Silicon Labs)
- **Toolchain:** GCC ARM
- **Debugger:** SEGGER J-Link
- **Logic Analyser:** Saleae Logic 2
- **Sensor Datasheet:** [TMP117 (TI SNOSD82D)](https://www.ti.com/lit/ds/symlink/tmp117.pdf)
