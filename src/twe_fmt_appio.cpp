/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_appio.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;

/**
 * @fn	E_PKT TwePacketAppIO::parse(uint8_t* pyld, uint8_t u16len)
 *
 * @brief	Parses TwePacketAppIO 0x81 packet content
 *
 *
 * @param [in,out]	pyld 	pointer to the byte array
 * @param 		  	u16len	The length of byte array
 *
 * @returns	An E_PKT.
 */
E_PKT TwePacketAppIO::parse(uint8_t* pyld, uint16_t u16len) {
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :01810F01DB8630000200645F000040004F004000??
	// :78811202848201015A003FC9000001000100010086
	//    **--**LqAddr_srcAdTimsRpIostIoenIoit--Cs

	bool bValid = false;
	TwePacket::common.clear();

	// length check
	if (u16len != 20) {
		return E_PKT::PKT_ERROR;
	}

	// misc vars
	int c;

	// read
	u8addr_src = G_OCTET(pyld); 					// addr src

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

	DI_mask = G_WORD(pyld);
	DI_active_mask = G_WORD(pyld);
	DI_int_mask = G_WORD(pyld);

	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppIO::u32addr_src;
	TwePacket::common.src_lid = DataAppIO::u8addr_src;
	TwePacket::common.lqi = DataAppIO::u8lqi;

	bValid = true;
	return bValid ? E_PKT::PKT_APPIO : E_PKT::PKT_ERROR;
}