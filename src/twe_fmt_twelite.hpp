#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/*****************************************************
 * APP TWELITE 0x81 command
 *****************************************************/

#include "twe_fmt_common.hpp"

namespace TWEFMT {
	struct DataTwelite {
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
		 * true: DI1 is activated (set as Lo),
		 */
		uint8_t DI1;

		/**
		 * true: DI1 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI1_active;

		/**
		 * true: DI2 is activated (set as Lo)
		 */
		uint8_t DI2;

		/**
		 * true: DI2 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI2_active;

		/**
		 * true: DI3 is activated (set as Lo)
		 */
		uint8_t DI3;

		/**
		 * true: DI3 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI3_active;

		/**
		 * true: DI4 is activated (set as Lo)
		 */
		uint8_t DI4;

		/**
		 * true: DI4 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI4_active;

		/**
		 * DI state mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * Note: same values as DI?.
		 */
		uint8_t DI_mask;

		/**
		 * DI active mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * Note: same values as DI?_active.
		 */
		uint8_t DI_active_mask;

		/**
		 * module voltage in mV
		 */
		uint16_t u16Volt;

		/**
		 * ADC1 value in mV
		 */
		uint16_t u16Adc1;

		/**
		 * ADC2 value in mV
		 */
		uint16_t u16Adc2;

		/**
		 * ADC3 value in mV
		 */
		uint16_t u16Adc3;

		/**
		 * ADC4 value in mV
		 */
		uint16_t u16Adc4;

		/**
		 * if bit set, Adc has value (LSB: ADC1, bit2: ADC2, ...),
		 * otherwise Adc is connected to Vcc voltage.
		 * (note: Setting Adc as Vcc level means unused to App_Twelite firmware,
		 *        even the hardware can measure up to 2.47V)
		 */
		uint8_t Adc_active_mask;
	};

	// the data class
	class TwePacketTwelite : public TwePacket, public DataTwelite {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_TWELITE;

		TwePacketTwelite() : TwePacket(_pkt_id), DataTwelite({ 0 }) { }
		~TwePacketTwelite() { }
		E_PKT parse(uint8_t* p, uint16_t len);

		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
			//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 4 5 6 -
			// :7881150175810000380026C9000C04220000FFFFFFFFFFA7
			if (u16len == 23
				&& p[1] == 0x81 			// 0x81 command
				&& p[3] == 0x01 			// protocol version
				&& (p[5] & 0x80) == 0x80 	// SID: MSB must be set
				) {
				return _pkt_id;
			}

			return E_PKT::PKT_ERROR;
		}
	};

	// reference to the object
	static inline TwePacketTwelite& refTwePacketTwelite(spTwePacket& p) {
		return refTwePacketGen<TwePacketTwelite>(p);
	}
}