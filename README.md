# FloatLevel
## Piezoelectric Submersible Level Sensor using ESP32 X4 Relay board. 
### Implements a Modbus server and an MQTT publisher. Relays are triggered at configurable water levels.

[![Hits](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2FClassicDIY%2FFloatLevel&count_bg=%2379C83D&title_bg=%23555555&icon=&icon_color=%23E7E7E7&title=hits&edge_flat=false)](https://hits.seeyoufarm.com)

[![GitHub stars](https://img.shields.io/github/stars/ClassicDIY/FloatLevel?style=for-the-badge)](https://github.com/ClassicDIY/FloatLevel/stargazers)

<a href="https://www.buymeacoffee.com/r4K2HIB" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

<p align="left">
  <img src="./Pictures/home_page.png" width="600"/>
</p>

This project is based on the ESP32, it works with a Submersible Level Sensor 4-20ma Pressure Sensors.

|<a href="https://www.aliexpress.com/item/1005005275943365.html"> ESP32 X4 Relay Board</a>|<img src="./Pictures/Esp32_X4_Relay_Board.png" width="200"/>|
|---|---|
|<a href="https://www.aliexpress.com/item/1005006366841583.html"> Piezoelectric Submersible 4-20mA Level Sensor </a>|<img src="./Pictures/Sensor.png" width="200"/>|

## Wiring

Device Pin | ESP32 |
--- | --- |
FloatLevel red  | 24VDC |
Use 135Ω (2 X 270Ω in parallel) from A0 to GND
FloatLevel black | A0 |

<p align="left">
  <img src="./Pictures/diagram.png" width="800"/>
</p>

Used the following development tools;

<ul>
  <li>Visual Studio Code with the PlatformIO extension.</li>
  <li>***** Don't forget to upload the index.htm file for the initial setup of the ESP32, Run 'Upload File System Image' platformio command</li>
  <li>Setup WIFI configuration by logging onto the "FloatLevel" Access Point, use the admin default pw: 12345678 as the AP password. Browse 192.168.4.1 and go to Configuration Page to provide your WIFI credentials.
</ul>

