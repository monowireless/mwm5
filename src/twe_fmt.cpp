/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>
#include "twe_fmt.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;

/**
 * @fn	E_PKT TWEFMT::identify_packet_type(uint8_t* p, uint8_t u16len)
 *
 * @brief	Identify packet type
 *
 * @param [in,out]	p	 	If non-null, a pointer to payload of byte array.
 * @param 		  	u16len	the length of 'p'
 *
 * @returns	An packet id, E_PKT::PKT_ERROR in case of an error.
 */
E_PKT TWEFMT::identify_packet_type(uint8_t* p, uint16_t u16len) {
	uint8_t* e = p + u16len;
	E_PKT type = E_PKT::PKT_ERROR;

	// TWELITE PAL
	type = TwePacketPal::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;

	// App Twelite 0x81 command
	type = TwePacketTwelite::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;

	// App IO 0x81
	type = TwePacketAppIO::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;
	
	// App UART
	type = TwePacketAppUART::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;

	// Act standard
	type = TwePacketActStd::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;

	// Act TAG
	type = TwePacketAppTAG::identify(p, e, u16len);
	if (type != E_PKT::PKT_ERROR) return type;

	// UNKNOWN PKT
	return E_PKT::PKT_ERROR;
}

template <class T>
inline spTwePacket _newTwePacket_parse(uint8_t* p, uint16_t u16len) {
	auto sp = spTwePacket(new T());
	sp->parse(p, u16len);
	return sp;
}

/**
 * @fn	spTwePacket TWEFMT::newTwePacket(uint8_t* p, uint16_t u16len, E_PKT eType = E_PKT::PKT_ERROR)
 *
 * @brief	Creates a new twe packet
 *
 * @param [in,out]	p	 	If non-null, an uint8_t to process.
 * @param 		  	u16len	The length.
 * @param 		  	eType	(Optional) The type.
 *
 * @returns	A spTwePacket.
 */
spTwePacket TWEFMT::newTwePacket(uint8_t* p, uint16_t u16len, E_PKT eType) {
	if (eType == E_PKT::PKT_ERROR) {
		eType = identify_packet_type(p, u16len);
	}

	switch (eType) {
	case E_PKT::PKT_PAL:
		return _newTwePacket_parse<TwePacketPal>(p, u16len);

	case E_PKT::PKT_TWELITE:
		return _newTwePacket_parse<TwePacketTwelite>(p, u16len);

	case E_PKT::PKT_APPIO:
		return _newTwePacket_parse<TwePacketAppIO>(p, u16len);

	case E_PKT::PKT_APPUART:
		return _newTwePacket_parse<TwePacketAppUART>(p, u16len);

	case E_PKT::PKT_ACT_STD:
		return _newTwePacket_parse<TwePacketActStd>(p, u16len);

	case E_PKT::PKT_APPTAG:
		return _newTwePacket_parse<TwePacketAppTAG>(p, u16len);

	default:
		return spTwePacket();
	}
}

