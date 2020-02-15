#pragma once 

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"
#include "twe_sercmd.hpp"

#include <memory>

namespace TWEFMT {

	/*****************************************************
	 * COMMON PACKET DEFS
	 *****************************************************/
	enum class E_PKT : uint8_t {
		PKT_ERROR,
		PKT_TWELITE, // for APP TWELITE
		PKT_PAL      // for APP PAL
	};

	class TwePacket {
	protected:
		const E_PKT _type;

	public:
		TwePacket(E_PKT ptyp = E_PKT::PKT_ERROR) : _type(ptyp) {}
		virtual E_PKT parse(uint8_t* p, uint8_t u8len) { return E_PKT::PKT_ERROR; }
		inline E_PKT get_type() { return _type; }
		virtual ~TwePacket() {}
	};

	/*****************************************************
	 * TWELITE PAL
	 *****************************************************/
	enum class E_PAL_PCB : uint8_t {
		NOPCB = 0x0,
		MAG = 0x01,
		AMB = 0x02,
		MOT = 0x03
	};

	enum class E_SNSCD : uint8_t {
		HALL = 0x0,
		TEMP = 0x1,
		HUMD = 0x2,
		LUMI = 0x3,
		ACCEL = 0x4,
		ACCEL_XYZ = 0x24,
		VOLT = 0x30,
		DIO = 0x31,
		EEP = 0x32
	};

	enum class E_EXCD_VOLT : uint8_t {
		POWER = 0x8,
		ADC1 = 0x1,
		ADC2 = 0x2,
		ADC3 = 0x3,
		ADC4 = 0x4
	};

	struct DataPal {
		uint32_t u32addr_rpt;
		uint32_t u32addr_src;

		uint16_t u16seq;

		uint8_t u8lqi;
		uint8_t u8addr_src;
		E_PAL_PCB u8palpcb;		// PCB kind (lower 5bits) and revision (higher 3 bits)
		uint8_t u8palpcb_rev;	// PCB resision
		uint8_t u8sensors;		// MSB=1:include parse error, lower bits: num of sensors
		uint8_t u8snsdatalen;   // if set MSB, it's as dynamic allocation.

		uint8_t au8snsdata[32];
		std::unique_ptr<uint8_t[]> uptr_snsdata;
	};

	struct PalBase {
		uint32_t u32StoredMask;
		uint16_t u16Volt;
	};

	struct PalMag : public PalBase {
		const uint8_t U8VARS_CT = 2; // Volt + MagStat
		const uint32_t STORE_COMP_MASK = (1 << U8VARS_CT) - 1;

		uint8_t u8MagStat;
		uint8_t bRegularTransmit; // MSB flag of u8MagStat
	};

	struct PalAmb : public PalBase {
		const uint8_t U8VARS_CT = 4; // Volt + 3Sensors
		const uint32_t STORE_COMP_MASK = (1 << U8VARS_CT) - 1;

		int16_t i16Temp;
		uint16_t u16Humd;
		uint32_t u32Lumi;
	};

	struct PalMot : public PalBase {
		const uint8_t U8VARS_CT = 17; // Volt + 3 AXIS*16samples
		const uint32_t STORE_COMP_MASK = 3; // Volt & 1sample

		uint8_t u8samples;
		int16_t i16X[16];
		int16_t i16Y[16];
		int16_t i16Z[16];
	};

	class TwePacketPal : public TwePacket, public DataPal {
		// parse each sensor data and convert into variables.
		uint32_t store_data(uint8_t u8listct, void** vars, const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint8_t* pu8dsList, const uint8_t* pu8exList);

	public:
		static const E_PKT _pkt_id = E_PKT::PKT_PAL;

		TwePacketPal() : TwePacket(_pkt_id), DataPal({ 0 }) { }
		~TwePacketPal() { }
		E_PKT parse(uint8_t* p, uint8_t u8len);

		PalMag& operator >> (PalMag& out);
		PalMag get_PalMag() {
			PalMag out;
			operator >> (out);
			return out;
		}

		PalAmb& operator >> (PalAmb& out);
		PalAmb get_PalAmb() {
			PalAmb out;
			operator >> (out);
			return out;
		}

		PalMot& operator >> (PalMot& out);
		PalMot get_PalMot() {
			PalMot out;
			operator >> (out);
			return out;
		}
	};
	
	/*****************************************************
	 * APP TWELITE 0x81 command
	 *****************************************************/
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
		bool b_lowlatency_tx;

		/**
		 * packet repeat count
		 *   e.g.) if set 1, the packet passed to one repeater (router) to the destination.
		 */
		uint16_t u8rpt_cnt;

		/**
		 * LQI value
		 */
		uint16_t u8lqi;

		/**
		 * true: DI1 is activated (set as Lo),
		 */
		bool DI1;

		/**
		 * true: DI1 is activated before.
		 * false: the port had not been activated ever.
		 */
		bool DI1_active;

		/**
		 * true: DI2 is activated (set as Lo)
		 */
		bool DI2;

		/**
		 * true: DI2 is activated before.
		 * false: the port had not been activated ever.
		 */
		bool DI2_active;

		/**
		 * true: DI3 is activated (set as Lo)
		 */
		bool DI3;

		/**
		 * true: DI3 is activated before.
		 * false: the port had not been activated ever.
		 */
		bool DI3_active;

		/**
		 * true: DI4 is activated (set as Lo)
		 */
		bool DI4;

		/**
		 * true: DI4 is activated before.
		 * false: the port had not been activated ever.
		 */
		bool DI4_active;

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

	class TwePacketTwelite : public TwePacket, public DataTwelite {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_TWELITE;

		TwePacketTwelite() : TwePacket(_pkt_id), DataTwelite({ 0 }) { }
		~TwePacketTwelite() { }
		E_PKT parse(uint8_t* p, uint8_t u8len);
	};

	/*****************************************************
	 * PACKET OPERATION
	 *****************************************************/
	// type definition (shared_ptr)
	typedef std::shared_ptr<TwePacket> spTwePacket;

	// check byte sequence and tell packet type.
	E_PKT identify_packet_type(uint8_t* p, uint8_t u8len);
	static inline E_PKT identify_packet_type(TWEUTILS::SmplBuf_Byte& sbuff) {
		return identify_packet_type(sbuff.begin(), (uint8_t)sbuff.length());
	}
	static inline E_PKT identify_packet_type(spTwePacket& sp) {
		return sp ? sp->get_type() : E_PKT::PKT_ERROR;
	}

	// check packet type with == operator.
	static inline bool operator == (spTwePacket& lhs, E_PKT rhs) {
		return lhs->get_type() == rhs;
	}
	static inline bool operator != (spTwePacket& lhs, E_PKT rhs) {
		return !operator == (lhs, rhs);
	}
	
	// Generic TWE Packet object generation
	spTwePacket newTwePacket(uint8_t* p, uint8_t u8len, E_PKT eType = E_PKT::PKT_ERROR);
	static inline spTwePacket newTwePacket(TWEUTILS::SmplBuf_Byte& sbuff, E_PKT eType = E_PKT::PKT_ERROR) {
		return newTwePacket(sbuff.begin(), (uint8_t)sbuff.length(), eType);
	}

	// reference to TwePacket for spTwePacket
	static inline TwePacket& refTwePacket(spTwePacket& p) {
		if (p) return *p;

		// error handling
		static spTwePacket _err;
		if (!_err) {
			_err.reset(new TwePacket());
		}
		return *_err;
	}

	// template for reference functions
	template <class T> 
	inline T& refTwePacketGen(spTwePacket& p) {
		static std::shared_ptr<T> _err;

		if (p && p->get_type() == T::_pkt_id) {
			return *static_cast<T*>(&*p);
		}

		// _err object (generate once)
		if (!_err) {
			_err.reset(new T());
		}
		return *_err;
	}

	/*****************************************************
	 * PACKET OPERATION (dedicated for PAL)
	 *****************************************************/
	typedef std::shared_ptr<TwePacketPal> spTwePacketPal;
	
	// generate new PAL packet
	spTwePacketPal newTwePacketPal(uint8_t* p, uint8_t u8len);
	static inline spTwePacketPal newTwePacketPal(TWEUTILS::SmplBuf_Byte& sbuff) {
		return newTwePacketPal(sbuff.begin(), (uint8_t)sbuff.length());
	}

	// reference to TwePacketPal for spTwePacket
	static inline TwePacketPal& refTwePacketPal(spTwePacket& p) { 
		return refTwePacketGen<TwePacketPal>(p);
	}

	/*****************************************************
	 * PACKET OPERATION (dedicated for TWELITE)
	 *****************************************************/
	 // reference to TwePacketPal for spTwePacket
	static inline TwePacketTwelite& refTwePacketTwelite(spTwePacket& p) {
		return refTwePacketGen<TwePacketTwelite>(p);
	}


}
