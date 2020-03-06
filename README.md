# DBELL 
## Smart doorbell and music player

1. Get a PCM5102 DAC module from Aliexpress/Ebay. (like this: https://nl.aliexpress.com/item/33061919226.html)
1. Get any ESP8266 board that you like

On the PCM5102 board iself:

1. Connect 3.3v with VCC, XMT
1. Connect GND with DMP, FLT, FMT

Connect ESP8266 
```
ESP         PCM5102
GPIO02 <--> LCK
GPIO15 <--> BCK
RXD    <--> DIN
3.3v   <--> 3.3v, VCC, XMT
GND    <--> GND, DMP, FLT, FMT
``` 

Note: to upload the sketch the first time you need to connect RX to the UART board.

