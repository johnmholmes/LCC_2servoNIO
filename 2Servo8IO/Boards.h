/*
 Board definitions if you alter the number servo or IO pins change below#
 Max servo 8 max IO 16 but only test with 8 so change at own risk

*/

#if defined(ESP32_BOARD)
  #define BOARD "ESP32"
  #define SERVOPINS 32, 33
  #define IOPINS    14,27,26,25,16,17,18,19
  #define CAN_TX_PIN (gpio_num_t) 2
  #define CAN_RX_PIN (gpio_num_t) 15
  #ifndef USEGCSERIAL
    #include "ACAN_ESP32Can.h"
  #endif // USEGCSERIAL
  
  #define EEPROMSIZE 4096
  #define EEPROMbegin { EEPROM.begin(EEPROMSIZE); dP("\nEEPROM begin "); dP(EEPROMSIZE)
  #define EEPROMcommit { EEPROM.commit(); dP("EEPROM COMMIT"); }

#elif defined(ATOM_BOARD)
  #define BOARD "Atom"
  #define NUM_SERVOS 2
  #define SERVOPINS 25, 21
  #define NUM_IO    4
  #define IOPINS    22, 19, 23, 33
  #define CAN_TX_PIN (gpio_num_t)26
  #define CAN_RX_PIN (gpio_num_t)32
  #ifndef USEGCSERIAL
    #include "ACAN_ESP32Can.h"
  #endif // USEGCSERIAL
  #include <ESP32Servo.h>
  #define EEPROMSIZE 4096
  #define EEPROMbegin { EEPROM.begin(EEPROMSIZE); dP("\nEEPROM begin "); dP(EEPROMSIZE)
  #define EEPROMcommit { EEPROM.commit(); dP("EEPROM COMMIT"); }

#else
  #error "No board selected"

#endif
