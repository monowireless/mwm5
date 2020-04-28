/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_binary.hpp"

using namespace TWE;
using namespace TWESERCMD;
using namespace TWESYS;
using namespace TWEUTILS;

namespace TWESERCMD {
	typedef enum {
		E_SERCMD_BINARY_EMPTY = 0,      //!< E_SERCMD_BINARY_EMPTY
		E_SERCMD_BINARY_READSYNC,       //!< E_SERCMD_BINARY_READSYNC
		E_SERCMD_BINARY_READLEN,        //!< E_SERCMD_BINARY_READLEN
		E_SERCMD_BINARY_READLEN2,       //!< E_SERCMD_BINARY_READLEN2
		E_SERCMD_BINARY_READPAYLOAD,    //!< E_SERCMD_BINARY_READPAYLOAD
		E_SERCMD_BINARY_READCRC,        //!< E_SERCMD_BINARY_READCRC
		E_SERCMD_BINARY_PLUS1,          //!< E_SERCMD_BINARY_PLUS1
		E_SERCMD_BINARY_PLUS2,          //!< E_SERCMD_BINARY_PLUS2
		E_SERCMD_BINARY_COMPLETE = 0x80,//!< E_SERCMD_BINARY_COMPLETE
		E_SERCMD_BINARY_ERROR = 0x81,   //!< E_SERCMD_BINARY_ERROR
		E_SERCMD_BINARY_CRCERROR = 0x82 //!< E_SERCMD_BINARY_CRCERROR
	} teSerCmd_Binary;
}

/// <summary>
/// バイナリ形式の１バイト解釈
/// </summary>
/// <param name="u8byte"></param>
/// <returns></returns>
uint8_t BinaryParser::_u8Parse(uint8_t u8byte) {
	// check for timeout
	if (TimeOut::is_enabled() && TimeOut::is_timeout()) {
		u8state = E_SERCMD_BINARY_EMPTY;
	}

	// check for complete or error status
	if (u8state >= 0x80) {
		u8state = E_SERCMD_BINARY_EMPTY;
	}

	// run state machine
	switch (u8state) {
	case E_SERCMD_BINARY_EMPTY:
		if (u8byte == SERCMD_SYNC_1) {
			u8state = E_SERCMD_BINARY_READSYNC;
			TimeOut::start(); // start timer again
		}
		break;

	case E_SERCMD_BINARY_READSYNC:
		if (u8byte == SERCMD_SYNC_2) {
			u8state = E_SERCMD_BINARY_READLEN;
		}
		else {
			u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READLEN:
		if (u8byte & 0x80) {
			// long length mode (1...
			u8byte &= 0x7F;
			u16pos = u8byte;
			u8state = E_SERCMD_BINARY_READLEN2;

		}
		else {
			// short length mode (1...127bytes)
			if (u8byte && payload.redim(u8byte)) {
				u8state = E_SERCMD_BINARY_READPAYLOAD;
				u16pos = 0;
				u16cksum = 0;
			}
			else {
				u8state = E_SERCMD_BINARY_ERROR;
			}
		}
		break;

	case E_SERCMD_BINARY_READLEN2:
		if (payload.redim(u16pos * 256 + u8byte)) {
			u16pos = 0;
			u16cksum = 0;
			u8state = E_SERCMD_BINARY_READPAYLOAD;
		}
		else {
			u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READPAYLOAD:
		payload[u16pos] = u8byte;
		u16cksum ^= u8byte; // update XOR checksum
		if (u16pos == payload.length() - 1) {
			u8state = E_SERCMD_BINARY_READCRC;
		}
		u16pos++;
		break;

	case E_SERCMD_BINARY_READCRC:
		u16cksum &= 0xFF;
		if (u8byte == u16cksum) {
			u8state = E_SERCMD_BINARY_COMPLETE;
		}
		else {
			u8state = E_SERCMD_BINARY_CRCERROR;
		}
		break;

	default:
		break;
	}

	return u8state;
}

/** @ingroup SERCMD
 * バイナリ形式の出力 (staticメソッド)
 * @param pc
 * @param ps
 */
void BinaryParser::s_vOutput(SmplBuf_Byte& payload, IStreamOut& vPutChar) {
	unsigned char u8xor = 0;

	vPutChar(SERCMD_SYNC_1);
	vPutChar(SERCMD_SYNC_2);
	vPutChar((unsigned char)(0x80 | (payload.length() >> 8)));
	vPutChar(payload.length() & 0xff);

	for (unsigned i = 0; i < payload.length(); i++) {
		u8xor ^= payload[i];
		vPutChar(payload[i]);
	}

	vPutChar(u8xor); // XOR check sum
	vPutChar(0x4); // EOT
}

