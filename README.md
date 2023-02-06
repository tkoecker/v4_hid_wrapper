# HID Keyboard and Mouse wrapper for the Vampire 4

The Vampire 4, in its current state, only supports a limited amount of USB Mice and Keyboards. This is a workaround to make more of them work with the V4. Also for already working devices that need to be unplugged and plugged in again in order to get recognized the adapter removes this requirement.

It uses combination of one or two "Pro Micro" (3.3V / 8 MHz version) Arduino clones, and a "Mini USB Host Shield 2.0" board as an adapter in between the keyboard/mouse and the V4.

The one device version can act as an adapter for either a mouse or a keyboard (which can be selected via a jumper).

![Adapter](img/adapter.jpg)

The two device version can act as an adapter for composite mouse+keyboard devices (for example wireless mouse+keyboard combos that only use one USB receiver dongle).

![Adapter](img/adapter2.jpg)

Build instructions to build the one device adapter can be found at:
- http://okiraku-camera.tokyo/blog/?p=8193 (japanese)
- https://geekhack.org/index.php?topic=80421.0

Build instructions to build the two device version:
[two_device_build.md](two_device_build.md)

3D printable cases can be found at: https://www.thingiverse.com/thing:4397113

### Uploading the precompiled hex file to the microcontroller
To ease the installation process I've uploaded a precompiled build: [v4_hid_wrapper.hex](v4_hid_wrapper.hex)
This file can be uploaded to the microcontroller using AVRDUDE: https://www.nongnu.org/avrdude/

#### Linux

Most distributions likely have an AVRDUDE package, for example on Ubuntu it can be installed using:
```
sudo apt-get install avrdude
```
When you plug in a "fresh" pro-micro the built-in CDC serial port will be recognized as a tty.
Here is an example output of `dmesg` when the microcontroller gets connected:
```
usb 1-9: new full-speed USB device number 63 using xhci_hcd
usb 1-9: New USB device found, idVendor=1b4f, idProduct=9208, bcdDevice= 1.00
usb 1-9: New USB device strings: Mfr=1, Product=2, SerialNumber=0
usb 1-9: Product: LilyPadUSB
usb 1-9: Manufacturer: SparkFun
cdc_acm 1-9:1.0: ttyACM0: USB ACM device
```
Here the name of the serial port is `ttyACM0`.
This port can be used to program the microcontroller with the downloaded hex file:
```
sudo avrdude -v -patmega32u4 -cavr109 -P/dev/ttyACM0 -b57600 -D -Uflash:w:v4_hid_wrapper.hex:i
```
#### Windows

You can download a Windows version of AVRDUDE from https://github.com/avrdudes/avrdude/releases.

Extract the AVRDUDE package and place the [v4_hid_wrapper.hex](v4_hid_wrapper.hex) file right next to the extracted files. Start a Windows Command Prompt in the same directory (for example by typing `cmd.exe` in the explorer address bar).

When you plug in a "fresh" pro-micro the built in CDC serial port will be recognized as a new COM port.

In this example the device manager has an entry called `USB Serial Device (COM5)`.

This port can be used to program the microcontroller with the downloaded hex file.
In the Windows Command Prompt type:
```
avrdude.exe -v -patmega32u4 -cavr109 -v -PCOM5 -b57600 -D -Uflash:w:v4_hid_wrapper.hex:i
```

### To build the software for the adapter the following is needed:
- Use Arduino IDE v1.6.5-r5 (needed for the changes below - the last version before a refactoring of the USB stack)
- Install the "SparkFun AVR Boards" (version 1.1.6, newer ones do not work anymore with the Arduino IDE v1.6.5-r5)
- Install the "USB Host Shield Library 2.0" (version 1.3.2 is fine (currently the latest one))
- Apply the changes from https://github.com/tkoecker/Arduino/commit/e6783cb5e4b7e1dc8abc4932b3e3543ad793c6ff.patch
  and https://github.com/tkoecker/Arduino/commit/790e3472ea00df73c100a9cd8d93a1ec98198a04.patch to
  get an USB core that uses low-speed USB, with only one boot protocol HID keyboard or mouse.

On the one device version the adapter software is in mouse mode by default. If a jumper is put between pin 2 and GND the adapter is in keyboard mode.

### TODO:
- Check if mice/keyboards connected via USB hubs can be supported (which would make it possible to share the same mouse and keyboard on different hosts via an USB switch).
