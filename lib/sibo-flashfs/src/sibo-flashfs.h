#ifndef SIBO_FLASHFS
#define SIBO_FLASHFS

#include <Arduino.h>
#include <time.h>
#ifndef ARDUINO_AVR_UNO
  #include <sys/utime.h>
#endif
#include <sibo-sp.h>

#include <Arduino.h>
// #if defined(ARDUINO_ARCH_MBED)
//   REDIRECT_STDOUT_TO(Serial)  // MBED  printf(...) to console
// #else
//   #define printf (Serial.printf)
// #endif


#define FLASH_TYPE   0xf1a5
#define IMAGE_ISROM  0xffffffff
#define NULL_PTR     0xffffff

#define IMAGE_ROOTPTR_OFFSET        11
#define IMAGE_ROOTPTR_LENGTH        3
#define IMAGE_NAME_OFFSET           14
#define IMAGE_NAME_LENGTH           11
#define IMAGE_FLASHCOUNT_OFFSET     25
#define IMAGE_FLASHCOUNT_LENGTH     4
#define IMAGE_FLASHIDSTRING_OFFSET  33
#define IMAGE_ROMIDSTRING_OFFSET    29

#define ENTRY_NEXTENTRYPTR_OFFSET         0
#define ENTRY_NEXTENTRYPTR_LENGTH         3
#define ENTRY_NAME_OFFSET                 3
#define ENTRY_NAME_LENGTH                 8
#define ENTRY_EXT_OFFSET                  11
#define ENTRY_EXT_LENGTH                  3
#define ENTRY_FLAGS_OFFSET                14
#define ENTRY_FLAGS_LENGTH                1
#define ENTRY_FIRSTENTRYRECORDPTR_OFFSET  15
#define ENTRY_FIRSTENTRYRECORDPTR_LENGTH  3
#define ENTRY_ALTRECORDPTR_OFFSET         18
#define ENTRY_ALTRECORDPTR_LENGTH         3
#define ENTRY_PROPERTIES_OFFSET           21
#define ENTRY_PROPERTIES_LENGTH           1
#define ENTRY_TIMECODE_OFFSET             22
#define ENTRY_TIMECODE_LENGTH             2
#define ENTRY_DATECODE_OFFSET             24
#define ENTRY_DATECODE_LENGTH             2
#define ENTRY_FIRSTDATARECORDPTR_OFFSET   26
#define ENTRY_FIRSTDATARECORDPTR_LENGTH   3
#define ENTRY_FIRSTDATALEN_OFFSET         29
#define ENTRY_FIRSTDATALEN_LENGTH         2

#define ENTRY_FLAG_ENTRYISVALID               1 << 0
#define ENTRY_FLAG_PROPERTIESDATETIMEISVALID  1 << 1
#define ENTRY_FLAG_ISFILE                     1 << 2
#define ENTRY_FLAG_NOENTRYRECORD              1 << 3
#define ENTRY_FLAG_NOALTRECORD                1 << 4
#define ENTRY_FLAG_ISLASTENTRY                1 << 5
#define ENTRY_FLAG_BIT6                       1 << 6
#define ENTRY_FLAG_BIT7                       1 << 7

#define ENTRY_PROPERTY_ISREADONLY    1 << 0
#define ENTRY_PROPERTY_ISHIDDEN      1 << 1
#define ENTRY_PROPERTY_ISSYSTEM      1 << 2
#define ENTRY_PROPERTY_ISVOLUMENAME  1 << 3
#define ENTRY_PROPERTY_ISDIRECTORY   1 << 4
#define ENTRY_PROPERTY_ISMODIFIED    1 << 5

#define FILE_FLAGS_OFFSET            0
#define FILE_FLAGS_LENGTH            1
#define FILE_NEXTRECORDPTR_OFFSET    1
#define FILE_NEXTRECORDPTR_LENGTH    3
#define FILE_ALTRECORDPTR_OFFSET     4
#define FILE_ALTRECORDPTR_LENGTH     3
#define FILE_DATARECORDPTR_OFFSET    7
#define FILE_DATARECORDPTR_LENGTH    3
#define FILE_DATARECORDLEN_OFFSET    10
#define FILE_DATARECORDLEN_LENGTH    2
#define FILE_ENTRYPROPERTIES_OFFSET  12
#define FILE_ENTRYPROPERTIES_LENGTH  1
#define FILE_TIMECODE_OFFSET         13
#define FILE_TIMECODE_LENGTH         2
#define FILE_DATECODE_OFFSET         15
#define FILE_DATECODE_LENGTH         2

struct PsiDateTime {
    uint16_t psi_time;
    uint16_t psi_date;
};

#endif

char *rtrim(char *s);
// char *rtrim(char *s) {
// 	char *p = s + strlen(s) - 1;
// 	const char *end = p;
// 	while (p >= s && isspace((unsigned char) *p)) {
// 		p--;
// 	}
// 	if (p != end) {
//         p++;
// 		*p = '\0';
//     }
// 	return s;
// }

void getFlashTitle(SIBOSPConnection &sibosp);
void walkpath(int pos, char path[], char *buffer[], const char img_name[], const long buffer_len);
