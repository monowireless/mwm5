/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_appuart.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;


E_PKT TwePacketAppUART::parse(uint8_t* pyld, uint16_t u16len) {
	// App UART (extended format)
	//   0 1 2 3 4 5 6 7 8 9 a b c d  
	// :78A01286300001FFFFFFFFD80003123456??
	//      ^1^2      ^3      ^4^5  ^6
	//  *1 response id 12
	//  *2 src address (86300001)
	//  *3 dst address (FFFFFFFF no info)
	//  *4 LQI
	//  *5 byte count of data part (0003=3bytes)
	//  *6 data (0x12 0x34 0x56)
	uint8_t* b = pyld;
	uint8_t* e = pyld + u16len;

	bool bValid = false;

	// init members
	DataAppUART::payload.clear();
	TwePacket::common.clear();

	// misc vars
	int c;

	// read
	DataAppUART::u8addr_src = G_OCTET(pyld); 					// addr src

	c = G_OCTET(pyld); // 0xA0
	if (!(c == 0xA0 || c == 0xAA)) return E_PKT::PKT_ERROR;

	DataAppUART::u8response_id = G_OCTET(pyld);

	DataAppUART::u32addr_src = G_DWORD(pyld); 					// SerID
	DataAppUART::u32addr_dst = G_DWORD(pyld); 					// SerID

	DataAppUART::u8lqi = G_OCTET(pyld); 							// LQI

	DataAppUART::u16paylen = G_WORD(pyld);

	if (pyld + DataAppUART::u16paylen != e) {
		return E_PKT::PKT_ERROR;
	}
	while (pyld != e) DataAppUART::payload.push_back(*pyld++);

	TwePacket::common.clear();
	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppUART::u32addr_src;
	TwePacket::common.src_lid = DataAppUART::u8addr_src;
	TwePacket::common.lqi = DataAppUART::u8lqi;

	bValid = true;
	return bValid ? E_PKT::PKT_APPUART : E_PKT::PKT_ERROR;
}
