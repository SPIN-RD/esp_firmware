# esp_firmware
ESP32 Firmware for the SiPIN experiment data acquisition. Compatible with ESP32-based boards.

##Instructions

1. Install Arduino IDE.
2. Add Espressif Systems' repository the Arduino Boards Manager: Go to File->Preferences->Additional Boards Manager URLs, and add "https://dl.espressif.com/dl/package_esp32_index.json".
3. Install ESP32 board files in the Arduino IDE: Go to Tools->Board->Boards Manager, and install the package named "esp32", by "Espressif Systems".
4. Install AsyncTCP library ("https://github.com/me-no-dev/AsyncTCP"): copy the "AsyncTCP-master" folder to the Arduino/libraries folder.
5. Install ESPAsyncWebServer library ("https://github.com/me-no-dev/ESPAsyncWebServer"): copy the "ESPAsyncWebServer-master" folder to the Arduino/libraries folder.
6. Download this repo, and open "SiPIN_esp32_firmware.ino". Connect the ESP32 board to the computer, select the correct board and port in the Tools menu (in our tutorial, we are working with a "NodeMCU-32S" board.
7. Compile, upload and run!
