## Component Datasheets

This folder contains datasheets for all components used in the Sleep Environment & Recovery Monitor project.

### Sensor Device Components

#### Microcontroller & Wireless
**Seeed Studio XIAO ESP32-C3**
- File: `esp32-c3_datasheet.pdf`
- Description: Low-power MCU with BLE and Wi-Fi capabilities
- Manufacturer: Espressif Systems
- [Product Page](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)

#### Sensors
**VEML7700**
- File: `veml7700_datasheet.pdf`
- Description: High-sensitivity ambient light sensor with I²C interface
- Manufacturer: Vishay Semiconductors
- Interface: I²C
- [Datasheet Link](https://www.vishay.com/docs/84286/veml7700.pdf)

**SPH0645LM4H-B**
- File: `sph0645lm4h_datasheet.pdf`
- Description: I²S MEMS microphone
- Manufacturer: Knowles
- Interface: I²S
- [Datasheet Link](https://www.digikey.com/htmldatasheets/production/1794128/0/0/1/SPH0645LM4H-B-Datasheet.pdf)

**TMP117**
- File: `tmp117_datasheet.pdf`
- Description: High-accuracy digital temperature sensor (±0.1°C)
- Manufacturer: Texas Instruments
- Interface: I²C
- [Datasheet Link](https://www.ti.com/lit/ds/symlink/tmp117.pdf)

OR **BME280**
- File: `bme280_datasheet.pdf`
- Description: digital temperature sensor
- Manufacturer: Bosch Sensortec
- Interface: I²C
- [Datasheet Link](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)

#### Power Management
**MCP73831**
- File: `mcp73831_datasheet.pdf`
- Description: LiPo battery charging IC
- Manufacturer: Microchip
- [Datasheet Link](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP73831-Family-Data-Sheet-DS20001984H.pdf)

### Display Device Components

#### Display
**SSD1306**
- File: `ssd1306_datasheet.pdf`
- Description: 128×64 OLED display driver with I²C interface
- Manufacturer: Solomon Systech
- Interface: I²C
- [Datasheet Link](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)

### Files in This Folder

All component datasheets are stored directly in the `datasheets/` folder:

```
datasheets/
├── README.md
├── esp32-c3_datasheet.pdf
├── veml7700_datasheet.pdf
├── sph0645lm4h_datasheet.pdf
├── tmp117_datasheet.pdf
├── mcp73831_datasheet.pdf
└── ssd1306_datasheet.pdf (to be added)
```