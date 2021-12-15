/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_fmt_pal.hpp"
#include "twe_fmt_stdin.h"

using namespace TWEFMT;
using namespace TWEUTILS;

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

	// PAL pcb & data format
	c = G_OCTET(p);
	e_board = (E_PAL_PCB)(c & 0x1F);
	// c >>= 5;
	// c = ((c & 1) << 2) | (c & 2) | ((c & 4) >> 2); // reverse 3bits
	u8datafmt = c & 0x80 ? 1 : 0; // 0: with PalDataInfo, 1: standard

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

	// store volt data
	if (!bErr) {
		auto res_volt = query_volt();
		if (res_volt.first) {
			TwePacket::common.volt = res_volt.second;
		}
	}

	// query data info
	if (!bErr) {
		if (DataPal::has_data_info()) {
			auto res_data_info = query_data_info();

			if (res_data_info.first) {
				static_cast<PalDataInfo&>(*this) = res_data_info.second;
			}
			else {
				bErr = true;
			}
		}
	}

	// query event data
	if (!bErr) {
		if (!DataPal::has_data_info() || (DataPal::has_data_info() && PalDataInfo::_b_stored_pal_event)) {
			auto res_ev = query_event();

			// flag is set, but no data (should be an error)
			if (DataPal::has_data_info() && PalDataInfo::_b_stored_pal_event && !res_ev.first) {
				bErr = true;
			}

			// copy data if present
			if (!bErr) {
				if (res_ev.first) {
					PalDataInfo::_b_stored_pal_event = true;
					static_cast<PalEvent&>(*this) = res_ev.second;
				}
				else {
					PalDataInfo::_b_stored_pal_event = false;
				}
			}
		}
	}

	// return packet ID
	return bErr ? E_PKT::PKT_ERROR : E_PKT::PKT_PAL;
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

/**
 * @fn	uint32_t TwePacketPal::store_data(uint8_t u8listct, void** vars, const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint16_t* pu8dsList, const uint16_t* pu8exList, uint8_t* pu8exListReads)
 *
 * @brief	parse function of sensors data, for operator >> (Pal???).
 *
 * @param 		  	u8listct	   	The 8listct.
 * @param [in,out]	vars		   	If non-null, the variables.
 * @param 		  	pu8argsize	   	The pu 8argsize.
 * @param 		  	pu8argcount_max	The pu 8argcount maximum.
 * @param 		  	pu8dsList	   	List of pu 8ds. (note: uint16_t, higher byte is used as maximum data count)
 * @param 		  	pu8exList	   	List of pu 8exes. (note: uint16_t, 0xFFFF means any exdata)
 * @param [in,out]	pu8exListReads 	If non-null, set ex_byte number of read item.
 * @param [in,out]	pu8bytesReads 	If non-null, set read byte count.
 *
 * @returns	An uint32_t where bit 'i' is set when list[i] is successfully read.
 */
uint32_t TwePacketPal::store_data(uint8_t u8listct, void** vars,
		const uint8_t* pu8argsize, const uint8_t* pu8argcount_max, const uint16_t* pu8dsList, const uint16_t* pu8exList,
		uint8_t* pu8exListReads, uint8_t* pu8ReadDataCount) {
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
			if (u8ds == pu8dsList[j] && (pu8exList[j] == 0xffff || (u8ex & u8ex_mask) == (pu8exList[j] & u8ex_mask))) {
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
						if (pu8ReadDataCount) pu8ReadDataCount[j] = u8ln;
						break;
					case 1: // short 
						for (uint8_t k = 0; k < u8ln / sizeof(uint16_t); k++) ((uint16_t*)vars[j])[k] = G_WORD(p);
						if (pu8ReadDataCount) pu8ReadDataCount[j] = u8ln;
						break;
					case 2: // long
						for (uint8_t k = 0; k < u8ln / sizeof(uint32_t); k++) ((uint32_t*)vars[j])[k] = G_DWORD(p);
						if (pu8ReadDataCount) pu8ReadDataCount[j] = u8ln;
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
std::pair<bool, PalDataInfo> TwePacketPal::query_data_info() {
	uint8_t param[8]; // max 8bytes (normally 3bytes)
	uint8_t param_len;

	PalDataInfo data;

	// find data
	void* argList[] = { param };
	const uint8_t  au8argsiz[] = { 1 };
	const uint8_t  au8argctm[] = { sizeof(param) };
	const uint16_t au8dsList[] = { 0x34 };
	const uint16_t au8exList[] = { 0x00 };

	uint32_t u32StoredMask = store_data(1, argList, au8argsiz, au8argctm, au8dsList, au8exList, nullptr, &param_len);

	if (u32StoredMask != 0) {
		if (param_len == 3) {
			data.u8data_type = param[0] & 0x7F;
			data._b_stored_pal_event = (param[0] & 0x80) != 0;
			data.u8_data_source = param[1];
			data.u8_data_cause = param[2];
		}
	}

	return std::pair<bool, PalDataInfo>(u32StoredMask != 0, data);
}

/**
 * @fn	uint16_t TwePacketPal::query_volt ()
 *
 * @brief	Queries the module voltage
 *
 * @returns	The volt.
 */
std::pair<bool, uint16_t> TwePacketPal::query_volt() {
	uint16_t u16volt = 0;

	// find data
	void* argList[] = { &u16volt };
	const uint8_t  au8argsiz[] = { 2 };
	const uint8_t  au8argctm[] = { 1 };
	const uint16_t au8dsList[] = { 0x30 };
	const uint16_t au8exList[] = { 0x08 };

	uint32_t u32StoredMask = store_data(1, argList, au8argsiz, au8argctm, au8dsList, au8exList);

	return std::pair<bool, uint16_t>(u32StoredMask != 0, u16volt);
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
		ret.u8event_id = val >> 24;
		ret.u32event_param = val & 0x00FFFFFF;
		ret.u8event_source = au8exListRead[0];
	}

	// returns
	return std::pair<bool, PalEvent>(u32StoredMask != 0, ret);
}

/**
 * @fn	PalMag& TwePacketPal::operator>> (PalMag& out)
 *
 * @brief	acquire PalMag data set.
 *
 * @param [in,out]	out	The out.
 *
 * @returns	The shifted result.
 */
PalMag& TwePacketPal::operator >> (PalMag& out) {
	out.u32StoredMask = 0;

	if (e_board != E_PAL_PCB::MAG) {
		return out;
	}

	// default data
	out.u16Volt = 0;
	out.u8MagStat = 0xFF;

	// find data
	void* argList[] = { &out.u16Volt, &out.u8MagStat };
	const uint8_t  au8argsiz[] = { 2,    1 };
	const uint8_t  au8argctm[] = { 1,    1, };
	const uint16_t au8dsList[] = { 0x30, 0x00 };
	const uint16_t au8exList[] = { 0x08, 0x00 };

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList);

	out.bRegularTransmit = out.u8MagStat & 0x80 ? 1 : 0; // MSB is 
	out.u8MagStat &= 0x7F;

	return out;
}

/**
 * @fn	PalAmb& TwePacketPal::operator>> (PalAmb& out)
 *
 * @brief	acquire PalAmb standard data set.
 * 			
 * @param [in,out]	out	The out.
 *
 * @returns	The shifted result.
 */

PalAmb& TwePacketPal::operator >> (PalAmb& out) {
	out.u32StoredMask = 0;

	if (e_board != E_PAL_PCB::AMB) {
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

/**
 * @fn	PalMot& TwePacketPal::operator>> (PalMot& out)
 *
 * @brief	acquire PalMOT standard data set.
 *
 * @param [in,out]	out	The out.
 *
 * @returns	The shifted result.
 */
PalMot& TwePacketPal::operator >> (PalMot& out) {
	out.u32StoredMask = 0;

	if (e_board != E_PAL_PCB::MOT) {
		return out;
	}

	// default data
	out.u16Volt = 0xFFFF;
	out.u8samples = 0;

	int16_t xyz[PalMot::MAX_SAMPLES][3];

	// find data (may get 16 samples)
	void* argList[] = { &out.u16Volt,    &xyz[0], &xyz[1], &xyz[2], &xyz[3], &xyz[4], &xyz[5], &xyz[6], &xyz[7], &xyz[8], &xyz[9], &xyz[10], &xyz[11],  &xyz[12],  &xyz[13],  &xyz[14],  &xyz[15] };
	const uint8_t  au8argsiz[] = { 2,         2,        2,       2,       2,       2,       2,       2,       2,       2,       2,       2,        2,         2,         2,         2,         2 };
	const uint8_t  au8argctm[] = { 1,         3,        3,       3,       3,       3,       3,       3,       3,       3,       3,       3,        3,         3,         3,         3,         3 };
	const uint16_t au8dsList[] = { 0x30,      4,        4,       4,       4,       4,       4,       4,       4,       4,       4,       4,        4,         4,         4,         4,         4 };
	const uint16_t au8exList[] = { 0x08, 0x0F00,   0x0F01,  0x0F02,  0x0F03,  0x0F04,  0x0F05,  0x0F06,  0x0F07,  0x0F08,  0x0F09,  0x0F0A,   0x0F0B,    0x0F0C,    0x0F0D,    0x0F0E,    0x0F0F };
	uint8_t au8exlist[sizeof(argList) / sizeof(void*)];

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList, au8exlist);

	for (int i = 0; i < PalMot::MAX_SAMPLES; i++) {
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

/**
 * @fn	TweCUE& TwePacketPal::operator>> (TweCUE& out)
 *
 * @brief	acquire TweCUE standard data set.
 *
 * @param [in,out]	out	The out.
 *
 * @returns	The shifted result.
 */
TweCUE& TwePacketPal::operator >> (TweCUE& out) {
	out.u32StoredMask = 0;

	if (e_board != E_PAL_PCB::CUE) {
		return out;
	}

	// basic defs
	static const uint8_t VOLT = uint8_t(E_SNSCD::VOLT);
	static const uint8_t VCC = uint8_t(E_EXCD_VOLT::POWER);
	static const uint8_t ADC1 = uint8_t(E_EXCD_VOLT::ADC1);

	// default data
	out.u16Volt = 0xFFFF;
	out.u8samples = 0;
	int16_t xyz[TweCUE::MAX_SAMPLES][3];

	// find data
	void* argList[]  {     &out.u16Volt, &out.u16Adc1, &out.u8MagStat, &xyz[0], &xyz[1], &xyz[2], &xyz[3], &xyz[4], &xyz[5], &xyz[6], &xyz[7], &xyz[8], &xyz[9], };
	const uint8_t  au8argsiz[] = {    2,            2,              1,       2,       2,       2,       2,       2,       2,       2,       2,       2,       2, };
	const uint8_t  au8argctm[] = {    1,            1,              1,       3,       3,       3,       3,       3,       3,       3,       3,       3,       3, };
	const uint16_t au8dsList[] = { VOLT,         VOLT,              0,       4,       4,       4,       4,       4,       4,       4,       4,       4,       4, };
	const uint16_t au8exList[] = {  VCC,         ADC1,              0,  0x0F00,  0x0F01,  0x0F02,  0x0F03,  0x0F04,  0x0F05,  0x0F06,  0x0F07,  0x0F08,  0x0F09, };
	uint8_t au8exlist[sizeof(argList) / sizeof(void*)];

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList, au8exlist);

	// magstat
	out.bMagRegularTransmit = out.u8MagStat & 0x80 ? 1 : 0;
	out.u8MagStat &= 0x7F;

	// accel
	uint32_t mask = TweCUE::STORE_ACCELERO_MASK;
	for (int i = 0; i < TweCUE::MAX_SAMPLES; i++, mask <<= 1) {
		if (out.u32StoredMask & mask) {
			out.u8samples = i + 1;
			out.u8sample_rate_code = au8exlist[i + 3] >> 4; // sample rate code (0x4=100Hz), same code throu all samples.
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

/**
 * @fn	TweARIA& TwePacketPal::operator>> (TweARIA& out)
 *
 * @brief	acquire TweARIA standard data set.
 *
 * @param [in,out]	out	The out.
 *
 * @returns	The shifted result.
 */
TweARIA& TwePacketPal::operator >> (TweARIA& out) {
	out.u32StoredMask = 0;

	if (e_board != E_PAL_PCB::ARIA) {
		return out;
	}

	// basic defs
	static const uint8_t VOLT = uint8_t(E_SNSCD::VOLT);
	static const uint8_t VCC = uint8_t(E_EXCD_VOLT::POWER);
	static const uint8_t ADC1 = uint8_t(E_EXCD_VOLT::ADC1);

	// default data
	out.u16Volt = 0xFFFF;
	// ambstat
	out.i16Temp = 0x7FFF;
	out.u16Humd = 0xFFFF;

	// magstat
	out.bMagRegularTransmit = out.u8MagStat & 0x80 ? 1 : 0;
	out.u8MagStat &= 0x7F;

	// find data
	void* argList[]{ &out.u16Volt, &out.u16Adc1, &out.u8MagStat, &out.i16Temp, &out.u16Humd };
	const uint8_t  au8argsiz[] = { 2,		2,		1,		2,		2, };
	const uint8_t  au8argctm[] = { 1,		1,		1,		1,		1, };
	const uint16_t au8dsList[] = { VOLT,	VOLT,	0,		1,		2, };
	const uint16_t au8exList[] = { VCC,		ADC1,	0,		0,		0, };
	uint8_t au8exlist[sizeof(argList) / sizeof(void*)];

	out.u32StoredMask = store_data(out.U8VARS_CT, argList, au8argsiz, au8argctm, au8dsList, au8exList, au8exlist);

	return out;
}