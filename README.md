ESP8266 probe sniffer
=====================

Listens on channel 1 for probe requests and reports them to UART. Might be useful to implement a simple WiFi-based occupancy sensor.

Baud rate is 9600 bps. New lines in DOS format to make terminals happy. Each report is in CSV format:

 * `init,,,` on power-up
 * `ping,,,` every 5 seconds (to let the receiver know it's alive)
 * `probe,<mac>,<rssi>,[ssid]` on probe request (SSID is optional)

Note that in crowded areas the default baud rade may not be enough to report all the probe requests. Espressif documentation mentions that sniffer function should return as quick as possible and I'm not sure whether `os_printf()` halts when output buffer is full or drops data. In my case the low speed is necessary because of the long cables.

Built as a native ESP8266 application using [PlatformIO](http://platformio.org/).

Distributed under the terms of MIT/Expat license.

