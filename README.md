[![Documentation Status](https://readthedocs.org/projects/gc-1000-gps/badge/?version=latest)](https://gc-1000-gps.readthedocs.io/en/latest/?badge=latest)
[![Firmware Workflow](https://github.com/ac1ja/gc-1000-gps/actions/workflows/firmware_workflow.yml/badge.svg)](https://github.com/ac1ja/gc-1000-gps/actions/workflows/firmware_workflow.yml)

# Setup
Requires arduino, arduino-cli, make

Install build deps
```
sudo apt install build-essential make arduino
```

Clone and build the rep
```
git clone https://gitlab.com/silvertech-iot/gc-1000-gps.git
cd gc-1000-gps
make
make upload
```

Debugging notes
```
git clone https://gitlab.com/silvertech-iot/gc-1000-gps.git
cd gc-1000-gps
make upload && minicom -D /dev/ttyACM0 -b 115200
```

See docs for full instruction! https://gc-1000-gps.readthedocs.io/en/latest/


# Docs
Docs written with sphinx for use with readthedocs.
Info here https://docs.readthedocs.io/en/stable/intro/getting-started-with-sphinx.html
