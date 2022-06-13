/************************************
 * CCITT-8 CRC Function
 * Author: Rob Magee
 * Copyright 2007 CompuGlobalHyperMegaNet LLC
 * Use this code at your own risk.
 * CRC.cs
 * **********************************/
#include "twe_common.hpp"

namespace TWEUTILS {
	uint8_t CRC8_u8Calc(uint8_t *pu8Data, size_t size);
    uint32_t CRC32_u32Calc(uint8_t* pu8Data, size_t size);
	uint8_t CRC8_u8CalcU32(uint32_t u32c);
	uint8_t XOR_u8Calc(uint8_t *pu8Data, size_t size);
	uint8_t LRC_u8Calc(uint8_t* pu8Data, size_t size);
}

