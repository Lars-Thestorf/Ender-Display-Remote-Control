# Ender-Display-Remote-Control
Adds wifi remote control ability to stock Creality 3D Printers using an ESP32 Microcontroller

ESP32 will read and interpret SPI Messages to the Display and host a Webpage to view the content

## Prerequirements
Install esp-idf
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started-get-esp-idf

## Setup
clone this repository
copy example config

``cd Software``

``cp example_sdkconfig sdkconfig``

Enter your wifi credentials with

``idf.py menuconfig``
