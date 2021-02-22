# ESP32 Frekvens Clock

ESP32 Firmware running inside a hacked Ikea x Teenage Engineering ][Frekvens LED Matrix Cube thingy](https://duckduckgo.com/?q=ikea+frekvens+led+matrix&iar=images&iax=images&ia=images).

Time synchronisation is handled via WiFI and NTP.

The led matrix is essentially a bunch of shift registers and a pwm signal to set brightness.
I've reverse engineerd the hardware, hooked up the esp and buildt an abstraction layer to drive the matrix.

## Build

using `idf.py`.

- Configure the project first with `idf.py menuconfig`
- Build with `idf.py build`
- Flash with `idf.py -p {PORT} flash monitor`
