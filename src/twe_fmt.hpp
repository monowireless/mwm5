#pragma once 

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"
#include "twe_sercmd.hpp"
#include "twe_utils_simplebuffer.hpp"

#include <memory>

namespace TWEFMT {

	/*****************************************************
	 * COMMON PACKET DEFS
	 *****************************************************/
	enum class E_PKT : uint8_t {
		PKT_ERROR,
		PKT_TWELITE, // for APP TWELITE
		PKT_PAL,     // for APP PAL
		PKT_APPIO,   // for APP IO
		PKT_APPUART, // for APP UART
		PKT_APPTAG,  // for APP UART
		PKT_ACT_STD  // for Act Standard packet structure
	};

	class TwePacket {
	protected:
		const E_PKT _type;
		
	public:
		TwePacket(E_PKT ptyp = E_PKT::PKT_ERROR) : _type(ptyp), common{} {}
		virtual E_PKT parse(uint8_t* p, uint16_t len) { return E_PKT::PKT_ERROR; }
		inline E_PKT get_type() { return _type; }
		virtual ~TwePacket() {}

		// common information set by parse()
		struct {
			uint32_t tick;
			uint32_t src_addr;
			uint8_t src_lid;
			uint8_t lqi;
			uint16_t volt;
			void clear() {
				tick = 0;
				src_addr = 0;
				src_lid = 0;
				lqi = 0;
				volt = 0;
			}
		} common;
	};

	/*****************************************************
	 * TWELITE PAL
	 *****************************************************/
	enum class E_PAL_PCB : uint8_t {
		NOPCB = 0x0,
		MAG = 0x01,
		AMB = 0x02,
		MOT = 0x03,
		NOTICE = 0x04
	};

	enum class E_SNSCD : uint8_t {
		HALL = 0x0,
		TEMP = 0x1,
		HUMD = 0x2,
		LUMI = 0x3,
		ACCEL = 0x4,
		EVENT = 0x5,
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

	// PAL event
	struct PalEvent {
		uint8_t b_stored;
		uint8_t u8event_source;
		uint8_t u8event_id;       // Event code
		uint32_t u32event_param;   // 24bit length
	};

	struct PalBase {
		uint32_t u32StoredMask; // bit0...: bit mask to store sensor data
		uint16_t u16Volt;       // module voltage
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

		uint8_t u8samples; // num of sample stored.
		uint8_t u8sample_rate_code; // sample rate (0: 25Hz, 4:100Hz)
		int16_t i16X[16]; // X axis samples
		int16_t i16Y[16]; // Y axis samples
		int16_t i16Z[16]; // Z axis samples
	};

	class TwePacketPal : public TwePacket, public DataPal, public PalEvent {
		// parse each sensor data and convert into variables.
		uint32_t store_data(uint8_t u8listct, void** vars, const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint16_t* pu8dsList, const uint16_t* pu8exList, uint8_t* pu8exListReads = nullptr);
		std::pair<bool, uint16_t> query_volt();
		std::pair<bool, PalEvent> query_event();

	public:
		static const E_PKT _pkt_id = E_PKT::PKT_PAL;

		TwePacketPal() : TwePacket(_pkt_id), DataPal({ 0 }) { }
		~TwePacketPal() { }
		E_PKT parse(uint8_t* p, uint16_t len);

		// query Pal Event Data
		bool is_PalEvent() { return PalEvent::b_stored; }
		PalEvent& get_PalEvent() { return *static_cast<PalEvent*>(this); }
		PalEvent& operator >> (PalEvent& out) {
			out = get_PalEvent();
			return out;
		}

		// query PalMag data structure
		PalMag& operator >> (PalMag& out);
		PalMag get_PalMag() {
			PalMag out;
			operator >> (out);
			return out;
		}

		// query PalMag data structure
		PalAmb& operator >> (PalAmb& out);
		PalAmb get_PalAmb() {
			PalAmb out;
			operator >> (out);
			return out;
		}

		// query PalMag data structure
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

	class TwePacketTwelite : public TwePacket, public DataTwelite {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_TWELITE;

		TwePacketTwelite() : TwePacket(_pkt_id), DataTwelite({ 0 }) { }
		~TwePacketTwelite() { }
		E_PKT parse(uint8_t* p, uint16_t len);
	};

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
	};


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
	};

	/*****************************************************
	 * Act standard packet structure
	 *****************************************************/
	class TwePacketActStd : public TwePacketAppUART {
	public:
		static const E_PKT _pkt_id = E_PKT::PKT_ACT_STD;

		TwePacketActStd() : TwePacketAppUART(this->_pkt_id) { }
		~TwePacketActStd() { }
	};

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
	};

	/*****************************************************
	 * PACKET OPERATION
	 *****************************************************/
	// type definition (shared_ptr)
	typedef std::shared_ptr<TwePacket> spTwePacket;

	// check byte sequence and tell packet type.
	E_PKT identify_packet_type(uint8_t* p, uint16_t len);
	static inline E_PKT identify_packet_type(TWEUTILS::SmplBuf_Byte& sbuff) {
		return identify_packet_type(sbuff.data(), (uint16_t)sbuff.length());
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
	spTwePacket newTwePacket(uint8_t* p, uint16_t len, E_PKT eType = E_PKT::PKT_ERROR);
	static inline spTwePacket newTwePacket(TWEUTILS::SmplBuf_Byte& sbuff, E_PKT eType = E_PKT::PKT_ERROR) {
		return newTwePacket(sbuff.data(), (uint8_t)sbuff.length(), eType);
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
	spTwePacketPal newTwePacketPal(uint8_t* p, uint16_t len);
	static inline spTwePacketPal newTwePacketPal(TWEUTILS::SmplBuf_Byte& sbuff) {
		return newTwePacketPal(sbuff.data(), (uint16_t)sbuff.length());
	}

	// reference to TwePacketPal for spTwePacket
	static inline TwePacketPal& refTwePacketPal(spTwePacket& p) { 
		return refTwePacketGen<TwePacketPal>(p);
	}

	/*****************************************************
	 * PACKET OPERATION (other Apps)
	 *****************************************************/
	 // reference to TwePacketPal for spTwePacket
	static inline TwePacketTwelite& refTwePacketTwelite(spTwePacket& p) {
		return refTwePacketGen<TwePacketTwelite>(p);
	}
	static inline TwePacketAppIO& refTwePacketAppIO(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppIO>(p);
	}
	static inline TwePacketAppUART& refTwePacketAppUART(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppUART>(p);
	}
	static inline TwePacketActStd& refTwePacketActStd(spTwePacket& p) {
		return refTwePacketGen<TwePacketActStd>(p);
	}
	static inline TwePacketAppTAG& refTwePacketAppTAG(spTwePacket& p) {
		return refTwePacketGen<TwePacketAppTAG>(p);
	}
}
