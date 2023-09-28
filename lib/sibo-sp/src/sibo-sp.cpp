/*
 *
 * SIBO-SP On Arduino
 *
 * Supports:
 *     - Pi Pico (official Arduino core and Earle Philhower - you will need level shifting from 3.3v to 5v and back!)
 *     - Arduino Uno (because sometimes it's nice to not have to deal with level shifters)
 *     - ESP32 (for now, but will likely be removed in the future)
 * 
 * THIS IS ALPHA CODE! It's poorly written and very likely to change signficantly in the future. Use it at your own risk.
 *
 * It is written using PlatformIO.
 */


// TODO: Better detection of official Arduino boards to enable Direct Pin Mode
// TODO: Reporting of IDs could be done differently. For example, will ASIC4 show ID:6 every time, whether or not it's forcing ASIC5 mode (ID:2)?
// TODO: MUCH separation! Hardware and the basic three types of frame (Control, Data Out, Data In) should be separate to specific peripheral types.
// ? Should check in the EPOC16 code to see how it's done there. If it's even in there...
// TODO: Make built-in Arduino GPIO functions switchable but "universal"
// ? Should each plaform's own GPIO functions be default, or have Arduino functions as default?
// TODO: Check device selection
// ? If Port D is set, what happens to Port C?
// ?  - Is it reset to 0?
// ?  - Are just the first 5 bits reset, so the device (CS) is the same?
// ?  - Is it untouched?
// TODO: Strings are bad in Arduino land, m'kay? Find an alternative (char*?)



#include "sibo-sp.h"

SIBOSPConnection::SIBOSPConnection() {
    _direct_pin_mode = false;
    _force_asic5 = false;

// Setting some arbitrary default pins here.
#if defined(RP2040)
    setDataPin(19);
    setClockPin(16);
    setDirPin(18);
    setClockEnablePin(17);
#elif defined(ESP32)
    setDataPin(17);
    setClockPin(16);
    setDirPin(5);
    setClockEnablePin(23);
    #define sleep_ms usleep  // Hacky way of dealing with a missing function
#else
    setDataPin(3);
    setClockPin(2);
#endif

    Reset();
}


// Headers and footers (idle bit) for frames

// _SendDataHeader()
// Send start bit, high ctl bit, then the idle bit
void SIBOSPConnection::_SendDataHeader(bool is_input_frame) {
    _DataPinMode(OUTPUT);
    _DataPinWrite(HIGH);
    _EnableClock();
    _ClockCycle(2);
    _DataPinWrite(LOW);

    if (is_input_frame) _DataPinMode(INPUT);

    // Idle Bit (Cycle 3)
    _ClockCycle(1);
}

// // Send start bit, low ctl bit, then the idle bit
// void SIBOSPConnection::_SendControlHeader() {
//     _DataPinMode(OUTPUT);
//     _DataPinWrite(HIGH);
//     _ClockCycle(1);

//     _DataPinWrite(LOW);
//     _ClockCycle(2);

//     // _SendIdleBit();
// }

// void SIBOSPConnection::_SendIdleBit(bool is_input_frame) {
//     if (is_input_frame) {
//         // As per Psion spec, if switching mode for input frame then
//         // controller sets input at end of cycle 2 and cycle 11
//         _DataPinMode(INPUT);
//     } else {
//         _DataPinWrite(LOW);
//     }

//     _ClockCycle(1);
// }


// Pin flipping utility methods

void SIBOSPConnection::_DataPinWrite(uint8_t val) {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) {
        if (val == LOW) {
            PORTD &= ~_data_bit;
        } else {
            PORTD |= _data_bit;
        }
        return;
    }
#endif
    digitalWrite(_data_pin, val);
}

void SIBOSPConnection::_DataPinMode(uint8_t mode) {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) {
        if (mode == INPUT) {
            PORTD &= ~_data_bit;
            DDRD &= ~_data_bit;
        } else if (mode == OUTPUT) {
            DDRD |= _data_bit;
        }
    } else {
        pinMode(_data_pin, mode);
    }
#else
    if (mode == INPUT) {
        // pinMode(_data_pin, INPUT_PULLDOWN);
        pinMode(_data_pin, INPUT);
        digitalWrite(_dir_pin, LOW);
    } else {
        pinMode(_data_pin, OUTPUT);
        digitalWrite(_dir_pin, HIGH);
    }
#endif
}

void SIBOSPConnection::_ClockPinReset() {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) {
        DDRD |= _clock_bit;
        return; // skip the rest
    // } else {
    //     pinMode(_clock_pin, OUTPUT);
    }
#endif
//#else
    pinMode(_clock_pin, OUTPUT);
//#endif
}

void SIBOSPConnection::_EnableClock() {
    pinMode(_clock_pin, OUTPUT);
    pinMode(_clock_enable_pin, OUTPUT);
    digitalWrite(_clock_enable_pin, LOW); // Possibly not needed
}

void SIBOSPConnection::_DisableClock() {
    pinMode(_clock_pin, INPUT);
    pinMode(_clock_enable_pin, OUTPUT);
    digitalWrite(_clock_enable_pin, HIGH);
//    pinMode(_clock_enable_pin, HIGH);
}

void SIBOSPConnection::_DataPinReset() {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) { // && BOARD == "Uno"
        DDRD |= _clock_bit;
    } else {
        pinMode(_clock_pin, OUTPUT);
    }
#else
    _DataPinMode(INPUT);
#endif
}

void SIBOSPConnection::_DirPinReset() {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) { // && BOARD == "Uno"
        DDRD |= _clock_bit;
    } else {
        pinMode(_clock_pin, OUTPUT);
    }
#else
    pinMode(_dir_pin, OUTPUT);
    digitalWrite(_dir_pin, LOW);
#endif
}


// Move Clock Pin HIGH and then LOW to indicate a cycle
void SIBOSPConnection::_ClockCycle(byte cycles) {
#if defined(FASTPIN_PORTD)
    if (_direct_pin_mode) {
        for (byte _cx = 0; _cx < cycles; _cx++) {
            PORTD |= _clock_bit;
            PORTD &= ~_clock_bit;
        }
    } else {
        for (byte _cx = 0; _cx < cycles; _cx++) {
            digitalWrite(_clock_pin, HIGH);
            digitalWrite(_clock_pin, LOW);
        }
    }
#elif defined(RP2040)
    for (byte _cx = 0; _cx < cycles; _cx++) {
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1); // Repeated multiple times to slow down the transfer
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        gpio_put(_clock_pin, 1);
        
        // sleep_us(0);
        gpio_put(_clock_pin, 0);

        // sleep_us(0);
        // digitalWrite(_clock_pin, HIGH);
        // digitalWrite(_clock_pin, LOW);
    }
#else
    for (byte _cx = 0; _cx < cycles; _cx++) {
        digitalWrite(_clock_pin, HIGH);
        digitalWrite(_clock_pin, LOW);
    }
#endif
}

// SIBO-SP reads on CLK up and writes on CLK down. So this sends:
// - down CLK and data up/down
// - up CLK and maintains data
//
//? What happens if this is called on a machine without any fast pin functionality?
//! This needs to error if the above happens. It should have been checked beforehand.
void SIBOSPConnection::_SendDataBitFast(bool val) {
#if defined(RP2040)
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (val << _data_pin));
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (val << _data_pin)); // Slow it down...
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (val << _data_pin));
    
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (1 << _clock_pin) + (val << _data_pin));
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (1 << _clock_pin) + (val << _data_pin));
    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  (1 << _clock_pin) + (val << _data_pin));

    gpio_put_masked((1 << _clock_pin) + (1 << _data_pin),  0);
#endif
}


// Frame functions

/* Frame structure
   Bit  0   1   2   3   4   5   6   7   8   9   10  11
        ST  CTL I1  D0  D1  D2  D3  D4  D5  D6  D7  I2
   -
   ST   = Start Bit
   CTL  = Control Bit
   I1   = Idle bit
   D0-7 = Data bits
   I2   = Idle bit

   According to Psion spec, DATA switches to INPUT at the end of the frame.
*/

// Resets the line, but not the slave. Transmitted by master to ensure all slaves are synchronised.
// Tristate the DATA pin, then send 12 CLOCK pulses.
void SIBOSPConnection::sendNullFrame() {
    _EnableClock();
    _DataPinMode(INPUT); // Just to make sure
    _ClockCycle(12);
    _DisableClock();
}

// Sends a 12-bit control frame to the slave with the payload in 'data'.
void SIBOSPConnection::sendControlFrame(byte data) {
    _EnableClock();

    // Send start bit (Cycle 1)
    _DataPinMode(OUTPUT);
#ifdef RP2040
    _SendDataBitFast(1);
    _SendDataBitFast(0);
    _SendDataBitFast(0);    
#else
    _DataPinWrite(HIGH);
    _ClockCycle(1);
    // Send low ctl bit and first idle bit (Cycles 2-3)
    _DataPinWrite(LOW);
    _ClockCycle(2);
#endif


    // Send payload (Cycles 4-11)
    _SendPayload(data);

    // Send last idle bit (Cycle 12)
    // _SendIdleBit();
#ifdef RP2040
    _SendDataBitFast(0);    
#else
    _ClockCycle(1);
#endif

    // End the frame by switching to input
    _DataPinMode(INPUT);
    _DisableClock();
}

// Received by controller from slave, input during header
byte SIBOSPConnection::fetchDataFrame() {
    // Send start bit, data bit, switch to INPUT and "send" first idle bit (Cycles 1-3)
    _SendDataHeader(IS_DATA_INPUT_FRAME);

    int input = 0;

    // Start fetching bits by sending clock pulses and reading the data line (Cycles 4-11)
#if !defined(FASTPIN_UNAVAILABLE)
    if (_direct_pin_mode) {
        for (byte _dx = 0; _dx < 8; _dx++) {
#if defined(FASTPIN_PORTD)
            PORTD |= _clock_bit;
            input |= (((PIND & _data_bit) == _data_bit) << _dx);
            PORTD ^= _clock_bit;
#elif defined(RP2040)
            gpio_put(_clock_pin, 1);
            input |= (gpio_get(_data_pin) << _dx);
            gpio_put(_clock_pin, 0);
#endif
        }
    } else {
        for (byte _dx = 0; _dx < 8; _dx++) {
            digitalWrite(_clock_pin, HIGH);
            input |= (digitalRead(_data_pin) << _dx);
            digitalWrite(_clock_pin, LOW);
        }
    }
// #elif defined(RP2040)
//     if (_direct_pin_mode) {
//         for (byte _dx = 0; _dx < 8; _dx++) {
//         }
//     } else {
//         for (byte _dx = 0; _dx < 8; _dx++) {
//             digitalWrite(_clock_pin, HIGH);
//             input |= (digitalRead(_data_pin) << _dx);
//             digitalWrite(_clock_pin, LOW);
//         }
//     }
#else
    for (byte _dx = 0; _dx < 8; _dx++) {
        digitalWrite(_clock_pin, HIGH);
        input |= (digitalRead(_data_pin) << _dx);
        digitalWrite(_clock_pin, LOW);
    }
#endif

    // Send last idle bit (Cycle 12)
    _ClockCycle(1);

    // NOTE: Data pin direction left as INPUT to comply with spec

    _DisableClock();
    return input;
}


// Sends a 12-bit data frame to the slave with the payload in 'data'.
void SIBOSPConnection::sendDataFrame(byte data) {
    _SendDataHeader(false);
    _SendPayload(data);
    // _SendIdleBit();
    _ClockCycle(1);

    // End the frame by switching to input
    _DataPinMode(INPUT);
    _DisableClock();
}

// Deselects slave without resetting it
void SIBOSPConnection::deselectASIC() {
    sendControlFrame(SP_SSEL);
}

void SIBOSPConnection::_SendPayload(byte data) {
    int output = 0;
    _DataPinMode(OUTPUT); // Not really needed if this is only ever being called after a header has been sent
    for (byte _dx = 0; _dx < 8; _dx++) {
        output = ((data & (0b00000001 << _dx)) == 0) ? LOW : HIGH;
//#if defined(RP2040)
//        _SendDataBitFast(output);
//#else
        _DataPinWrite(output);
        _ClockCycle(1);
//#endif
    }
    if (output) _DataPinWrite(0); // switch off data pin if it's been left on by the last bit in the payload (D7)
    // _DataPinMode(INPUT);
}


// Address Control

// TODO: Double check that this is actually how ASIC5 is controlled, because writing to Port B doesn't seem to be doing what I thought it should do.
void SIBOSPConnection::_SetAddress5(unsigned long address) {
    byte a0 = address & 0xFF;
    byte a1 = (address >> 8) & 0xFF;
    byte a2 = ((address >> 16) & 0b00011111) | ((_cur_device << 6) & 0b11000000);

    // ports D + C
    sendControlFrame(SP_SCTL_WRITE_MULTI_BYTE | 3);
    sendDataFrame(a1); // Port D
    sendDataFrame(a2); // Port C //? Don't know if this actually needs to be sent when a2 == 0, but EPOC16 does it

    // send port B address
    // // ! This is being ignored by ASIC5.
    // sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 1);
    // sendDataFrame(a0);

    // Emulating the weird method EPOC16 uses to increment Port B, as writing to Register 1 to change Port B directly seems to do nothing.
    // TODO: Check Forced ASIC5 mode on ASIC4.
    if (a0 > 0) {
        sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 2);
        for (byte i=0; i < a0; i++) {
            fetchDataFrame();
        }
    }
}

void SIBOSPConnection::_SetAddress4(unsigned long address) {
    byte a0 = address & 0xFF;
    byte a1 = (address >> 8) & 0xFF;
    byte a2 = (address >> 16) & 0xFF;
    byte a3 = ((address >> 24) & 0b00001111) | ((_cur_device << 4) & 0b00110000); // | 0b01000000);

    sendControlFrame(SP_SCTL_WRITE_MULTI_BYTE | 3);

    // Now send the address, least significant byte first
    // Observed: The first two address bytes are always sent, even if 0x00
    sendDataFrame(a0);
    sendDataFrame(a1);
    // Observed: The last two address bytes are only sent if populated
    if (address >> 16 != 0) sendDataFrame(a2);
    if (address >> 24 != 0) sendDataFrame(a3);
}

void SIBOSPConnection::setAddress(unsigned long address) {
    if (_id == SIBO_ID_ASIC4 && !_force_asic5) {
        _SetAddress4(address);
    } else {
        _SetAddress5(address);
    }
    _cur_address = address;
}

void SIBOSPConnection::setDevice(byte device) {
    _cur_device = device;
    setAddress(0);
}


// Getters
byte SIBOSPConnection::getInfoByte() {
    return _infobyte;
}

byte SIBOSPConnection::getSizeCode() {
    return _sizecode;
}

byte SIBOSPConnection::getTotalDevices() {
    return _devices;
}

int SIBOSPConnection::getTotalBlocks() {
    return _blocks;
}

byte SIBOSPConnection::getTypeCode() {
    return _typecode;
}

byte SIBOSPConnection::getASIC4InputRegister() {
    return _asic4_input_register;
}

byte SIBOSPConnection::getClockPin() {
    return _clock_pin;
}

byte SIBOSPConnection::getDataPin() {
    return _data_pin;
}

byte SIBOSPConnection::getDirPin() {
    return _dir_pin;
}

byte SIBOSPConnection::getClockBit() {
    return _clock_bit;
}

byte SIBOSPConnection::getDataBit() {
    return _data_bit;
}

bool SIBOSPConnection::getDirectPinMode() {
    return _direct_pin_mode;
}

// Returns human-readable memory size
// TODO: New method returning the actual size as an integer multiplied by the number of devices
String SIBOSPConnection::getSize() {
  switch (_sizecode) {
    case 1:
      return("32KB");
    case 2:
      return("64KB");
    case 3:
      return("128KB");
    case 4:
      return("256KB");
    case 5:
      return("512KB");
    case 6:
      return("1MB");
    case 7:
      return("2MB");
  }

  return("");
}


String SIBOSPConnection::getType() {
  switch (_typecode) {
    case 0:
      return("RAM");
    case 1:
      return("Type 1 Flash");
    case 2:
      return("Type 2 Flash");
    case 3:
      return("TBS");
    case 4:
      return("TBS");
    case 5:
      return("???");
    case 6:
      return("ROM");
    case 7:
      return("Hardware write-protected SSD");
  }
  return("");
}


// get/setID: Device ID used to communicate with ASIC
// TODO: If this is zero, IDs will be picked in order ID:6, then ID:2
byte SIBOSPConnection::getID() {
    return _id;
}

// void SIBOSPConnection::setID(byte id) {
//     _id = id & 0b00111111;
// }

void SIBOSPConnection::setForceASIC5(bool flag) {
    _force_asic5 = flag;
    Reset();
}

bool SIBOSPConnection::getForceASIC5() {
    return _force_asic5;
}

bool SIBOSPConnection::setDataPin(byte pin) {
    // if (_clock_pin != pin) {
        _data_pin = pin;
        _data_bit = (1 << (_data_pin));
        _DataPinReset();
        return true;
    // }
    // return false;
}

bool SIBOSPConnection::setClockPin(byte pin) {
    // if (_data_pin != pin % 8) {
        _clock_pin = pin;
        _clock_bit = (1 << _clock_pin);
    _ClockPinReset();
        return true;
    // }
    // return false;
}

bool SIBOSPConnection::setClockEnablePin(byte pin) {
    _clock_enable_pin = pin;
    return true;
}

bool SIBOSPConnection::setDirPin(byte pin) {
    // if (_data_pin != pin % 8) {
        _dir_pin = pin;
//        _clock_bit = (1 << _clock_pin);
    _DirPinReset();
        return true;
    // }
    // return false;
}

void SIBOSPConnection::setDirectPinMode(bool flag) {
#if defined(FASTPIN_UNAVAILABLE)
    _direct_pin_mode = false; // Just keep setting it to false
#else
    _direct_pin_mode = flag;
#endif
}


// Reset

void SIBOSPConnection::Reset() {
    _id = 0;

    // _ClockPinReset();
    _DisableClock();
    _DataPinMode(INPUT);

    // sleep(1);

    // Send null frame, then reset frame, then null frame
    // (undocumented: this is what all SIBO machines do)
    sendNullFrame();
    sendControlFrame(0); // Reset frame
    sendNullFrame();


    // Select ASIC4 (ID:6)
    sendControlFrame(SP_SSEL | SIBO_ID_ASIC4);
    _FetchSSDInfo();
    if (_infobyte != 0) { // if it's ASIC4
        _id = SIBO_ID_ASIC4;
        _FetchASIC4ExtraInfo();
        sendASIC4DeviceSizeRegister();  // Send size back to ASIC4, because reasons
        if (_force_asic5) { // Reset, just in case, then ask for ID:2
            sendNullFrame();
            sendControlFrame(0); // Reset frame
            sendNullFrame();
            sendControlFrame(SP_SSEL | SIBO_ID_ASIC5);
            _FetchSSDInfo();
        }
        return;
    }

    // If it's ASIC5
    sendControlFrame(SP_SSEL | SIBO_ID_ASIC5);
    _FetchSSDInfo();
    _id = SIBO_ID_ASIC5;
}

void SIBOSPConnection::sendASIC4DeviceSizeRegister() {
    sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 1);
    sendDataFrame(_sizecode);
}

void SIBOSPConnection::_FetchSSDInfo() {
  _infobyte = fetchDataFrame();

  _typecode = (_infobyte & 0b11100000) >> 5;
  _devices  = ((_infobyte & 0b00011000) >> 3) + 1;
  _sizecode = (_infobyte & 0b00000111);
  _blocks   = (_sizecode == 0) ? 0 : ((0b10000 << _sizecode) * 4);
}

bool SIBOSPConnection::_FetchASIC4ExtraInfo() {
    if (_id == SIBO_ID_ASIC4) {
        sendControlFrame(SP_SCTL_READ_SINGLE_BYTE | 1);
        _asic4_input_register = fetchDataFrame();
        return true;
    } else {
        _ResetASIC4ExtraInfo();
        return false;
    }
}

void SIBOSPConnection::_ResetASIC4ExtraInfo() {
    _asic4_input_register = 0;
}
