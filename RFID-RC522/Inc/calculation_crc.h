#ifndef CALC_CRC_A_N_B
#define CALC_CRC_A_N_B

#define CRC_A 1
#define CRC_B 2

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CRC_A 1
#define CRC_B 2

unsigned short UpdateCrc(unsigned char ch, unsigned short *lpwCrc);
void ComputeCrc(uint8_t CRCType, uint8_t *Data, uint8_t Length, uint8_t *TransmitFirst, uint8_t *TransmitSecond);

#endif
