#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /*****************************************************
  * App tag format
  * Note: parsing detailed sensor information has not 
  *       been implemented.
  *****************************************************/

#include "twe_fmt_common.hpp"

namespace TWEFMT {
	/*****************************************************
	 * APP TAG PAYLOAD (very limited support)
	 *   - sensor data is not parsed, but store raw data
	 *     into 'payload'
	 *****************************************************/
	struct DataAppTAG {
		//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
		// :80000000B10001810043C10032C9047C02AF0A41D2
		//  ^^^^^^^1^2^^^3^^^^^^^4^5^6^7^^^8^^^9^^^a^b

		uint32_t u32addr_rpt;
		uint32_t u32addr_src;

		uint16_t u16seq;
		uint16_t u16Volt;

		uint8_t u8lqi;
		uint8_t u8addr_src;

		uint8_t u8sns;
		TWEUTILS::SmplBuf_Byte payload;
	};

	class TwePacketAppTAG : public TwePacket, public DataAppTAG {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_APPTAG;

		TwePacketAppTAG() : TwePacket(_pkt_id), DataAppTAG({ 0 }) { }
		~TwePacketAppTAG() { }
		E_PKT parse(uint8_t* p, uint16_t len);


		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
			// TWELITE TAG
			//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
			// :80000000B10001810043C10032C9047C02AF0A41D2
			//  ^^^^^^^1^2^^^3^^^^^^^4^5^6^7^^^8^^^9^^^a^b
			if (u16len > 14 // at senser data count
				&& p[0] & 0x80 // 
				&& p[7] & 0x80
				&& p[12] != 0x80
				) {
				return _pkt_id;
			}

			return E_PKT::PKT_ERROR;
		}

	};


	static inline TwePacketAppTAG& refTwePacketAppTAG(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppTAG>(p);
	}
}