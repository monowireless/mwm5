#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/*****************************************************
 * APP UART command (extended format only)
 * 
 * NOTE: Act standard format is based on AppUART.
 *****************************************************/

#include "twe_fmt_common.hpp"


namespace TWEFMT {
	/*****************************************************
	 * APP UART PAYLOAD
	 *****************************************************/
	struct DataAppUART {
		/**
		 * source address (Serial ID)
		 */
		uint32_t u32addr_src;

		/**
		 * source address (Serial ID)
		 */
		uint32_t u32addr_dst;

		/**
		 * source address (logical ID)
		 */
		uint8_t u8addr_src;

		/**
		 * destination address (logical ID)
		 */
		uint8_t u8addr_dst;

		/**
		 * LQI value
		 */
		uint8_t u8lqi;

		/**
		 * Response ID
		 */
		uint8_t u8response_id;

		/**
		 * Payload length
		 */
		uint16_t u16paylen;

		/**
		 * payload
		 */
		TWEUTILS::SmplBuf_Byte payload;
	};

	class TwePacketAppUART : public TwePacket, public DataAppUART {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_APPUART;

		TwePacketAppUART(E_PKT pktid = E_PKT::PKT_APPUART) : TwePacket(pktid), DataAppUART({ 0 }) { }
		~TwePacketAppUART() { }
		E_PKT parse(uint8_t* p, uint16_t len);

		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
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
			if (   u16len > 13
				&& p[1] == 0xA0
				&& (p[3] & 0x80) == 0x80
				&& ((p[12] * 256 + p[13]) == u16len - 14) // length
				) {
				return _pkt_id;
			}

			return E_PKT::PKT_ERROR;
		}
	};

	static inline TwePacketAppUART& refTwePacketAppUART(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppUART>(p);
	}
}
