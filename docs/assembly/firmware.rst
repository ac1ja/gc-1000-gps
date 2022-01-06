Loading the firmware
====================

There are two methods for getting the code, you can download a release, or build from ``main``


Build from a release
####################

To build from a release, go to the releases_ page on our gitlab, and select the most recent release.

.. image:: images/releases_screenshot.png
  :width: 400
  :alt: Releases sreenshot

Download a copy of the sourcecode and extract it to your computer.

Open a terminal in the extracted repository.

.. code-block:: shell

    cd gc-1000-gps
    make
    make upload


Build from ``main``
###################

Building from the main branch may result in more experimental but more up to date features and stability.

Start by cloning the repo down, then CD to the build location and build the code.

.. code-block:: shell

    git clone https://github.com/ac1ja/gc-1000-gps
    cd gc-1000-gps/gc-1000-gps
    make
    make upload

Troubleshooting
###############

If you get an error such as ``avrdude: ser_open(): can't open device "unknown": No such file or directory`` your mega might not be plugged in or may not have enough power over usb to turn on.

If you get an error such as ``command not found: arduino-cli`` make sure you installed all dependencies_. 


.. _dependencies: https://github.com/ac1ja/gc-1000-gps#setup
.. _releases: https://github.com/ac1ja/gc-1000-gps/releases
