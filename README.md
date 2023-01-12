[![Documentation Status](https://readthedocs.org/projects/gc-1000-gps/badge/?version=latest)](https://gc-1000-gps.readthedocs.io/en/latest/?badge=latest)

# Setup
Requires arduino, arduino-cli, make

Install build deps
```
# Ubuntu/Debian
sudo apt install build-essential make

# Platformio install script, see https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"
```

Clone and build the rep
```
git clone https://gitlab.com/silvertech-iot/gc-1000-gps.git
cd gc-1000-gps/firmware
make
make upload
```

Debugging notes
```
git clone https://gitlab.com/silvertech-iot/gc-1000-gps.git
cd gc-1000-gps/firmware
make debug
```

See docs for full instruction! https://gc-1000-gps.readthedocs.io/en/latest/


# Docs
Docs written with sphinx for use with readthedocs.
Info here https://docs.readthedocs.io/en/stable/intro/getting-started-with-sphinx.html
