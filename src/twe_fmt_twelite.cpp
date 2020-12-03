/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_twelite.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;

/**
 * @fn	E_PKT TwePacketTwelite::parse(uint8_t* pyld, uint8_t u16len)
 *
 * @brief	Parses App_Twelite 0x81 packet content
 *
 *
 * @param [in,out]	pyld 	pointer to the byte array
 * @param 		  	u16len	The length of byte array
 *
 * @returns	An E_PKT.
 */
E_PKT TwePacketTwelite::parse(uint8_t* pyld, uint16_t u16len) {
	bool bValid = false;
	TwePacket::common.clear();

	// length check
	if (u16len != 23) {
		return E_PKT::PKT_ERROR;
	}

	// misc vars
	int c;

	// read
	u8addr_src = G_OCTET(pyld); 					// addr src (LID)

	c = G_OCTET(pyld); // 0x81

	c = G_OCTET(pyld); // packet identified (need AppID to calculate)

	c = G_OCTET(pyld); // protocol version

	u8lqi = G_OCTET(pyld); 							// LQI

	u32addr_src = G_DWORD(pyld); 					// SerID

	u8addr_dst = G_OCTET(pyld); 					// addr dst (LID)

	c = G_WORD(pyld);								// timestamp&low latency flag
	u16timestamp = c & 0x7FFF;								// -time stamp
	b_lowlatency_tx = ((c & 0x8000) == 0x8000);				// -low latency flag

	u8rpt_cnt = G_OCTET(pyld);					 	// repeat count

	u16Volt = (short)G_WORD(pyld);					// module voltage

	c = G_OCTET(pyld);								// unused

	c = G_OCTET(pyld);								// DI state bit
	DI1 = ((c & 0x01) == 0x01);
	DI2 = ((c & 0x02) == 0x02);
	DI3 = ((c & 0x04) == 0x04);
	DI4 = ((c & 0x08) == 0x08);
	DI_mask = c;

	c = G_OCTET(pyld);								// DI active state bit
	DI1_active = ((c & 0x01) == 0x01);
	DI2_active = ((c & 0x02) == 0x02);
	DI3_active = ((c & 0x04) == 0x04);
	DI4_active = ((c & 0x08) == 0x08);
	DI_active_mask = c;

	Adc_active_mask = 0;									// ADC
	u16Adc1 = G_OCTET(pyld);
	if (u16Adc1 != 0xFF) Adc_active_mask |= 1; else u16Adc1 = 0xFFFF;
	u16Adc2 = G_OCTET(pyld);
	if (u16Adc2 != 0xFF) Adc_active_mask |= 2; else u16Adc2 = 0xFFFF;
	u16Adc3 = G_OCTET(pyld);
	if (u16Adc3 != 0xFF) Adc_active_mask |= 4; else u16Adc3 = 0xFFFF;
	u16Adc4 = G_OCTET(pyld);
	if (u16Adc4 != 0xFF) Adc_active_mask |= 8; else u16Adc4 = 0xFFFF;
	c = G_OCTET(pyld);

	if (u16Adc1 != 0xFFFF) u16Adc1 = ((u16Adc1 * 4 + ((c >> 0) & 0x3)) * 4); // additional two bits
	if (u16Adc2 != 0xFFFF) u16Adc2 = ((u16Adc2 * 4 + ((c >> 2) & 0x3)) * 4);
	if (u16Adc3 != 0xFFFF) u16Adc3 = ((u16Adc3 * 4 + ((c >> 4) & 0x3)) * 4);
	if (u16Adc4 != 0xFFFF) u16Adc4 = ((u16Adc4 * 4 + ((c >> 6) & 0x3)) * 4);

	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataTwelite::u32addr_src;
	TwePacket::common.src_lid = DataTwelite::u8addr_src;
	TwePacket::common.lqi = DataTwelite::u8lqi;
	TwePacket::common.volt = DataTwelite::u16Volt;

	bValid = true;
	return bValid ? E_PKT::PKT_TWELITE : E_PKT::PKT_ERROR;
}