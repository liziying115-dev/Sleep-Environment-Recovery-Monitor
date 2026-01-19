## Sleep Environment & Recovery Monitor
### Project Overview
#### What it does

The Sleep Environment & Recovery Monitor evaluates overnight sleep conditions — light, sound, temperature — and translates them into a simple, physical recovery score visible in the morning. Instead of tracking the body, the system focuses on environmental quality to encourage healthier sleep habits without medical claims.

#### What it looks like

The system consists of two devices:

- A sensor unit placed on a nightstand that quietly monitors the sleep environment overnight

- A display unit resembling a small alarm clock with an OLED screen 
  and soft LED indicator that communicates the sleep quality score at a glance.

📌 Insert overview sketch here
/sketches/overview.png
(Show both devices, gauge, LED glow, button, and nightstand placement)

### Sensor Device (Sleep Environment Monitor)
#### Description
The sensor device continuously monitors environmental conditions during sleep using a low-power duty cycle. Data is aggregated overnight and transmitted wirelessly to the display device in the morning.

#### Sensors & Parts (part numbers)
| Function | Part Number | Description |
|---------|------------|-------------|
| MCU + Wireless| Seeed Studio XIAO ESP32-C3| BLE, low power |
| Ambient Light Sensor | VEML7700 | High-sensitivity ambient light sensor (I²C) |
| Microphone | SPH0645LM4H-B | I²S MEMS microphone, RMS only |
| Temperature | TMP117 | High-accuracy temperature sensing |
| Battery Charger | MCP73831 | LiPo charging IC |

#### How it works
- The device wakes on a scheduled interval during sleep hours.
- Light, sound RMS, temperature are sampled.
- Audio is processed locally (no recording or storage).
- All data is time-weighted and stored in local memory.
- At wake time, a summary packet is transmitted to the display device.

### Display Device (Score Interface)
#### Description
The display device presents the overnight Sleep Environment Score using a low-power OLED screen combined with a soft ambient LED indicator. The OLED screen provides clear, numerical feedback in the morning, while the LED glow offers an immediate, non-intrusive visual sense of sleep quality. A single button allows the user to cycle through secondary metrics such as light exposure, noise level, and temperature.

#### Components & Parts
| Function | Part Number | Notes |
|---------|------------|------|
| MCU + Wireless | Seeed Studio XIAO ESP32-C3 | Wireless receiver |
| Display | SSD1306 OLED (128×64) | Low-power I²C screen |
| LED | Warm White Diffused LED | Ambient sleep quality indicator |
| Button | Tactile Momentary Switch | Metric navigation |
| Battery | LiPo 2000 mAh | Multi-day operation |

### System Communication & Data Flow

![Data Flow Diagram](./data_flow_diagram.png)

*Figure: Complete data pipeline from sensors through processing to display outputs.*


#### System Communication
The system consists of two standalone devices: a sleep environment sensor device and a display device. The sensor device operates independently overnight, collecting and aggregating environmental data. In the morning, a single summary packet is transmitted wirelessly to the display device using BLE or Wi-Fi.

Communication between the sensor device and the display device is implemented using BLE. BLE is selected due to its low power consumption and suitability for short-range, infrequent data transmission in a sleep environment context.

### Data Flow & Processing Pipeline
During the sleep period, the sensor device periodically samples ambient light, sound level, and temperature.
Audio data is processed locally using RMS calculation and basic spectral weighting to emphasize disruptive noise patterns.
All sensor readings are time-weighted and aggregated into a single overnight dataset.

After aggregation, the data is converted into a normalized Sleep Environment Score.
This score is transmitted to the display device, where it is visualized on a low-power screen.
A soft ambient LED glow provides an additional non-screen feedback channel, reinforcing the overall sleep quality without requiring focused attention.

### Future Work
- Optional Wi-Fi connectivity to support long-term data logging or remote visualization