# Growver Motor Control

The Main Application for Growver Robot using a Growver 2020 control module. The control module is based on an ESP32 (ESP-WROOM32 Module) from EspressIF.

Follow instructions at [ESDP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/) for configuring the ESP32 toolchain.
This code build with ESP-IDF version 4.1.

The following APIs are supported

| API               | Method | Resource Example                                      | Description                                                                              |
| ------------------| ------ | ----------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| `/api/v1/motor`   | `GET`  | {<br />left_speed:100,<br />left_dir:0<br /> right_speed:100,<br />right_dir:0}<br />} | Reads current motor speed and direction                 |
| `/api/v1/motor`   | `POST` | {<br />left_speed:100,<br />left_dir:0<br />}         | Sets motor speed and direction                                                           |
| `/api/v1/pump`    | `POST` | {<br />speed:100<br />}                               | Set pump speed from 0..100%                                                              |
| `/api/v1/status`  | `GET`  | { <br />battery_v:12.0<br />}                         | Read system status including battery voltage                                             |
| `/api/v1/servo`   | `POST` | { <br />angle:12.0<br />}                             | Set servo angle in degrees                                                               |

For UART control refer to the commandline module, or connect a terminal to the CMD port (115200,8,n,1) and type `help`