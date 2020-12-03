#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_common.hpp"

namespace TWEFMT {
	/*****************************************************
	 * APP IO 0x81 command
	 *****************************************************/
	struct DataAppIO {
		/**
		 * source address (Serial ID)
		 */
		uint32_t u32addr_src;

		/**
		 * source address (logical ID)
		 */
		uint8_t u8addr_src;

		/**
		 * destination address (logical ID)
		 */
		uint8_t u8addr_dst;

		/**
		 * sequence counter
		 */
		uint16_t u16timestamp;

		/**
		 * true when trying to low latency transmit (same packets will come)
		 */
		uint8_t b_lowlatency_tx;

		/**
		 * packet repeat count
		 *   e.g.) if set 1, the packet passed to one repeater (router) to the destination.
		 */
		uint8_t u8rpt_cnt;

		/**
		 * LQI value
		 */
		uint8_t u8lqi;

		/**
		 * DI state mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 */
		uint16_t DI_mask;

		/**
		 * DI active mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * note: if not changed from power/reset,
		 */
		uint16_t DI_active_mask;

		/**
		 * DI interrupt mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * note: if the change is caused by an I/O interrupt, set the corresponding bit.
		 */
		uint16_t DI_int_mask;
	};

	class TwePacketAppIO : public TwePacket, public DataAppIO {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_APPIO;

		TwePacketAppIO() : TwePacket(_pkt_id), DataAppIO({ 0 }) { }
		~TwePacketAppIO() { }
		E_PKT parse(uint8_t* p, uint16_t len);

		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
			//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
			// :01810F01DB8630000200645F000040004F004000??
			// :78811202848201015A003FC9000001000100010086
			//    **--**LqAddr_srcAdTimsRpIostIoenIoit--Cs
			if (   u16len == 20
				&& p[1] == 0x81				// 0x81 command
				&& p[3] == 0x02				// protocol version
				&& (p[5] & 0x80) == 0x80 	// SID: MSB must be set
				) {			// the difference from App_Twelite is the length 
				return _pkt_id;
			}

			return E_PKT::PKT_ERROR;
		}
	};

	static inline TwePacketAppIO& refTwePacketAppIO(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppIO>(p);
	}
}