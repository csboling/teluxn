# teluxn

teluxn is an [uxn](https://wiki.xxiivv.com/site/uxn.html) emulator for the [monome teletype](http://monome.org/docs/teletype/).

You can load a compiled uxn .rom file by putting it on a USB stick and inserting it in Teletype. If your roms are already on an SD card, you can also use a USB SD card reader (at least the ones I've tried). Pick which rom you want with the PARAM knob, then load it by tapping the button next to the USB port. To load a different rom, re-insert the USB disk. Currently, only the first 8 .rom files on the drive are available.

| addr | device | 0x00   | 0x08  |
| ---- | ------ | ------ | ----- |
| 0x00 | system |        | red   |
|      |        |        | green |
|      |        |        | blue  |
| 0x02 | screen | vector | x     |
|      |        | width  | y     |
|      |        | height | addr  |
|      |        |        | color |
| 0x80 | ctrl   | vector |       |
|      |        | button |       |
|      |        | key    |       |
| 0xc0 | gpio   | vector |       |
|      |        | tr out |       |
|      |        | tr in  |       |
