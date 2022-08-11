/*
 * libsibo: A collection of libraries for communicating with peripherals compatible with Psion's SIBO range of computers.
 * 
 * It currently only includes code for the SIBO Serial Protocol (SIBO-SP) and dealing with Psion's proprietary flash
 * filesystem, FEFS.
 * 
 *** TEST ROUTINES ***
 * This used to be the main code for the Arduino portion of SIBODUMP, but its use has expanded and will continue to expand.
 * You can still use this with SIBODUMP for ripping Psion SSDs. The next release of SIBODUMP will remove the old Dump.ino
 * file and advise people to use this library instead.
 * 
 */

#include <Arduino.h>

// #if defined(ARDUINO_ARCH_MBED)
//   REDIRECT_STDOUT_TO(Serial)  // MBED  printf(...) to console
// #else
//   #define printf (Serial.printf)
// #endif

//#include "misc.h"

#include "sibo-sp.h"
#include "sibo-fefs.h"


#define BAUDRATE 115200


unsigned int curblock = 0;
byte curdev = 0;
bool is_asic4 = false;
bool direct_pin_mode = false;

SIBOSPConnection sibosp;

// Sets the Port B Mode register (Reg 2) on an ASIC5 to whatever is in "mode".
// If it's run on ASIC4, it just increments the address register by one, no matter what the value of "mode" is.
// TODO: This should be moved to SIBO-SP, assuming it's being kept
void SetASIC5PortBMode(byte mode) {
  sibosp.sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 2);
  sibosp.sendDataFrame(mode & 0x0F);
}


// Dumps the whole peripheral/SSD over Serial
void dump(int _blocks) {
  if (sibosp.getID() == SIBO_ID_ASIC5 || sibosp.getForceASIC5()) SetASIC5PortBMode(0);
  for (int _block = 0; _block < _blocks; _block++) {
    sibosp.setAddress(((unsigned long)_block << 8));
    sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0);
    for (int b = 0; b < 256; b++) {
      Serial.write(sibosp.fetchDataFrame());
    }
  }
}

// Dumps a block of 256 bytes over serial, based on the current block number
void dumpblock(int _block) {
  if (sibosp.getID() == SIBO_ID_ASIC5 || sibosp.getForceASIC5()) SetASIC5PortBMode(0);
  sibosp.setAddress(((unsigned long)_block << 8));
  sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0);
  for (int b = 0; b < 256; b++) {
    Serial.write(sibosp.fetchDataFrame());
  }
}


// Detects the filesystem on the SSD based on a small number of options and sends the result to Serial
// (There are only two types of filesystem on an SSD: FEFS and FAT [FAT12?])
void getSSDFormat() {
  unsigned int first_two_bytes = 0;
  
  sibosp.setDevice(0); // Automatically sets address to 0 as well.
  // sibosp.setAddress(0);
  sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0); // Tell the SSD we want to read some data

  first_two_bytes = sibosp.fetchDataFrame() | (sibosp.fetchDataFrame() << 8);

  if (first_two_bytes == FEFS_TYPE) {
    Serial.println("Found Psion FEFS formatted device.");
    getFEFSTitle(sibosp);
  } else {
    Serial.print("Found this header: ");
    Serial.println(first_two_bytes);
  }
  sibosp.setAddress(0);
}


// Sends basic information about an attached peripheral/SSD over Serial
void printinfo() {
  // long unsigned int blocks;
  char out_str[50];

  Serial.println();

  sprintf(out_str, "Pin Numbers (CLK/DATA/DIR): %d/%d/%d\n", sibosp.getClockPin(), sibosp.getDataPin(), sibosp.getDirPin());
  Serial.print(out_str);

  Serial.print("Direct pin mode: ");
  if (sibosp.getDirectPinMode()) {
    Serial.println("On");

    Serial.print("Pin Bits (CLK/DATA): ");
    Serial.print(sibosp.getClockBit(), BIN);
    Serial.print("/");
    Serial.println(sibosp.getDataBit(), BIN);
  } else {
    Serial.println("Off");
  }
  if (sibosp.getForceASIC5()) {
    Serial.println("ASIC5 mode forced.");
  }

  Serial.println();

  if (sibosp.getSizeCode() == 0) {
    Serial.println("No SSD detected.");
    return;
  }


  if (sibosp.getType() == "") {
    Serial.print("Error with getType(): size code is ");
    Serial.println(sibosp.getTypeCode());
  } else {
    // printf("TYPE: %s\n", sibosp.getType().c_str());
    Serial.print("TYPE: ");
    Serial.println(sibosp.getType().c_str());
  }


  if (sibosp.getSize() == "") {
    // printf("Error with getSize(): size code is %d\n", sibosp.getSizeCode());
    Serial.print("Error with getSize(): size code is ");
    Serial.println(sibosp.getSizeCode());
    // printf("DEVICES: %d\n", sibosp.getTotalDevices());
    Serial.print("DEVICES: ");
    Serial.println(sibosp.getTotalDevices());
  } else {
    // printf("SIZE: %dx%s\n", sibosp.getTotalDevices(), sibosp.getSize().c_str());
    Serial.print("SIZE: ");
    Serial.print(sibosp.getTotalDevices());
    Serial.println(sibosp.getSize().c_str());
  }

  // printf("BLOCKS: %d\n", sibosp.getTotalBlocks());

  switch (sibosp.getID()) {
    case SIBO_ID_ASIC4:
      Serial.print("ASIC4 detected");
      if (sibosp.getForceASIC5()) Serial.print(" (ASIC5 mode forced)");
      Serial.println();
      // printf("Input Register: %d\n", sibosp.getASIC4InputRegister());
      Serial.print("Input Register: ");
      Serial.println(sibosp.getASIC4InputRegister());
      break;

    case SIBO_ID_ASIC5:
      Serial.println("ASIC5 detected");
      break;

    default:
      Serial.println("Unknown controller!");
}

  getSSDFormat();

  // printf("Current dev/block: %d/%d\n", curdev, curblock);
  Serial.print("Current dev/block: ");
  Serial.print(curdev);
  Serial.print("/");
  Serial.println(curblock);

  Serial.println();
}

void Reset() {
  curblock = 0;
  curdev = 0;
  sibosp.Reset();
}


//
// MAIN CODE
//

void setup() {
  Serial.begin(BAUDRATE);

  Reset();
}

void loop() {
  while (Serial.available()) {
    char incomingCharacter = Serial.read();
    switch (incomingCharacter) {
      case '1':
        sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 1);
        // printf("%d\n", sibosp.fetchDataFrame());
        Serial.println(sibosp.fetchDataFrame());
        break;

      case '2':
        sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 2);
        // printf("%d\n", sibosp.fetchDataFrame());
        Serial.print(sibosp.fetchDataFrame());
        break;

      case '4':
        sibosp.setForceASIC5(false);
        // Serial.print("ID:6");
        break;

      case '5':
        sibosp.setForceASIC5(true);
        // Serial.print("ID:2");
        break;

      case 'a':
      case 'A':
        Serial.write(sibosp.getID() == SIBO_ID_ASIC4 ? 4 : 5);
        break;

      case 'b':
      case 'B':
        Serial.write(sibosp.getInfoByte());
        break;
 
      case 'd':
      case 'D':
        dump(sibosp.getTotalBlocks());
        break;

      case 'i':
      case 'I':
        printinfo();
        break;

      case 'f':
        dumpblock(curblock);
        break;

      // case 'j': // jump to an address
      //   getstartaddress();
      //   break;
        
      case 'n':
        curblock = (curblock + 1) % (sibosp.getTotalBlocks());
        break;

      case 'N':
        curblock = 0;
        curdev = (curdev + 1) % (sibosp.getTotalDevices());
        sibosp.setDevice(curdev);
        break;

      case 'p':
        sibosp.setDirectPinMode(false);
        break;

      case 'P':
        sibosp.setDirectPinMode(true);
        break;

      case 'r':
        curblock = 0;
        break;

      case 'R':
        Reset();
        break;
      
      case 13:
        Serial.println();
        break;
    }
  }
}
