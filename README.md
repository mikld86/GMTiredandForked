<p align="center">
<img src="docs/assets/logo.png" alt="Logo" width="250px" />
<br />
  
[![](https://dcbadge.vercel.app/api/server/APw7rgPGPf)](https://discord.gg/APw7rgPGPf)
[![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]


</p>


This project upgrades a Gaggia espresso machine with smart controls to improve your coffee-making experience. By adding a display and custom electronics, you can monitor and control the machine more easily.

<img src="docs/assets/gaggimate-render.png" alt="Gaggia Classic Installation" width="500" />

## Features

- **Temperature Control**: Monitor the boiler temperature to ensure optimal brewing conditions.
- **Brew timer**: Set a target duration and run the brewing for the specific time.
- **Steam and Hot Water mode**: Control the pump and valve to run the respective task.
- **Safety Features**: Automatic shutoff if the system becomes unresponsive or overheats.
- **User Interface**: Simple, intuitive display to control and monitor the machine.

## Screenshots and Images

<img src="docs/assets/standby-screen.png" alt="Standby Screen" width="250px" />
<img src="docs/assets/brew-screen.png" alt="Brew Screen" width="250px" />
<img src="docs/assets/pcb_render.jpg" alt="Brew Screen" width="250" />

## BOM

AliExpress links provided are affiliate links and supporting the project.

### PCB Build

The recommended way to build this machine is to use the PCB designed for this project. There's currently a group buy started for this.

https://forms.gle/KEXdpgJGCZbsFdKD7

### Lego Build

If you do not want to use the PCB you can buy the components on their own:

- **ESP-S3 DevKit** https://s.click.aliexpress.com/e/_EzXyAvP (N8R8)
- **2-Channel 5V Relay** https://s.click.aliexpress.com/e/_ExUVY9J (2 or 3 channel if you want the Grinder integration)
- **MAX6675 Temperature sensor board** https://s.click.aliexpress.com/e/_EG1t3V7 (Module)

### General components

- **LilyGo T-RGB 2.1" Display** https://s.click.aliexpress.com/e/_Eju6rYD (Full Circle)
- **AC SSR** https://s.click.aliexpress.com/e/_EvPScvr (SSR-40DA)
- **K-Type M4 Thermocouple** https://s.click.aliexpress.com/e/_Exzhqx7 (K-Type M4 0.5m)

## How It Works

The display allows you to control the espresso machine and see live temperature updates. If the machine becomes unresponsive or the temperature goes too high, it will automatically turn off for safety.

## Installation

The installation process of this mod consists of purely electrical changes. You will have to wire up the 3-Way valve and pump to the relays provided on the board.
The existing thermostats of your machine have to be removed and bridged while connecting the heater to the SSR instead. The M4 thermistor goes into the screw hole where the brew
thermostat would usually sit.

I will soon update this with detailed instructions.

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg?style=for-the-badge

## License

This work is licensed under CC BY-NC-SA 4.0. To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
