<p align="center"><img src="docs/assets/logo.png" alt="Logo" width="250px" /></p>


This project upgrades a Gaggia espresso machine with smart controls to improve your coffee-making experience. By adding a display and custom electronics, you can monitor and control the machine more easily.

## Features

- **Temperature Control**: Monitor the boiler temperature to ensure optimal brewing conditions.
- **Brew timer**: Set a target duration and run the brewing for the specific time.
- **Steam and Hot Water mode**: Control the pump and valve to run the respective task.
- **Safety Features**: Automatic shutoff if the system becomes unresponsive or overheats.
- **User Interface**: Simple, intuitive display to control and monitor the machine.

## Screenshots

<img src="docs/assets/standby-screen.png" alt="Standby Screen" width="250px" />
<img src="docs/assets/brew-screen.png" alt="Brew Screen" width="250px" />

## Hardware

- **Gaggia Espresso Machine**
- **LilyGo T-RGB 2.1" Display**: Runs the user interface and communicates with the system.
- **ESP-WROOM32 DevKit**: Handles the machine’s low-level control.
- **K-Type M4 Thermocouple with MAX6675**: Reads the temperature of the boiler.
- **AC SSR**: Controls the heating element (boiler).
- **2-Channel 5V Relay**: Controls the pump and valve.

## How It Works

The display allows you to control the espresso machine and see live temperature updates. If the machine becomes unresponsive or the temperature goes too high, it will automatically turn off for safety.

## Installation

1. Connect the ESP32-S3 Display, ESP-WROOM32 DevKit, and other components to the Gaggia.
2. Upload the firmware to both microcontrollers.
3. Enjoy your smarter coffee-making experience!

## License

This work is licensed under CC BY-NC-SA 4.0. To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
