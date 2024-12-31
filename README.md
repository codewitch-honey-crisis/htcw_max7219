# MAX7219

A device driver for the MAX7219 matrix display from Arduino or the ESP-IDF

This takes the number of columns and rows of 8x8 matrices as an input.**

It then presents a `frame_buffer()` you can read or write to. It's in monochrome format with pixels packed 8 to a byte horizontally, with no alignment or padding.

Unlike many MAX7219 drivers this one does not expose drawing methods. It is largely intended to be used with a graphics library of your choosing so long as it can create monochrome bitmaps.

Instead, it prioritizes having a small footprint and being unobtrusive. It's a very compact library.

To use it you simply modify the contents of the frame buffer and call `update()` to update the display.

Make sure to call `initialize()`. This will autoinit for you but it's good practice.

Note that the Arduino orchestration will initialize the SPI bus for you, but the ESP-IDF variation requires you to initialize the bus/host yourself.

** I'm not so sure the rows work great. I've had some trouble getting those going past a single row


PlatformIO ini example
```
[env:node32s]
platform = espressif32
board = node32s
framework = arduino
lib_deps = 
	codewitch-honey-crisis/htcw_max7219
```