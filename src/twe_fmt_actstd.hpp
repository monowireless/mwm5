#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /*****************************************************
  * Act standard format
  *
  * NOTE: Act standard format is based on AppUART.
  *****************************************************/

#include "twe_fmt_common.hpp"
#include "twe_fmt_appuart.hpp"

namespace TWEFMT {
	/*****************************************************
     * Act standard packet structure
     *****************************************************/
	class TwePacketActStd : public TwePacketAppUART {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_ACT_STD;

		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
			// Act
			//   0 1 2 3 4 5 6 7 8 9 a b c d e f
			// :FEAA008201015A00000000B7000F424154310F0CEE000B03FF03FF03FF92
			//  ^1^2^3^^^^^^^4^^^^^^^5^6^^^7^^^^^^^^^^^^^^^^^^^^^^^^^^^^^8^9
			//
			//  *1 dst adderss(LID)
			//  *2 0xAA (fixed)
			//  *4 src address
			//  *5 dst address
			//  *6 LQI
			//  *7 byte count of data part (000F=15bytes)
			//  *8 data(0x0c 0x42 0x41 ... 0xff)
			if (   u16len > 13
				&& p[1] == 0xAA
				&& (p[3] & 0x80) == 0x80
				&& ((p[12] * 256 + p[13]) == u16len - 14) // length
				) {
				return _pkt_id;
			}

			return E_PKT::PKT_ERROR;

		}

		TwePacketActStd() : TwePacketAppUART(this->_pkt_id) { }
		~TwePacketActStd() { }
	};


	static inline TwePacketActStd& refTwePacketActStd(spTwePacket& p) {
		return refTwePacketGen<TwePacketActStd>(p);
	}
}