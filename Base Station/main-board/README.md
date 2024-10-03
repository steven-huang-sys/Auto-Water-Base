# Main Board of the Base Station
This contains the code and build for the ESP32 planned to be inside the hub.

## Self Notes
- Remember to run the menuconfig as Bluetooth needs to be enabled before the code can be built. Adding an sdkconfig.default would also work.
- Currently using NimBLE instead of Bluedroid stack as we likely don't need Bluetooth Classic.


