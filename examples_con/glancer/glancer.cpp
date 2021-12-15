/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/*
 * glancer (console version)
 * 
 *   Parse serial format from App_Wings(or other App_???) and display brief information.
 *   - The format is ASCII style (e.g. :00112233...)
 *   - To minimize dependency of MWM5 library, this project picks minimum set of codes.  
 *     refer to Makefile(APPSRC_CXX,APPSRC_HPP) or VC++ (examples/glancer_con/src_from_mwm5).
 *   
 *   Compile:
 *   - VC++ -> build the project `examples_con/glancer_con'  
 *   - GCC  -> edit the Makefile (CXX:g++ command name, CFLAGS, DEFINES, ...)
 *   
 *   Compile Notices:
 *   - set -DTWE_STDINOUT_ONLY (if skipped, more files are depended to compile.)
 *   - set -DTWE_HAS_MILLIS if you implement millis().  
 *     This is used to store system tickcount[ms] when receiving serial message of a packet.
 *   
 *   Notes:
 *   - Timeout is disabled. 
 *     Timeout is check duration from format header (`:') to closing (`X' or CRLF).
 *   - std::cin and std::cout is used, however you can implement C standard IO (e.g. printf()).
 *     
 */


#include <iostream>
#include <iomanip> 
#include <string>

#include "twe_common.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_fmt.hpp"

using namespace TWE;
using namespace TWEUTILS;
using namespace TWESERCMD;
using namespace TWEFMT;

AsciiParser parse_ascii(256);

void print_unknown(TWEUTILS::SmplBuf_Byte& payl) {
	std::cout << ":MSG=0x";
	for(auto x : payl) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << int(x); 
	}
}

void print_pal(spTwePacket pkt) {
	auto&& pal = refTwePacketPal(pkt);
	PalEvent ev;

	// acquire event data.
	if (pal.has_PalEvent()) {
		ev = pal.get_PalEvent();
	}

	switch(pal.get_PalDataType()) {
		case E_PAL_DATA_TYPE::EVENT_ONLY: 
		{
			if (ev) std::cout << "PAL_EVENT:ID=" << int(ev.u8event_id);
		} break;

		case E_PAL_DATA_TYPE::MAG_STD:
		{
			// generate pal board specific data structure.
			PalMag mag = pal.get_PalMag();

			std::cout << "PAL_MAG";
			std::cout << ":STAT=" << int(mag.u8MagStat);
			std::cout << "(";
			switch(mag.u8MagStat & 0x7F) {
			case 0: std::cout << "NO MANGET"; break;
			case 1: std::cout << "N POLE"; break;
			case 2: std::cout << "S POLE"; break;
			}
			std::cout << ")";
		} break;

		case E_PAL_DATA_TYPE::AMB_STD:
		{
			// generate pal board specific data structure.
			PalAmb amb = pal.get_PalAmb();

			std::cout << "PAL_AMB";
			std::cout << ":TEMP=" << (double)amb.i16Temp / 100.0;
			std::cout << ":HUMD=" << int((amb.u16Humd + 50) / 100);
			std::cout << ":LUMI=" << amb.u32Lumi;
		} break;

		case E_PAL_DATA_TYPE::MOT_STD:
		{
			// generate pal board specific data structure.
			PalMot mot = pal.get_PalMot();

			std::cout << "PAL_MOT";
			std::cout << ":SAMPLES=" << int(mot.u8samples);
			std::cout << ":SR=" << int(mot.u8sample_rate_code);
			for (int i = 0; i < mot.u8samples; i++) {
				if (mot.u32StoredMask & (1UL << (i + 1))) { // check if stored properly.
					std::cout << ":" << i << "(" << mot.i16X[i];
					std::cout << "," << mot.i16Y[i];
					std::cout << "," << mot.i16Z[i];
					std::cout << ")";
				}
			}
		} break;

		case E_PAL_DATA_TYPE::EX_CUE_STD:
		{
			// generate TWELITE CUE standard data
			TweCUE cue = pal.get_TweCUE();

			std::cout << "TWELITE_CUE";

			// extended header
			std::cout << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";

			// event data
			if (ev) std::cout << ":EVENT=" << int(ev.u8event_id);

			// volt
			if (cue.has_vcc()) {
				std::cout << ":VCC=" << int(cue.get_vcc_i16mV());
			}

			// adc1
			if (cue.has_adc1()) {
				std::cout << ":AD1=" << int(cue.get_adc1_i16mV());
			}

			// mag
			if (cue.has_mag()) {
				std::cout << ":MAG=";
				std::cout << "(";
				switch (cue.get_mag_stat_u8() & 0x7F) {
				case 0: std::cout << "NO MANGET"; break;
				case 1: std::cout << "N POLE"; break;
				case 2: std::cout << "S POLE"; break;
				}
				std::cout << ")";
			}

			// mot
			if (cue.has_accel()) {
				std::cout << "MOT";
				std::cout << ":SAMPLES=" << int(cue.get_accel_count_u8());
				std::cout << ":SR=" << int(cue.u8sample_rate_code);
				for (int i = 0; i < cue.get_accel_count_u8(); i++) {
					std::cout << ":" << i << "(" << cue.get_accel_X_i16mG(i);
					std::cout << "," << cue.get_accel_Y_i16mG(i);
					std::cout << "," << cue.get_accel_Z_i16mG(i);
					std::cout << ")";
				}
			}
		} break;

		case E_PAL_DATA_TYPE::EX_ARIA_STD:
		{
			// generate TWELITE ARIA standard data
			TweARIA aria = pal.get_TweARIA();

			std::cout << "TWELITE_ARIA";

			// extended header
			std::cout << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";

			// event data
			if (ev) std::cout << ":EVENT=" << int(ev.u8event_id);

			// volt
			if (aria.has_vcc()) {
				std::cout << ":VCC=" << int(aria.get_vcc_i16mV());
			}

			// adc1
			if (aria.has_adc1()) {
				std::cout << ":AD1=" << int(aria.get_adc1_i16mV());
			}

			// mag
			if (aria.has_mag()) {
				std::cout << ":MAG=";
				std::cout << "(";
				switch (aria.get_mag_stat_u8() & 0x7F) {
				case 0: std::cout << "NO MANGET"; break;
				case 1: std::cout << "N POLE"; break;
				case 2: std::cout << "S POLE"; break;
				}
				std::cout << ")";
			}

			// temp
			if (aria.has_temp()) {
				std::cout << ":TEMP=" << (aria.get_temp_i16_100xC() / 100.0) << "C";
				
			}

			// humid
			if (aria.has_humidity()) {
				std::cout << ":HUMID=" << (aria.get_humidity_u16_100xPC() / 100.0) << "%";
			}
		} break;
	}
}

void print_app_uart(spTwePacket pkt) {
	auto&& aur = refTwePacketAppUART(pkt);

	std::cout << "App_UART";
	auto p = aur.payload.begin();
	
	// display the rest as hex.
	std::cout << ":MSG=0x";
	while(p != aur.payload.end()) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << int(*p); 
		++p;
	}
}

void print_act(spTwePacket pkt) {
	auto&& act = refTwePacketActStd(pkt);

	if (act.payload.length() >= 5) { // should have 5 bytes or more (FOURCC, DATA ...)
		auto p = act.payload.begin();
		// display four cc
		std::cout << "Act[";
		for (int i = 0; i < 4; ++i, ++p) std::cout << char(*p);
		std::cout << "]=0x";

		// display the rest as hex.
		while(p != act.payload.end()) {
			std::cout << std::hex << std::setw(2) << std::setfill('0') << int(*p); 
			++p;
		}
	}
}

void print_app_twelite(spTwePacket pkt) {
	auto&& atw = refTwePacketTwelite(pkt);

	std::cout << "App_Twelite";
	std::cout << ":DI1..4=" << char(atw.DI1 ? 'L' : 'H') << char(atw.DI2 ? 'L' : 'H') << char(atw.DI3 ? 'L' : 'H') << char(atw.DI4 ? 'L' : 'H');
	std::cout << ":AI1..4=(";
	if (atw.Adc_active_mask & (1 << 0)) {
		std::cout << std::setw(4) << std::setfill('0') << atw.u16Adc1;
	} else std::cout << "----";
	std::cout << ',';
	if (atw.Adc_active_mask & (1 << 1)) {
		std::cout << std::setw(4) << std::setfill('0') << atw.u16Adc2;
	} else std::cout << "----";
	std::cout << ',';
	if (atw.Adc_active_mask & (1 << 2)) {
		std::cout << std::setw(4) << std::setfill('0') << atw.u16Adc3;
	} else std::cout << "----";
	std::cout << ',';
	if (atw.Adc_active_mask & (1 << 3)) {
		std::cout << std::setw(4) << std::setfill('0') << atw.u16Adc4;
	} else std::cout << "----";
	std::cout << ')';
}

void print_app_io(spTwePacket pkt) {
	auto&& aio = refTwePacketAppIO(pkt);

	std::cout << "App_IO";
	std::cout << ":DI1..8=";
	for (int i = 0; i < 8; i++) {
		std::cout << char((aio.DI_mask & (1 << i)) ? 'L' : 'H');
	}
	
	std::cout << ":DI_ACTIVE1..8=";
	for (int i = 0; i < 8; i++) {
		std::cout << char((aio.DI_active_mask & (1 << i)) ? 'L' : 'H');
	}
	
	std::cout << ":DI_INT1..8=";
	for (int i = 0; i < 8; i++) {
		std::cout << char((aio.DI_int_mask & (1 << i)) ? 'L' : 'H');
	}
}

void print_app_tag(spTwePacket pkt) {
	auto&& tag = refTwePacketAppTAG(pkt);

	std::cout << "App_TAG";
	std::cout << ":SENSOR=" << int(tag.u8sns);
	
	std::cout << ":DATA=0x";
	for (auto x : tag.payload) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << int(x); 
	}
}

int main() {
	int c;
	while ((c = std::cin.get()) >= 0) {
		if (c == '\r' || c == '\n') {
			parse_ascii << char_t(0x0d); // always 0x0d (this parser does not check the following 0x0a)
		}
		else {
			parse_ascii << char_t(c);
		}

		if (parse_ascii) {
			std::cout << "PKT";
			auto&& payl = parse_ascii.get_payload(); // payload data array
			auto&& pkt = newTwePacket(payl);         // packet object (contains minimum information)
			auto&& typ = identify_packet_type(pkt);  // packet type
			std::cout << ":Typ=" << int(typ);

			if (typ != E_PKT::PKT_ERROR) {
				std::cout << ":"
					<< "Lq="    << std::dec << std::setw(2) << std::setfill('0') << int(pkt->common.lqi)
					<< ":Ad=0x" << std::hex << std::setw(8) << std::setfill('0') << uint32_t(pkt->common.src_addr)
					<< "(0x"    << std::hex << std::setw(2) << std::setfill('0') << int(pkt->common.src_lid) << ")"
					<< ":Vmv="  << std::dec << std::setw(4) << std::setfill('0') << int(pkt->common.volt)
					<< ":Tms="  << std::dec << std::setw(2) << std::setfill('0') << int(pkt->common.tick % 10000)
					<< ":"
					;
			}

			switch (typ) {
			case E_PKT::PKT_PAL: print_pal(pkt); break;
			case E_PKT::PKT_ACT_STD: print_act(pkt); break;
			case E_PKT::PKT_TWELITE: print_app_twelite(pkt); break;
			case E_PKT::PKT_APPIO: print_app_io(pkt); break;
			case E_PKT::PKT_APPUART: print_app_uart(pkt); break;
			case E_PKT::PKT_APPTAG: print_app_tag(pkt); break;
			default: print_unknown(payl); // e.g. UART message of App_Twelite, App_UART(simple format), or corrupted.
			}

			std::cout << std::endl;
		}
	}

	return 0;
}

/// implement millis()
#if defined(_MSC_VER) || defined(__MINGW32__)
#include "windows.h"
# pragma comment(lib, "secur32.lib")
# pragma comment(lib, "winmm.lib")
# pragma comment(lib, "dmoguids.lib")
# pragma comment(lib, "wmcodecdspuuid.lib")
# pragma comment(lib, "msdmo.lib")
# pragma comment(lib, "Strmiids.lib")
#elif defined(__APPLE__) || defined(__linux)
#include <sys/time.h>
#endif

uint32_t millis() {
#if defined(_MSC_VER) || defined(__MINGW32__)
	return (uint32_t)timeGetTime();
#elif defined(__APPLE__) || defined(__linux)
	timeval time;
	gettimeofday(&time, NULL);
	long ms = (time.tv_sec * 1000) + (time.tv_usec / 1000);
	return (uint32_t)ms;
#else
	# warning "no u32GetTick_ms() implementation."
	return 0;
#endif
}