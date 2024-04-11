#ifndef __FPM383C_H
#define __FPM383C_H

#include "stdint.h"

void FPM383C_Init(void);
void FPM383C_SendData(int length, uint8_t buffer[]);
void FPM383C_Sleep(void);

uint8_t FPM383C_GetImage(uint32_t timeout);
uint8_t FPM383C_GenChar(uint32_t timeout);
uint8_t FPM383C_Search(uint32_t timeout);
uint8_t FPM383C_Empty(uint32_t timeout);
uint8_t FPM383C_Delete(uint16_t pageID, uint32_t timeout);
uint8_t FPM383C_ControlLED(uint8_t PS_ControlLEDBuf[], uint32_t timeout);
void FPM383C_Identify(void);
void FPM383C_Enroll(uint16_t pageID, uint16_t timeout);

#endif
