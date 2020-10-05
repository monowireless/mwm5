/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>
#include "twe_fmt.hpp"

using namespace TWEFMT;
using namespace TWEUTILS;

#if defined(TWE_STDINOUT_ONLY)
# ifdef TWE_HAS_MILLIS
extern uint32_t millis();
# else
#define millis() (0)
# endif
#else
#include "twe_sys.hpp"
#endif

/// <summary>
/// parse the PAL packet.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
E_PKT TwePacketPal::parse(uint8_t* pb, uint16_t u16len) {
	uint8_t* p = pb;
	uint8_t* e = pb + u16len;
	uint8_t c = 0;
	bool bErr = false;

	TwePacket::common.clear();
	
	// at least 11 bytes are necessary
	if (u16len < 11) {
		return E_PKT::PKT_ERROR;
	}

	// the first 4 bytes: ser# of router or 0x80000000(direct)
	u32addr_rpt = G_DWORD(p);

	// LQI value
	u8lqi = G_OCTET(p);

	// sequence number
	u16seq = G_WORD(p);

	// src address (32bit)
	u32addr_src = G_DWORD(p);

	// src address (8bit)
	u8addr_src = G_OCTET(p);

	// 0x80 fixed
	c = G_OCTET(p);
	if (c != 0x80) {
		return E_PKT::PKT_ERROR;
	}

	// PAL pcb
	c = G_OCTET(p);
	u8palpcb = (E_PAL_PCB)(c & 0x1F);
	c >>= 5;
	c = ((c & 1) << 2) | (c & 2) | ((c & 4) >> 2);
	u8palpcb_rev = c; // revision (upper 4bits)

	// num of sensors
	u8sensors = G_OCTET(p);

	// save this ptr (to calculate sensor data length)
	uint8_t* psensor = p;

	// check data
	int iStored = 0;
	// Sensors.clear(); // clear map structure
	for (int i = 0; i < u8sensors; i++) {
		// check header len
		if (e < p + 4) { break; }

		// check headers
		uint8_t u8dt = G_OCTET(p);
		uint8_t u8ds = G_OCTET(p);
		uint8_t u8ex = G_OCTET(p);
		uint8_t u8ln = G_OCTET(p);

		// check payload len
		if (e < p + u8ln) { break; }

		// update counters, pointers
		iStored++;
		p += u8ln;
	}

	if (iStored != u8sensors) {
		// some error but accept parse sensor data.
		u8sensors = iStored;
		bErr = true;
	}
	else {
		// perform checksum check
		uint8_t u8crc = TWEUTILS::CRC8_u8Calc(pb, (uint8_t)(p - pb));
		uint8_t c = G_OCTET(p);
		if (c != u8crc) bErr = true;
	}

	// copy data (copy serial format as is)
	int len = int((p - 1) - psensor);
	if (len <= sizeof(au8snsdata)) { // TODO
		// copy data into pre-allocated area
		memcpy(au8snsdata, psensor, len);
		u8snsdatalen = len;
	}
	else {
		// allocate with unique pointer. (note: dynamic allocation with new operator)
		uptr_snsdata.reset(new uint8_t[len]);
		memcpy(uptr_snsdata.get(), psensor, len);
		u8snsdatalen = len | 0x8000;
	}

	// store common data
	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataPal::u32addr_src;
	TwePacket::common.src_lid = DataPal::u8addr_src;
	TwePacket::common.lqi = DataPal::u8lqi;

	if (!bErr) {
		// store volt data
		auto res_volt = query_volt();
		if (res_volt.first) {
			TwePacket::common.volt = res_volt.second;
		}

		auto res_ev = query_event();
		if (res_ev.first) {
			PalEvent::b_stored = true;
			PalEvent::u8event_id = res_ev.second.u8event_id;
			PalEvent::u8event_source = res_ev.second.u8event_source;
			PalEvent::u32event_param = res_ev.second.u32event_param;
		}
		else {
			PalEvent::b_stored = false;
		}
	}

	// return packet ID
	return E_PKT::PKT_PAL;
}

/// <summary>
/// parse the AppTAG packet.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
E_PKT TwePacketAppTAG::parse(uint8_t* pb, uint16_t u16len) {
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :80000000B10001810043C10032C9047C02AF0A41D2
	//  ^^^^^^^1^2^^^3^^^^^^^4^5^6^7^^^8^^^9^^^a^b

	uint8_t* p = pb;
	uint8_t* e = pb + u16len;
	uint8_t c = 0;
	bool bChecksumErr = false;

	DataAppTAG::payload.clear();
	TwePacket::common.clear();
	
	// at least 13 bytes are necessary
	if (u16len < 13) {
		return E_PKT::PKT_ERROR;
	}

	// the first 4 bytes: ser# of router or 0x80000000(direct)
	DataAppTAG::u32addr_rpt = G_DWORD(p);

	// LQI value
	DataAppTAG::u8lqi = G_OCTET(p);

	// sequence number
	DataAppTAG::u16seq = G_WORD(p);

	// src address (32bit)
	DataAppTAG::u32addr_src = G_DWORD(p);

	// src address (8bit)
	DataAppTAG::u8addr_src = G_OCTET(p);

	// 0x80 fixed
	DataAppTAG::u8sns = G_OCTET(p);
	if (c == 0x80) {
		return E_PKT::PKT_ERROR;
	}

	// volt
	DataAppTAG::u16Volt = DecodeVolt(G_OCTET(p));

	// rest of bytes
	if (p < e) {
		DataAppTAG::payload.reserve_and_set_empty(int(e - p));
		DataAppTAG::payload << std::make_pair(p, e);
	}
	
	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppTAG::u32addr_src;
	TwePacket::common.src_lid = DataAppTAG::u8addr_src;
	TwePacket::common.lqi = DataAppTAG::u8lqi;
	TwePacket::common.volt = DataAppTAG::u16Volt;

	// return packet ID
	return E_PKT::PKT_APPTAG;
}

/**
 * @fn	E_PKT TwePacketTwelite::parse(uint8_t* pyld, uint8_t u16len)
 *
 * @brief	Parses App_Twelite 0x81 packet content
 *
 *
 * @param [in,out]	pyld 	pointer to the byte array
 * @param 		  	u16len	The length of byte array
 *
 * @returns	An E_PKT.
 */
E_PKT TwePacketTwelite::parse(uint8_t* pyld, uint16_t u16len) {
	bool bValid = false;
	TwePacket::common.clear();
	
	// length check
	if (u16len != 23) {
		return E_PKT::PKT_ERROR;
	}

	// misc vars
	int c;

	// read
	u8addr_src = G_OCTET(pyld); 					// addr src (LID)

	c = G_OCTET(pyld); // 0x81

	c = G_OCTET(pyld); // packet identified (need AppID to calculate)

	c = G_OCTET(pyld); // protocol version

	u8lqi = G_OCTET(pyld); 							// LQI

	u32addr_src = G_DWORD(pyld); 					// SerID

	u8addr_dst = G_OCTET(pyld); 					// addr dst (LID)

	c = G_WORD(pyld);								// timestamp&low latency flag
	u16timestamp = c & 0x7FFF;								// -time stamp
	b_lowlatency_tx = ((c & 0x8000) == 0x8000);				// -low latency flag

	u8rpt_cnt = G_OCTET(pyld);					 	// repeat count

	u16Volt = (short)G_WORD(pyld);					// module voltage

	c = G_OCTET(pyld);								// unused

	c = G_OCTET(pyld);								// DI state bit
	DI1 = ((c & 0x01) == 0x01);
	DI2 = ((c & 0x02) == 0x02);
	DI3 = ((c & 0x04) == 0x04);
	DI4 = ((c & 0x08) == 0x08);
	DI_mask = c;

	c = G_OCTET(pyld);								// DI active state bit
	DI1_active = ((c & 0x01) == 0x01);
	DI2_active = ((c & 0x02) == 0x02);
	DI3_active = ((c & 0x04) == 0x04);
	DI4_active = ((c & 0x08) == 0x08);
	DI_active_mask = c;

	Adc_active_mask = 0;									// ADC
	u16Adc1 = G_OCTET(pyld);
	if (u16Adc1 != 0xFF) Adc_active_mask |= 1; else u16Adc1 = 0xFFFF;
	u16Adc2 = G_OCTET(pyld);
	if (u16Adc2 != 0xFF) Adc_active_mask |= 2; else u16Adc2 = 0xFFFF;
	u16Adc3 = G_OCTET(pyld);
	if (u16Adc3 != 0xFF) Adc_active_mask |= 4; else u16Adc3 = 0xFFFF;
	u16Adc4 = G_OCTET(pyld);
	if (u16Adc4 != 0xFF) Adc_active_mask |= 8; else u16Adc4 = 0xFFFF;
	c = G_OCTET(pyld);

	if (u16Adc1 != 0xFFFF) u16Adc1 = ((u16Adc1 * 4 + ((c >> 0) & 0x3)) * 4); // additional two bits
	if (u16Adc2 != 0xFFFF) u16Adc2 = ((u16Adc2 * 4 + ((c >> 2) & 0x3)) * 4);
	if (u16Adc3 != 0xFFFF) u16Adc3 = ((u16Adc3 * 4 + ((c >> 4) & 0x3)) * 4);
	if (u16Adc4 != 0xFFFF) u16Adc4 = ((u16Adc4 * 4 + ((c >> 6) & 0x3)) * 4);

	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataTwelite::u32addr_src;
	TwePacket::common.src_lid = DataTwelite::u8addr_src;
	TwePacket::common.lqi = DataTwelite::u8lqi;
	TwePacket::common.volt = DataTwelite::u16Volt;

	bValid = true;
	return bValid ? E_PKT::PKT_TWELITE : E_PKT::PKT_ERROR;
}

/**
 * @fn	E_PKT TwePacketAppIO::parse(uint8_t* pyld, uint8_t u16len)
 *
 * @brief	Parses TwePacketAppIO 0x81 packet content
 *
 *
 * @param [in,out]	pyld 	pointer to the byte array
 * @param 		  	u16len	The length of byte array
 *
 * @returns	An E_PKT.
 */
E_PKT TwePacketAppIO::parse(uint8_t* pyld, uint16_t u16len) {	
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :01810F01DB8630000200645F000040004F004000??
	// :78811202848201015A003FC9000001000100010086
	//    **--**LqAddr_srcAdTimsRpIostIoenIoit--Cs

	bool bValid = false;
	TwePacket::common.clear();
	
	// length check
	if (u16len != 20) {
		return E_PKT::PKT_ERROR;
	}

	// misc vars
	int c;

	// read
	u8addr_src = G_OCTET(pyld); 					// addr src

	c = G_OCTET(pyld); // 0x81

	c = G_OCTET(pyld); // packet identified (need AppID to calculate)

	c = G_OCTET(pyld); // protocol version

	u8lqi = G_OCTET(pyld); 							// LQI

	u32addr_src = G_DWORD(pyld); 					// SerID

	u8addr_dst = G_OCTET(pyld); 					// addr dst (LID)

	c = G_WORD(pyld);								// timestamp&low latency flag
	u16timestamp = c & 0x7FFF;								// -time stamp
	b_lowlatency_tx = ((c & 0x8000) == 0x8000);				// -low latency flag

	u8rpt_cnt = G_OCTET(pyld);					 	// repeat count

	DI_mask = G_WORD(pyld);
	DI_active_mask = G_WORD(pyld);
	DI_int_mask = G_WORD(pyld);

	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppIO::u32addr_src;
	TwePacket::common.src_lid = DataAppIO::u8addr_src;
	TwePacket::common.lqi = DataAppIO::u8lqi;

	bValid = true;
	return bValid ? E_PKT::PKT_APPIO : E_PKT::PKT_ERROR;
}

E_PKT TwePacketAppUART::parse(uint8_t* pyld, uint16_t u16len) {
	// App UART (extended format)
	//   0 1 2 3 4 5 6 7 8 9 a b c d  
	// :78A01286300001FFFFFFFFD80003123456??
	//      ^1^2      ^3      ^4^5  ^6
	//  *1 応答番号 12
	//  *2 送信元アドレス(86300001)
	//  *3 送信先アドレス（FFFFFFFF 情報なし）
	//  *4 通信品質(LQI)
	//  *5 データ部バイト数 (0003=3バイト)
	//  *6 データ (3バイトのデータ)
	uint8_t* b = pyld;
	uint8_t* e = pyld + u16len;
	
	bool bValid = false;

	// init members
	DataAppUART::payload.clear();
	TwePacket::common.clear();
	
	// misc vars
	int c;

	// read
	DataAppUART::u8addr_src = G_OCTET(pyld); 					// addr src

	c = G_OCTET(pyld); // 0xA0
	if (!(c == 0xA0 || c == 0xAA)) return E_PKT::PKT_ERROR;

	DataAppUART::u8response_id = G_OCTET(pyld);

	DataAppUART::u32addr_src = G_DWORD(pyld); 					// SerID
	DataAppUART::u32addr_dst = G_DWORD(pyld); 					// SerID

	DataAppUART::u8lqi = G_OCTET(pyld); 							// LQI

	DataAppUART::u16paylen = G_WORD(pyld);

	if (pyld + DataAppUART::u16paylen != e) {
		return E_PKT::PKT_ERROR;
	}
	DataAppUART::payload << std::make_pair(pyld, e);
	
	TwePacket::common.clear();
	TwePacket::common.tick = millis();
	TwePacket::common.src_addr = DataAppUART::u32addr_src;
	TwePacket::common.src_lid = DataAppUART::u8addr_src;
	TwePacket::common.lqi = DataAppUART::u8lqi;

	bValid = true;
	return bValid ? E_PKT::PKT_APPUART : E_PKT::PKT_ERROR;
}

/**
 * @fn	E_PKT TWEFMT::identify_packet_type(uint8_t* p, uint8_t u16len)
 *
 * @brief	Identify packet type
 *
 * @param [in,out]	p	 	If non-null, a pointer to payload of byte array.
 * @param 		  	u16len	the length of 'p'
 *
 * @returns	An packet id, E_PKT::PKT_ERROR in case of an error.
 */
E_PKT TWEFMT::identify_packet_type(uint8_t* p, uint16_t u16len) {
	uint8_t* e = p + u16len;

	bool bAccept = false;

	// TWELITE PAL
	if (!bAccept
		&& u16len > 14 // at senser data count
		&& p[0] & 0x80 // 
		&& p[7] & 0x80
		&& p[12] == 0x80
	) {
		// check sensor data part
		uint8_t u8ct = p[14], *ps0 = &p[15], *ps = ps0;
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
					return E_PKT::PKT_PAL;
				}
			}
		}
	}

	// App Twelite 0x81 command
	//  
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 4 5 6 -
	// :7881150175810000380026C9000C04220000FFFFFFFFFFA7
	if (!bAccept
		&& u16len == 23
		&& p[1] == 0x81 			// 0x81 command
		&& p[3] == 0x01 			// protocol version
		&& (p[5] & 0x80) == 0x80 	// SID: MSB must be set
		) {
		return E_PKT::PKT_TWELITE;
	}

	// App IO 0x81
	//
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :01810F01DB8630000200645F000040004F004000??
	// :78811202848201015A003FC9000001000100010086
	//    **--**LqAddr_srcAdTimsRpIostIoenIoit--Cs
	if (!bAccept
		&& u16len == 20
		&& p[1] == 0x81				// 0x81 command
		&& p[3] == 0x02				// protocol version
		&& (p[5] & 0x80) == 0x80 	// SID: MSB must be set
		) {			// the difference from App_Twelite is the length 
		return E_PKT::PKT_APPIO;
	}

	// App UART (extended format)
	//   0 1 2 3 4 5 6 7 8 9 a b c d  
	// :78A01286300001FFFFFFFFD80003123456??
	//      ^1^2      ^3      ^4^5  ^6
	//  *1 応答番号 12
	//  *2 送信元アドレス(86300001)
	//  *3 送信先アドレス（FFFFFFFF 情報なし）
	//  *4 通信品質(LQI)
	//  *5 データ部バイト数 (0003=3バイト)
	//  *6 データ (3バイトのデータ)
	if (!bAccept
		&& u16len > 13
		&& p[1] == 0xA0
		&& (p[3] & 0x80) == 0x80
		&& ((p[12] * 256 + p[13]) == u16len - 14) // length
		) {
		return E_PKT::PKT_APPUART;
	}

	// Act
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f
	// :FEAA008201015A00000000B7000F424154310F0CEE000B03FF03FF03FF92
	//  ^1^2^3^^^^^^^4^^^^^^^5^6^^^7^^^^^^^^^^^^^^^^^^^^^^^^^^^^^8^9
	//
	//  *1 送信先LID
	//  *2 0xAA 固定
	//  *4 送信元アドレス
	//  *5 宛先アドレス
	//  *6 LQI
	//  *7 データ部バイト数 (000F=15バイト)
	//  *8 データ (15バイトのデータ)
	if (!bAccept
		&& u16len > 13
		&& p[1] == 0xAA
		&& (p[3] & 0x80) == 0x80
		&& ((p[12] * 256 + p[13]) == u16len - 14) // length
		) {
		return E_PKT::PKT_ACT_STD;
	}

	// TWELITE TAG
	//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 -
	// :80000000B10001810043C10032C9047C02AF0A41D2
	//  ^^^^^^^1^2^^^3^^^^^^^4^5^6^7^^^8^^^9^^^a^b
	if (!bAccept
		&& u16len > 14 // at senser data count
		&& p[0] & 0x80 // 
		&& p[7] & 0x80
		&& p[12] != 0x80
	) {
		return E_PKT::PKT_APPTAG;
	}

	// UNKNOWN PKT
	return E_PKT::PKT_ERROR;
}


template <class T>
inline spTwePacket _newTwePacket_parse(uint8_t* p, uint16_t u16len) {
	auto sp = spTwePacket(new T());
	sp->parse(p, u16len);
	return sp;
}

/**
 * @fn	spTwePacket TWEFMT::newTwePacket(uint8_t* p, uint16_t u16len, E_PKT eType = E_PKT::PKT_ERROR)
 *
 * @brief	Creates a new twe packet
 *
 * @param [in,out]	p	 	If non-null, an uint8_t to process.
 * @param 		  	u16len	The length.
 * @param 		  	eType	(Optional) The type.
 *
 * @returns	A spTwePacket.
 */
spTwePacket TWEFMT::newTwePacket(uint8_t* p, uint16_t u16len, E_PKT eType) {
	if (eType == E_PKT::PKT_ERROR) {
		eType = identify_packet_type(p, u16len);
	}

	switch (eType) {
	case E_PKT::PKT_PAL:
		return _newTwePacket_parse<TwePacketPal>(p, u16len);

	case E_PKT::PKT_TWELITE:
		return _newTwePacket_parse<TwePacketTwelite>(p, u16len);

	case E_PKT::PKT_APPIO:
		return _newTwePacket_parse<TwePacketAppIO>(p, u16len);

	case E_PKT::PKT_APPUART:
		return _newTwePacket_parse<TwePacketAppUART>(p, u16len);

	case E_PKT::PKT_ACT_STD:
		return _newTwePacket_parse<TwePacketActStd>(p, u16len);

	case E_PKT::PKT_APPTAG:
		return _newTwePacket_parse<TwePacketAppTAG>(p, u16len);

	default:
		return spTwePacket();
	}
}

/**
 * @fn	spTwePacketPal TWEFMT::newTwePacketPal(uint8_t* p, uint16_t u16len)
 *
 * @brief	Creates a new twe packet palette
 *
 * @param [in,out]	p	 	If non-null, an uint8_t to process.
 * @param 		  	u16len	The 8len.
 *
 * @returns	A spTwePacketPal.
 */
spTwePacketPal TWEFMT::newTwePacketPal(uint8_t* p, uint16_t u16len) {
	auto pobj = std::make_shared<TwePacketPal>();
	pobj->parse(p, u16len);
	return pobj;
}

// parse function of sensors data, for operator >> (Pal???).
// TODO: it does not have datatype check (normally, it does not have any mismatch).
uint32_t TwePacketPal::store_data (uint8_t u8listct, void** vars, 
			const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint16_t* pu8dsList, const uint16_t* pu8exList, uint8_t* pu8exListReads) {
	uint32_t u32store_mask = 0;
	uint8_t* p = 0;

	if (uptr_snsdata) {
		p = uptr_snsdata.get();
	}
	else {
		p = au8snsdata;
	}

	for (int i = 0; i < u8sensors; i++) {
		uint8_t u8dt = G_OCTET(p);
		uint8_t u8ds = G_OCTET(p);
		uint8_t u8ex = G_OCTET(p);
		uint8_t u8ln = G_OCTET(p);

		uint8_t* pNext = p + u8ln;

		for (int j = 0; j < u8listct; j++) {
			uint32_t val = 0;
			
			uint8_t u8ex_mask = (pu8exList[j] & 0xFF00) ? (pu8exList[j] >> 8) : 0xFF;
			if (u8ds == pu8dsList[j] && (pu8exList[j] == 0xffff || (u8ex & u8ex_mask) == (pu8exList[j] & u8ex_mask) )) {
				if (!(u8dt & 0x80)) {
					uint8_t u8ty = u8dt & 0x3;
					uint8_t typ_siz = (u8ty <= 2) ? 1 << u8ty : 1;

					bool bTypeErr = false;

					// type check
					if (u8ty <= 2) {
						if (pu8argsize[j] != typ_siz) break; // type size does not match
						if (pu8argcount_max[j] * typ_siz < u8ln) break; // buffer length does not match
					}

					// match!
					switch (u8dt & 0x03) {
					case 0: case 3: // char
						for (uint8_t k = 0; k < u8ln; k++) ((uint8_t*)vars[j])[k] = G_OCTET(p);
						break;
					case 1: // short 
						for (uint8_t k = 0; k < u8ln/sizeof(uint16_t); k++) ((uint16_t*)vars[j])[k] = G_WORD(p);
						break;
					case 2: // long
						for (uint8_t k = 0; k < u8ln/sizeof(uint32_t); k++) ((uint32_t*)vars[j])[k] = G_DWORD(p);
						break;
					}

					if (pu8exListReads) pu8exListReads[j] = u8ex;

					u32store_mask |= (1 << j);
					break;
				}
			}
		}

		p = pNext;
	}

	return u32store_mask;
}

/**
 * @fn	uint16_t TwePacketPal::query_volt ()
 *
 * @brief	Queries the module voltage
 *
 * @returns	The volt.
 */
std::pair<bool, uint16_t> TwePacketPal::query_volt () {
	uint16_t u16volt = 0;

	// find data
	void* argList[] = { &u16volt };
	const uint8_t  au8argsiz[] = { 2    };
	const uint8_t  au8argctm[] = { 1    };
	const uint16_t au8dsList[] = { 0x30 };
	const uint16_t au8exList[] = { 0x08 };

	uint32_t u32StoredMask = store_data(1, argList, au8argsiz, au8argctm, au8dsList, au8exList);

	return std::pair<bool, uint16_t>(u32StoredMask!=0, u16volt);
}


/**
 * @fn	PalEvent TwePacketPal::query_event()
 *
 * @brief	Queries the event
 *
 * @returns	The event.
 */
std::pair<bool, PalEvent> TwePacketPal::query_event() {
	uint32_t val;

	void* argList[] = { &val };
	const uint8_t  au8argsiz[] = { 4 };
	const uint8_t  au8argctm[] = { 1 };
	const uint16_t au8dsList[] = { 0x5 };
	const uint16_t au8exList[] = { 0xFFFF };
	uint8_t au8exListRead[1];

	uint32_t u32StoredMask = store_data(1, argList, au8argsiz, au8argctm, au8dsList, au8exList, au8exListRead);

	// prepare return value
	PalEvent ret{};
	if (u32StoredMask) {
		ret.b_stored = true;
		ret.u8event_id = val >> 24;
		ret.u32event_param = val & 0x00FFFFFF;
		ret.u8event_source = au8exListRead[0];
	}

	// returns
	return std::pair<bool, PalEvent>(u32StoredMask != 0, ret);
}

/// <summary>
/// to PalMag.
/// </summary>
/// <param name="out"></param>
/// <returns></returns>
PalMag& TwePacketPal::operator >> (PalMag& out) {
	out.u32StoredMask = 0;

	if (u8palpcb != E_PAL_PCB::MAG) {
		return out;
	}

	// default data
	out.u16Volt = 0;
	out.u8MagStat = 0xFF;

	// find data
	void* argList[] = { &out.u16Volt, &out.u8MagStat };
	const uint8_t  au8argsiz[] = { 2,    1    };
	const uint8_t  au8argctm[] = { 1,    1,   };
	const uint16_t au8dsList[] = { 0x30, 0x00 };
	const uint16_t au8exList[] = { 0x08, 0x00 };

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList);

	out.bRegularTransmit = out.u8MagStat & 0x80 ? 1 : 0; // MSB is 
	out.u8MagStat &= 0x7F;

	return out;
}

/// <summary>
/// to PalAmb
/// </summary>
/// <param name="out"></param>
/// <returns></returns>
PalAmb& TwePacketPal::operator >> (PalAmb& out) {
	out.u32StoredMask = 0;

	if (u8palpcb != E_PAL_PCB::AMB) {
		return out;
	}

	// default data
	out.u16Volt = 0xFFFF;
	out.u16Humd = 0xFFFF;
	out.u32Lumi = 0xFFFFFFFF;
	out.i16Temp = 0x7FFF;

	// find data
	void* argList[] = { &out.u16Volt, &out.i16Temp, &out.u16Humd, &out.u32Lumi };
	const uint8_t  au8argsiz[] = { 2,    2,    2,    4 };
	const uint8_t  au8argctm[] = { 1,    1,    1,    1 };
	const uint16_t au8dsList[] = { 0x30, 0x01, 0x02, 0x03 };
	const uint16_t au8exList[] = { 0x08, 0x00, 0x00, 0x00 };

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList);

	return out;
}

/// <summary>
/// to PalAmb
/// </summary>
/// <param name="out"></param>
/// <returns></returns>
PalMot& TwePacketPal::operator >> (PalMot& out) {
	out.u32StoredMask = 0;

	if (u8palpcb != E_PAL_PCB::MOT) {
		return out;
	}

	// default data
	out.u16Volt = 0xFFFF;
	out.u8samples = 0;

	int16_t xyz[16][3];
	
	// find data (may get 16 samples)
	void* argList[] = { &out.u16Volt,    &xyz[0], &xyz[1], &xyz[2], &xyz[3], &xyz[4], &xyz[5], &xyz[6], &xyz[7], &xyz[8], &xyz[9], &xyz[10], &xyz[11],  &xyz[12],  &xyz[13],  &xyz[14],  &xyz[15] };
	const uint8_t  au8argsiz[] = { 2,         2,        2,       2,       2,       2,       2,       2,       2,       2,       2,       2,        2,         2,         2,         2,         2};
	const uint8_t  au8argctm[] = { 1,         3,        3,       3,       3,       3,       3,       3,       3,       3,       3,       3,        3,         3,         3,         3,         3};
	const uint16_t au8dsList[] = { 0x30,      4,        4,       4,       4,       4,       4,       4,       4,       4,       4,       4,        4,         4,         4,         4,         4};
	const uint16_t au8exList[] = { 0x08, 0x0F00,   0x0F01,  0x0F02,  0x0F03,  0x0F04,  0x0F05,  0x0F06,  0x0F07,  0x0F08,  0x0F09,  0x0F0A,   0x0F0B,    0x0F0C,    0x0F0D,    0x0F0E,    0x0F0F};
	uint8_t au8exlist[sizeof(argList)/sizeof(void*)];

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList, au8exlist);

	for (int i = 0; i < 16; i++) {
		if (out.u32StoredMask & (1 << (i + 1))) {
			out.u8samples = i + 1;
			out.u8sample_rate_code = au8exlist[i + 1] >> 4; // sample rate code (0x4=100Hz), same code throu all samples.
			out.i16X[i] = xyz[i][0];
			out.i16Y[i] = xyz[i][1];
			out.i16Z[i] = xyz[i][2];
		}
		else {
			break;
		}
	}
	
	return out;
}
