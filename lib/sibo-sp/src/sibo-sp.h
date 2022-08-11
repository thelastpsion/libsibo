// TODO: Refactor pins setting routines so that changes on CLK and DATA happen at the same time (not essential, but keeps the signal clean)


#ifndef _SIBOSP_H
#define _SIBOSP_H

#include <Arduino.h>
#include <unistd.h> 

#if defined(ARDUINO_ARCH_MBED_RP2040) || defined(ARDUINO_ARCH_RP2040) || defined(RASPBERRYPI_PICO)
    #define RP2040
#elif defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_MEGA)
    #define FASTPIN_PORTD
// #elif defined(ARDUINO_ARCH_MBED_RP2040) || defined(ARDUINO_ARCH_RP2040)
//     #define FASTPIN_PIO
#else
    #define FASTPIN_UNAVAILABLE
#endif

#define SP_SCTL_READ_MULTI_BYTE   0b11010000
#define SP_SCTL_READ_SINGLE_BYTE  0b11000000
#define SP_SCTL_WRITE_MULTI_BYTE  0b10010000
#define SP_SCTL_WRITE_SINGLE_BYTE 0b10000000
#define SP_SSEL                   0b01000000

#define SIBO_ID_ASIC4 6
#define SIBO_ID_ASIC5 2

#define IS_DATA_INPUT_FRAME true

#define UPAUSE 0



class SIBOSPConnection {
    public:
        SIBOSPConnection();

        void setForceASIC5(bool flag);
        bool getForceASIC5();
        void setForcePinMode(bool flag);

        byte getID();
        byte getInfoByte();
        byte getTypeCode();
        byte getTotalDevices();
        int  getCurrentDevice();
        byte getSizeCode();
        int  getTotalBlocks();
        int  getCurrentBlock();

        // Human-readable interpretation of values detected on device
        String getSize();
        String getType();

        bool setClockPin(byte pin);
        bool setDataPin(byte pin);
        bool setDirPin(byte pin);
        bool setClockEnablePin(byte pin);
        void setDirectPinMode(bool flag);
        byte getClockPin();
        byte getDataPin();
        byte getDirPin();
        byte getClockBit();
        byte getDataBit();
        bool getDirectPinMode();

        // Physical frames
        void sendControlFrame(byte data);
        void sendDataFrame(byte data);
        byte fetchDataFrame();

        void sendNullFrame();
        void deselectASIC();

        // Control Commands
        void setAddress(unsigned long);
        void useLastSetAddress();

        void setDevice(byte);

        byte getASIC4InputRegister();
        void sendASIC4DeviceSizeRegister();

        void Reset();

    private:
        // Flags
        bool _force_asic5;
        bool _direct_pin_mode;

        // Device Info
        byte _id;
        byte _infobyte;
        byte _typecode;
        byte _devices;
        byte _sizecode;
        int  _blocks;
        byte _asic4inputreg;
        byte _asic4devsizereg;

        byte _clock_pin;
        byte _data_pin;
        byte _dir_pin;
        byte _clock_enable_pin;
        byte _clock_bit;
        byte _data_bit;

        // Address
        unsigned long _cur_address;
        unsigned long _last_setAddress;
        byte _cur_device;

        // Frame headers and footers (idle bit)
        void _SendDataHeader(bool is_input_frame = false);
        // void _SendControlHeader(); // Now rolled into sendControlFrame()
        // void _SendIdleBit(bool is_input_frame = false); // Redundant after removing _SendControlHeader() and tidying _SendDataHeader()
        void _SendPayload(byte data);

        // Pin flipping
        void _DataPinWrite(byte val);
        void _DataPinMode(byte mode);
        void _DataPinReset();
        void _ClockPinReset();
        void _ClockCycle(byte cycles);
        void _DirPinReset();
        void _EnableClock();
        void _DisableClock();
        void _SendDataBitFast(bool val);

        void _FetchSSDInfo();

        // Address management
        void _SetAddress4(unsigned long);
        void _SetAddress5(unsigned long);
        
        // ASIC4 specific registers
        byte _asic4_input_register;
        // byte _asic4_device_size_register;
        bool _FetchASIC4ExtraInfo();
        void _ResetASIC4ExtraInfo();
};

#endif