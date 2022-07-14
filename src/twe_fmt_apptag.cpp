/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_apptag.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;

/// <summary>
/// parse the AppTAG packet.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
E_PKT TwePacketAppTAG::parse(uint8_t* pb, uint16_t u16len) {
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :80000000B10001810043C10032C9047C02AF0A41D2
	//  ^^^^^^^1^2^^^3^^^^^^^4^5^6^7^^^8^^^9^^^a^b

	uint8_t* p = pb;
	uint8_t* e = pb + u16len;
	uint8_t c = 0;
	bool bChecksumErr = false;

	DataAppTAG::payload.clear();
	TwePacket::common.clear();

	// at least 13 bytes are necessary
	if (u16len < 13) {
		return E_PKT::PKT_ERROR;
	}

	// the first 4 bytes: ser# of router or 0x80000000(direct)
	DataAppTAG::u32addr_rpt = G_DWORD(p);

	// LQI value
	DataAppTAG::u8lqi = G_OCTET(p);

	// sequence number
	DataAppTAG::u16seq = G_WORD(p);

	// src address (32bit)
	DataAppTAG::u32addr_src = G_DWORD(p);

	// src address (8bit)
	DataAppTAG::u8addr_src = G_OCTET(p);

	// 0x80 fixed
	DataAppTAG::u8sns = G_OCTET(p);
	if (c == 0x80) {
		return E_PKT::PKT_ERROR;
	}

	// volt
	DataAppTAG::u16Volt = DecodeVolt(G_OCTET(p));

	// rest of bytes
	if (p < e) {
		DataAppTAG::payload.reserve_and_set_empty(int(e - p));
		while (p != e) DataAppTAG::payload.push_back(*p++);
	}

	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppTAG::u32addr_src;
	TwePacket::common.src_lid = DataAppTAG::u8addr_src;
	TwePacket::common.lqi = DataAppTAG::u8lqi;
	TwePacket::common.volt = DataAppTAG::u16Volt;

	// return packet ID
	return E_PKT::PKT_APPTAG;
}


