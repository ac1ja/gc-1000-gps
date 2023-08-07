[![Documentation Status](https://readthedocs.org/projects/gc-1000-gps/badge/?version=latest)](https://gc-1000-gps.readthedocs.io/en/latest/?badge=latest)
[![Firmware Workflow](https://github.com/ac1ja/gc-1000-gps/actions/workflows/firmware_workflow.yml/badge.svg)](https://github.com/ac1ja/gc-1000-gps/actions/workflows/firmware_workflow.yml)
[![Hardware Workflow](https://github.com/ac1ja/gc-1000-gps/actions/workflows/hardware_workflow.yml/badge.svg)](https://github.com/ac1ja/gc-1000-gps/actions/workflows/hardware_workflow.yml)

# Firmware
Requires platformio, make

Install build deps
```
# Ubuntu/Debian
sudo apt install build-essential make

# Platformio install script, see https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"
```

Or install platformio from [here](https://platformio.org/install), this project assumes
only pio core but the ide integrations will also work

## Build

Clone and build the repo
```
git clone --recurse-submodules https://github.com/ac1ja/gc-1000-gps.git
cd firmware
make
make flash
```

Debugging notes
```
git clone --recurse-submodules https://github.com/ac1ja/gc-1000-gps.git
cd firmware
make debug
```

See docs for full instruction! https://gc-1000-gps.readthedocs.io/en/latest/


# Hardware

## Install KiCAD

Install KiCAD from [here](https://www.kicad.org/download/), find the hardware files under `hardware`


# Where is everything?

```shell
├── cad                     # CAD and design files
├── firmware                # Software and Firmware source code
└── hardware                # PCB And component design files
    └── gc-1000-gps         # GC-1000-gps board!
        └── Libraries       # External kicad libs
```

# Docs
Docs written with sphinx for use with readthedocs.
Info here https://docs.readthedocs.io/en/stable/intro/getting-started-with-sphinx.html
