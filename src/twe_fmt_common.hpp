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
		TwePacket(E_PKT ptyp = E_PKT::PKT_ERROR) : _type(ptyp), common() {}
		virtual E_PKT parse(uint8_t* p, uint16_t len) { return E_PKT::PKT_ERROR; } // parse the packet
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
}