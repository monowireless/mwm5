#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /*****************************************************
  * APP TWELITE 0x81 command
  *****************************************************/

#include "twe_fmt_common.hpp"

namespace TWEFMT {
	/*****************************************************
	 * TWELITE PAL
	 *****************************************************/
	enum class E_PAL_PCB : uint8_t {
		NOPCB = 0x0,
		MAG = 0x01,		// MAG PAL (magnet switch)
		AMB = 0x02,		// AMB PAL (ambient sensor)
		MOT = 0x03,		// MOT PAL (motion PAL)
		NOTICE = 0x04,	// NOTICE PAL
		CUE = 0x05		// TWELITE CUE
	};

	/*****************************************************
	 * DATA TYPE (get_XXX)
	 *****************************************************/
	enum class E_PAL_DATA_TYPE : uint8_t {
		MAG_STD = 0x01,
		AMB_STD = 0x02,
		MOT_STD = 0x03,
		EX_CUE_STD = 0x05, // (extended) CUE standard
		EVENT_ONLY = 0x80, // has event, but no further data
		NODEF = 0xFF
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
		EEP = 0x32,
		INFO = 0x34,
		TIMER = 0x35,
		NODEF = 0xFF
	};

	enum class E_EXCD_VOLT : uint8_t {
		POWER = 0x8,
		ADC1 = 0x1,
		ADC2 = 0x2,
		ADC3 = 0x3,
		ADC4 = 0x4
	};

	enum class E_CAUSE : uint8_t {
		EVENT = 0x00,
		VALUE_CHANGED = 0x01,
		VALUE_OVER_LIMIT = 0x02,
		VALUE_UNDER_LIMIT = 0x03,
		VALUE_WITHIN_RANGE = 0x04,
		NODEF = 0xFF
	};

	enum class E_EVENT_ACCEL : uint8_t {
		DICE_1 = 0x01,
		DICE_2 = 0x02,
		DICE_3 = 0x03,
		DICE_4 = 0x04,
		DICE_5 = 0x05,
		DICE_6 = 0x06,
		SHAKE = 0x08,
		MOVE = 0x10,
		NODEF = 0xFF
	};

	struct DataPal {
		uint32_t u32addr_rpt;
		uint32_t u32addr_src;

		uint16_t u16seq;

		uint8_t u8lqi;
		uint8_t u8addr_src;
		union {
			E_PAL_PCB u8palpcb; // for backward compatibility
			uint8_t u8board;
			TWEUTILS::enum_wapper<E_PAL_PCB> e_board;	// board ID
		};
		uint8_t u8datafmt;		// 0:With PacketDataInfo, 1:standard data set (w/o PacketDataInfo)
		uint8_t u8sensors;		// MSB=1:include parse error, lower bits: num of sensors
		uint8_t u8snsdatalen;   // if set MSB, it's as dynamic allocation.

		uint8_t au8snsdata[32];
		std::unique_ptr<uint8_t[]> uptr_snsdata;

		bool has_data_info() { return u8datafmt == 0; }

		static const uint8_t DATAFMT_PACKET_WITH_DATA_INFO = 0;
		static const uint8_t DATAFMT_PACKET_STANDARD = 1;
	};

	// PAL event
	struct PalDataInfo {
		uint8_t _b_stored_pal_event; // 1: data has pal event, 0: no pal event
		uint8_t u8data_type;         // depending of u8palpcb (0: always event only data, 1: standard data, 2... others)

		// it stores information that which sensor/device is the cause of the transmit. (Timer, Sensor type)
		union {
			TWEUTILS::enum_wapper<E_SNSCD> e_data_source;		 // the main packet data (sensor)
			uint8_t u8_data_source;
		};

		// if the data source is timer, it implies the packet is transmitted regularly.
		bool is_data_source_timer() { return e_data_source == E_SNSCD::TIMER; }

		// it stored why the packet is transmitted. (cause an EVENT, sensor value, etc...)
		union {
			TWEUTILS::enum_wapper<E_CAUSE> e_data_cause;		 // indicates the data comes from
			uint8_t u8_data_cause;
		};

		PalDataInfo() : _b_stored_pal_event(0), u8data_type(0), e_data_source(E_SNSCD::NODEF), e_data_cause(E_CAUSE::NODEF) {}
	};

	struct PalEvent {
		//uint8_t b_stored;
		union {
			TWEUTILS::enum_wapper<E_SNSCD> e_event_source;
			uint8_t u8event_source;
		};
		union {
			uint8_t u8event_id;			// Event code
			TWEUTILS::enum_wapper<E_EVENT_ACCEL> e_event_accel;
		};
		uint32_t u32event_param;	// 24bit length

		// true if stored
		operator bool() { return !(u8event_source == 0xFF && u8event_id == 0xFF); }

		PalEvent() : u8event_source(0xFF), u8event_id(0xFF), u32event_param(0) {}
	};

	struct PalBase {
		uint32_t u32StoredMask; // bit0...: bit mask to store sensor data

		PalBase() : u32StoredMask(0) {}
	};

	struct PalMag : public PalBase {
		const uint8_t U8VARS_CT = 2; // Volt + MagStat
		const uint32_t STORE_COMP_MASK = (1 << U8VARS_CT) - 1;

		uint16_t u16Volt;       // module voltage

		uint8_t u8MagStat;
		uint8_t bRegularTransmit; // MSB flag of u8MagStat

		PalMag() : u16Volt(0), u8MagStat(0), bRegularTransmit(0) {}
	};

	struct PalAmb : public PalBase {
		const uint8_t U8VARS_CT = 4; // Volt + 3Sensors
		const uint32_t STORE_COMP_MASK = (1 << U8VARS_CT) - 1;
		static const uint32_t STORE_VOLT_MASK = 0b1; // Volt & ADC1 & 1sample
		static const uint32_t STORE_VOLT_TEMP = 0b10; // temperature
		static const uint32_t STORE_VOLT_HUMID = 0b100; // humidity
		static const uint32_t STORE_VOLT_LUMI = 0b1000; // luminance

		uint16_t u16Volt;       // module voltage

		int16_t i16Temp;
		uint16_t u16Humd;
		uint32_t u32Lumi;

		bool has_vcc() { return u32StoredMask & STORE_VOLT_MASK; }
		int16_t get_vcc_i16mV() { return (int16_t)u16Volt; }

		int has_temp() { return u32StoredMask & STORE_VOLT_TEMP; }
		int16_t get_temp_i16_100xC() { return (int16_t)i16Temp; }
		int has_humidity() { return (u32StoredMask & STORE_VOLT_HUMID) && !(i16Temp <= -32760); }
		int16_t get_humidity_i16_100xPC() { return (int16_t)u16Humd && (u16Humd <= 10000); }
		int has_luminance() { return u32StoredMask & STORE_VOLT_LUMI; }
		uint32_t get_luminance_LUX() { return u32Lumi; }

		PalAmb() : u16Volt(0), i16Temp(0), u16Humd(0), u32Lumi(0) {}
	};

	struct PalMot : public PalBase {
		static const int MAX_SAMPLES = 16;

		static const uint8_t U8VARS_CT = MAX_SAMPLES + 1; // Volt + 3 AXIS*16samples
		static const uint32_t STORE_COMP_MASK = 0b11; // 0b11 Volt & 1sample
		static const uint32_t STORE_VOLT_MASK = 0b1; // Volt & ADC1 & 1sample
		static const uint32_t STORE_ACCELERO_MASK = 0b10; // Volt & ADC1 & 1sample

		uint16_t u16Volt;   // module voltage

		uint8_t u8samples;  // num of sample stored.
		uint8_t u8sample_rate_code; // sample rate (0: 25Hz, 4:100Hz)

		int16_t i16X[16]; // X axis samples
		int16_t i16Y[16]; // Y axis samples
		int16_t i16Z[16]; // Z axis samples

		// query method
		bool has_vcc() { return u32StoredMask & STORE_VOLT_MASK; }
		int16_t get_vcc_i16mV() { return (int16_t)u16Volt; }
		bool has_accel() { return u32StoredMask & STORE_ACCELERO_MASK; }
		uint8_t get_accel_count_u8() { return u8samples; }
		int16_t get_accel_X_i16mG(int i) { return i16X[i]; }
		int16_t get_accel_Y_i16mG(int i) { return i16Y[i]; }
		int16_t get_accel_Z_i16mG(int i) { return i16Z[i]; }

		PalMot() : u16Volt(0xFFFF), u8samples(0xFF), u8sample_rate_code(0xFF), i16X{}, i16Y{}, i16Z{} {}
	};

	struct TweCUE : public PalBase {
		static const int MAX_SAMPLES = 10;

		static const uint8_t U8VARS_CT = MAX_SAMPLES + 3; // Volt + ADC1 + MAG + 3 AXIS*10samples
		static const uint32_t STORE_COMP_MASK = 0b1111; // Volt & ADC1 & MAG & 1sample
		static const uint32_t STORE_VOLT_MASK = 0b1; // Volt & ADC1 & 1sample
		static const uint32_t STORE_ADC1_MASK = 0b10; // Volt & ADC1 & 1sample
		static const uint32_t STORE_MAG_MASK = 0b100; // Volt & ADC1 & 1sample
		static const uint32_t STORE_ACCELERO_MASK = 0b1000; // Volt & ADC1 & 1sample

		uint16_t u16Volt;   // module voltage
		uint16_t u16Adc1;	// ADC1 voltage

		uint8_t u8MagStat;  // MAGNET SW status
		uint8_t bMagRegularTransmit; // MSB flag of u8MagStat

		uint8_t u8samples;  // num of sample stored.
		uint8_t u8sample_rate_code; // sample rate (0: 25Hz, 4:100Hz)

		int16_t i16X[10]; // X axis samples
		int16_t i16Y[10]; // Y axis samples
		int16_t i16Z[10]; // Z axis samples

		// query method
		bool has_vcc() { return u32StoredMask & STORE_VOLT_MASK; }
		int16_t get_vcc_i16mV() { return int16_t(u16Volt); }
		bool has_adc1() { return u32StoredMask & STORE_ADC1_MASK; }
		int16_t get_adc1_i16mV() { return int16_t(u16Adc1); }
		bool has_mag() { return u32StoredMask & STORE_MAG_MASK; }
		uint8_t get_mag_stat_u8() { return u8MagStat; }
		uint8_t is_mag_regular_transmit() { return bMagRegularTransmit; }
		bool has_accel() { return u32StoredMask & STORE_ACCELERO_MASK; }
		uint8_t get_accel_count_u8() { return u8samples; }
		int16_t get_accel_X_i16mG(int i) { return i16X[i]; }
		int16_t get_accel_Y_i16mG(int i) { return i16Y[i]; }
		int16_t get_accel_Z_i16mG(int i) { return i16Z[i]; }

		TweCUE() : u16Volt(0xFFFF), u16Adc1(0xFFFF), u8MagStat(0xff), bMagRegularTransmit(0xFF), u8samples(0xFF), u8sample_rate_code(0xFF), i16X{}, i16Y{}, i16Z{} {}
	};

	class TwePacketPal : public TwePacket, public DataPal, public PalEvent, public PalDataInfo {
		// parse each sensor data and convert into variables.
		uint32_t store_data(uint8_t u8listct, void** vars,
				const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint16_t* pu8dsList, const uint16_t* pu8exList,
				uint8_t* pu8exListReads = nullptr, uint8_t* pu8ReadDataCount = nullptr);
		std::pair<bool, uint16_t> query_volt();
		std::pair<bool, PalEvent> query_event();
		std::pair<bool, PalDataInfo> query_data_info();

	public:
		static const E_PKT _pkt_id = E_PKT::PKT_PAL;

		TwePacketPal() : TwePacket(_pkt_id), DataPal({ 0 }) { }
		~TwePacketPal() { }
		E_PKT parse(uint8_t* p, uint16_t len);

		static inline E_PKT identify(uint8_t* p, uint8_t* e, uint16_t u16len) {
			// TWELITE PAL
			if (   u16len > 14 // at senser data count
				&& p[0] & 0x80 // 
				&& p[7] & 0x80
				&& p[12] == 0x80
				) {
				// check sensor data part
				uint8_t u8ct = p[14], * ps0 = &p[15], * ps = ps0;
				uint8_t u8Sensors = 0;
				for (int i = 0; i < u8ct; i++) {
					if (e > ps + 4) { // check packet len
						uint8_t l = ps[3];
						ps += 4; // skip header

						if (e > ps + l) { // check packet len
							u8Sensors++;
						}
						ps += l;
					}
				}
				if (u8ct == u8Sensors) {
					if (e > ps) {
						// CRC
						uint8_t u8crc = TWEUTILS::CRC8_u8Calc(p, uint8_t(ps - p));

						if (u8crc == *ps) { // match CRC8
							// accept
							return _pkt_id;
						}
					}
				}
			}

			return E_PKT::PKT_ERROR;
		}

		// query data format is extended or not.
		bool is_extended_format() {
			return DataPal::has_data_info();
		}
		
		// query Pal Event Data
		bool is_PalEvent() { return PalDataInfo::_b_stored_pal_event; }
		bool has_PalEvent() { return is_PalEvent(); }

		// query event
		PalEvent& get_PalEvent() { return *static_cast<PalEvent*>(this); }
		PalEvent& operator >> (PalEvent& out) {
			out = get_PalEvent();
			return out;
		}

		// determine which get_??? function is used to get proper data.
		E_PAL_DATA_TYPE get_PalDataType() {
			if (DataPal::has_data_info()) {
				// if having extended data.
				if (e_board == E_PAL_PCB::CUE) {
					return E_PAL_DATA_TYPE::EX_CUE_STD; // use get_TweCUE()
				} else
				if (has_PalEvent()) {
					return E_PAL_DATA_TYPE::EVENT_ONLY;
				}
			}
			else {
				switch (e_board) {
				case E_PAL_PCB::AMB: return E_PAL_DATA_TYPE::AMB_STD;
				case E_PAL_PCB::MAG: return E_PAL_DATA_TYPE::MAG_STD;
				case E_PAL_PCB::MOT: return E_PAL_DATA_TYPE::MOT_STD;
				case E_PAL_PCB::NOTICE: return E_PAL_DATA_TYPE::EVENT_ONLY;
				default: break;
				}
			}

			return E_PAL_DATA_TYPE::NODEF;
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

		// query PalMag data structure
		TweCUE& operator >> (TweCUE& out);
		TweCUE get_TweCUE() {
			TweCUE out;
			operator >> (out);
			return out;
		}
	};

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

}