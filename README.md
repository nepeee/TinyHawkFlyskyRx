# TinyHawkFlyskyRx
This is a Flysky AFHDS 2A compatible receiver firmware for the TI CC251x rf transceiver IC written for the Emax TinyHawk brushless quadcopter. It allows you to directly control the quad with a Flysky AFHDS 2A compatible RC radio transmitter like the FS-i6X without the use of an external receiver module.

Features:
- Supports the standard Flysky binding procedure, uses randomly generated receiver id for every bind
- Uses IBUS protocol instead of the original SBUS protocol for lower CPU overhead
- Supports 9 RC channels
- Reports the RSSI level(signal strength) over the AUX 10 channel
- Synchronizes with the channel hopping time period that makes the reception robust
- Fast resynchronization after a complete loss of signal (3.875ms)
- Uses hardware watchdog that allows the system to regain reception within 1 second in case of a system failure

Challenges for this project:
- The CC251x is not not completely compatible with the AMICCOM A7105 used in the Flysky receivers
- The AFHDS 2A protocol uses Hamming(7,4) error-correcting code that not supported by the CC251x
- The AFHDS 2A protocol uses 4 byte sync word, the CC251x only supports 2 bytes
- The AFHDS 2A protocol uses  CRC that not supported by the CC251x natively
- The AFHDS 2A protocol uses 500KHz channel spacing that is not supported by the CC251x with the 26MHz crystal oscillator
- Not much info on the internet about the AFHDS 2A protocol

How to flash the firmware:
1. Download the main.ihx file from this repo
2. Get some CC251x compatible flasher tool like the CC debugger or you can use an arduino or a raspberry pi

CC debugger: http://www.ti.com/tool/CC-DEBUGGER

arduino: https://github.com/fishpepper/CC2510Lib

raspberry pi: https://github.com/jimmyw/CC2510Lib

3. Connect the programmer to the TinyHawk board, the pinout is shown in the following link:
https://raw.githubusercontent.com/nepeee/TinyHawkFlyskyRx/master/pinout.jpg
4. Make a backup copy of the original firmware by downloading it from the CC251x (optional but highly recommended)
5. Flash the firmware

Binding procedure:
1. disconnect the battery from the quadcopter
2. press and hold down the bind button while connecting the battery to the quad
3. the green led indicates that the receiver is in binding mode
4. enter to binding mode on your radio transmitter
5. if the binding is completed the green led turns of, the transmitter exits from the binding mode

Betaflight configuration:
1. On the ports tab make sure that UART1 is configured as Serial Rx
2. On the configuration tab in the receiver section select serial-based receiver and the IBUS serial receiver provider
3. On the receiver tab select AETR1234 channel map and AUX 10 as the RSSI channel

Build:

If you want to build the binary from the source yo can use the sdcc compiler. The make.sh can be used to build, the clean.sh to delete the files made by the sdcc compiler(except the hex file).
Its highly recommmended to use linux or the windows bash subsystem because of the line endings in the generated hex file.

The code is heavily based on the OpenSky project:

https://github.com/fishpepper/OpenSky

Big thanks to fishpepper for their work!

The firmware is tested with the Emax TinyHawk 1.0 and a Flysky FS-I6X radio transmitter. If you used a different transmitter please let me now so i can update this readme.

If you have any questions or like to contribute in this project don't hesitate to contact me.
