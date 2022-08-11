# libsibo

This is a collection of Arduino libraries for communicating with peripherals compatible with Psion's SIBO range of computers. It currently only includes code for the SIBO Serial Protocol (SIBO-SP) and dealing with Psion's proprietary flash filesystem, FEFS.

It used to be the main code for the Arduino portion of SIBODUMP, but it has had a major overhaul since then. It's still compatible with the Arduino Uno, but it now also adds:

* RP2040, specifically the Raspberry Pi Pico (using the official Arduino MBED core, and the one by [Earle Philhower](https://github.com/earlephilhower/arduino-pico) - [Wiz-IO](https://github.com/Wiz-IO/wizio-pico) support coming soon!)
* ESP32 (although this might go in the future as I'm focussing now on the RP2040)

It may also work with other microcontrollers - let me know if you get it working!

## Level Shifting

Part of the reason why I have left in the Arduino Uno support is that Psion's peripherals all use 5v logic. It makes life a lot easier to deal with when you're only having to worry about one voltage.

The Pico and the ESP32 use 3.3v logic, so you will need some form of level shifting. I personally am using a 74AXP1T45 and a 74HCT1G125, which both work with the ESP32 and the RP2040. These need extra pins to control the direction of DATA and disable CLK. I will release a schematic as soon as I can.

*I cannot guarantee that you won't fry your 3.3v microcontroller without some sort of level shifting!* I have been able to get an ESP32 to talk to some SSDs (ASIC4-based ones) without level shifting, and it didn't blow anything up. (ASIC5 won't work at all because it uses 5v CMOS, so completely ignores 3.3v logic.) However, that was my personal experience and YMMV. If you want to send 5v to your GPIOs, that's on you!

## SIBODUMP and ripping SSDs

As stated earlier, this code was originally just designed to send the contents of a Psion SSD over serial, but its features have expanded and will continue to expand.

You can still use this with [SIBODUMP](https://github.com/PocketNerdIO/sibo-ssd-dump) for ripping Psion SSDs.

If you don't want to use it with SIBODUMP and want to see the raw data coming through, you can use the Arduino sketch by itself with an app like RealTerm on Windows. Connect RealTerm to your Arduino's serial device, set RealTerm to 115200 baud, start the capture (input only) and send "d" to the Arduino. The dump will start immediately.

## Other Notes

**Please note that THIS IS ALPHA-QUALITY SOFTWARE.** It's poorly written and very likely to change signficantly in the future. Use it at your own risk.

This is a PlatformIO project. It may need some work to get it running with the official Arduino IDE.

The original version of this library was written by Karl with new features such as the command interpreter and block-by-block dumping added by me.

Try it, break it, give me feedback!
