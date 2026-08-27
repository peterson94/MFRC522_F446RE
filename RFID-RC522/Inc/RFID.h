#ifndef RFID_H
#define RFID_H

#include "stm32f4xx_hal.h"
#include "MFRC522_STM32.h"
//#include <stdint.h>
//#include <stdio.h>

// Prototypes
uint8_t RFID_Detect(MFRC522_t *rfID);
uint8_t RFID_Remove(MFRC522_t *rfID);
uint8_t RFID_UID_Read(MFRC522_t *rfID, uint8_t *uid, volatile uint8_t *MENU);
uint8_t RFID_FullRead(MFRC522_t *rfID, uint8_t *uid);
uint8_t RFID_FullErase(MFRC522_t *rfID, uint8_t *uid);
uint8_t RFID_ReadBlock(MFRC522_t *rfID, uint8_t *uid, uint8_t MENU);
uint8_t RFID_WriteBlock(MFRC522_t *rfID, uint8_t *uid, uint8_t MENU);

#endif
