/************************************
 * CCITT-8 CRC Function
 * Author: Rob Magee
 * Copyright 2007 CompuGlobalHyperMegaNet LLC
 * Use this code at your own risk.
 * CRC.cs
 * **********************************/
#include "twecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8 TWE_CRC8_u8Calc(uint8* pu8Data, uint8 size);
uint8 TWE_CRC8_u8CalcU32(uint32 u32c);

uint8 TWE_XOR_u8Calc(uint8* pu8Data, uint8 size);

#ifdef __cplusplus
}
#endif
