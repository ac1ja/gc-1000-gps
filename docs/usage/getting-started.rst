Getting Started
===============

The GC-1000-GPS is a drop-in replacement board for the GC-1000 radio disciplined clock, giving
some life and utility back to your super accurate radio synchronized clock!


Where to start?
---------------

Once you have a kit, check out the :ref:`Assemble the GC-1000-GPS` section 
to begin the process of installing and configuring your board for the first time!


How it works
============

The original GC-1000 synced with the WWV station in colorado for precise control over the disciplined oscillator.
The GC-1000-GPS board uses GPS to accomplish much the same goal, syncing a local RTC to use as a high precision
source and not drifting over time.


Sequence Diagram
----------------

.. mermaid::

   sequenceDiagram
      participant GPS
      participant Arduino
      participant RTC

      Arduino->>RTC: Is sync required?
      RTC-->>Arduino: Check if RTC is not synced,<br/>time between sync is too long,<br/>or gps is available
      
      Arduino-->>GPS: Check if GPS is ready<br/>Sat number, last time, etc
      GPS->>Arduino: Crack a valid gps time
      Note left of GPS: GPS may not have<br/>valid sync yet.<br/>Arduino will wait here.
      Note right of Arduino: We set a flag here and advance the<br/>second counter. We will wait for the GPS<br/>Inturrupt to trigger
      Arduino->>Arduino: Set SyncReady Flag
      critical Inturrupt Sequence
        GPS-)Arduino: PPS inturrupt!
        RTC-->>Arduino: Compare time to rtc to calculate drift
        Arduino-)RTC: Set precise time.
      end
      
GPS Time Spec
-------------

The gps time spec is formatted like such...


Disciplined Oscillator
----------------------
.. note::

   The current revision of boards do NOT have a disciplined oscillator mode.
   This may be changed in the future!


Extra tidbits
=============

* Original Manual + schematics :download:`pdf <../../gc-1000-gps/docs/Heathkit_GC1000H_Accurate_Clock_(schematic).pdf>`.
* `PRC68.com/Brooke Clarke`_ has some interesting time disecting their gc-1000 and the info was very helpful for making this project.


.. _PRC68.com/Brooke Clarke: http://www.prc68.com/I/HeathkitGC1000.shtml
