/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#ifdef _MSC_VER
#include "conio.h"
#include "windows.h"

#include <stdio.h>

#include "twe_common.hpp"

#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_sercmd_binary.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_fmt.hpp"
#include "twe_csettings.hpp"

#include "twe_utils_fixedque.hpp"

#include "twesettings/twecommon.h"
#include "twesettings/tweutils.h"
#include "twesettings/tweserial.h"
#include "twesettings/tweprintf.h"
#include "twesettings/twecrc8.h"

#include "twesettings/twesercmd_gen.h"
#include "twesettings/twesercmd_plus3.h"
#include "twesettings/tweinputstring.h"
#include "twesettings/twestring.h"

#include "twesettings/twesettings.h"
#include "twesettings/twesettings_std.h"
#include "twesettings/twesettings_cmd.h"
#include "twesettings/twesettings_validator.h"

#include "twesettings/twesettings_std_defsets.h"
#include "twesettings/tweinteractive.h"
#include "twesettings/tweinteractive_defmenus.h"
#include "twesettings/tweinteractive_settings.h"
#include "twesettings/tweinteractive_nvmutils.h"
#include "twesettings/twenvm.h"

#include "twesettings/msc_sys.h"

#include <memory>
#include <vector>
#include <map>
#include <list>
#include <valarray>

 //using namespace TWE;
using namespace TWEUTILS;
using namespace TWESERCMD;
using namespace TWETERM;
using namespace TWESYS;
using namespace TWEFMT;

/* Hardware related */
// extern "C" volatile uint32_t u32TickCount_ms; //! TWENET compatibility

/* serial functions */
static int TWETERM_iPutC(int c) { return _putch(c); }
static int TWETERM_iGetC() { if (_kbhit()) return _getch(); else return -1; }

/// <summary>
/// system time
/// </summary>
//uint32_t TWESYS::u32GetTick_ms() { return u32TickCount_ms; } // no timeout

 /// <summary>
 /// stdio out
 /// </summary>
class TWE_PutChar_STDIO : public TWE::IStreamOut {
	FILE* _fp;
public:
	TWE_PutChar_STDIO(FILE* fp) : _fp(fp) {}
	TWE_PutChar_STDIO(const TWE_PutChar_STDIO& o) { _fp = o._fp; }
	inline TWE::IStreamOut& operator ()(char_t c) { fputc(c, _fp); fflush(_fp); return *this; }
};

class TWE_PutChar_CONIO : public TWE::IStreamOut {
public:
	TWE_PutChar_CONIO() {}
	inline TWE::IStreamOut& operator ()(char_t c) { TWETERM_iPutC(c); return *this; }
};

/// <summary>
/// HEX dump
/// </summary>
class TWE_PutHexDump_STDIO : public TWE::IStreamOut {
	FILE* _fp;
	uint8_t _u8ct;
public:
	TWE_PutHexDump_STDIO(FILE* fp) : _fp(fp), _u8ct(0) {}
	TWE_PutHexDump_STDIO(const TWE_PutHexDump_STDIO& o) {
		_fp = o._fp;
		_u8ct = o._u8ct;
	}
	inline TWE::IStreamOut& operator ()(char_t c) {
		_u8ct++;
		fprintf(_fp, "%02X ", c);
		fflush(_fp);
		return *this;
	}
};

/// <summary>
/// conio input class
/// </summary>
class TWE_InputChar_CONIO : public TWE::IStreamIn {
public:
	inline int get_a_byte() {
		return TWETERM_iGetC();
	}
};

// C TWESETTINGS
/*
TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg) {
	return 0;
}

TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	return 0;
}

TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	return 0;
}
*/

class TWETerm_WinConsole_SCRATCH : public TWETERM::ITerm {
	TWE_PutChar_CONIO& _of_putchar;
public:
	TWETerm_WinConsole_SCRATCH(uint8_t u8c, uint8_t u8l, SimpBuf_GChar* pAryLines, GChar* pBuff, TWE_PutChar_CONIO of_putchar) :
		ITerm(u8c, u8l, pAryLines, pBuff), _of_putchar(of_putchar) {}

	void refresh() {
		if (dirtyLine) {
			const int B = 256;
			char fmt[B];

			if (dirtyLine.is_full()) {
				_cputs("\033[2J\033[H"); // clear all screen and HOME
			}
			for (int i = 0; i <= max_line; i++) {
				if (dirtyLine.is_dirty(i)) {
					sprintf_s(fmt, "\033[%d;%dH", i + 1, 1); // move cursor
					_cputs(fmt);

					int j = calc_line_index(i);
					memset(fmt, 0x20, sizeof(fmt));

					GChar* p = astr_screen[j].begin().raw_ptr();
					GChar* e = astr_screen[j].end().raw_ptr();
					int ifmt = 0;
					while (p < e) {
						if (*p < 0x80) {
							fmt[ifmt++] = (char)*p;
						}
						else {
							fmt[ifmt++] = (*p & 0xFF00) >> 8;
							fmt[ifmt++] = *p & 0xFF;
						}
						p++;
					}
					fmt[ifmt] = 0;

					_cputs(fmt);
				}
			}

			sprintf_s(fmt, "\033[%d;%dH123456789 223456789 323456789 423456789 523456789 623456789 723456789 823456789X", max_line + 2, 1); // move cursor
			_cputs(fmt);

			int c_vis = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
			sprintf_s(fmt, "\033[%d;%dH", cursor_l + 1, c_vis + 1); // move cursor
			_cputs(fmt);
		}
		dirtyLine.clear();
	}
};

/* DEBUG SCALE (80COLUMNS)
123456789 223456789 323456789 423456789 523456789 623456789 723456789 823456789X
*/

void console_test() {
	// console defs
	const uint8_t U8COL = 53;
	const uint8_t U8LINE = 24;

	TWE_PutChar_CONIO screen_io; // stream def

	GChar screen_buf[U8COL * U8LINE]; // raw screen buffer.
	SimpBuf_GChar screen_lines[U8LINE]; // line buffer
	TWETerm_WinConsole_SCRATCH the_screen(U8COL, U8LINE, screen_lines, screen_buf, screen_io); // init the screen.
	the_screen.clear_screen(); // clear screen at the initial

	// serial parser (ASCII)
	static uint8_t auBuff_a[256];
	SmplBuf_Byte buff_a = SmplBuf_Byte(auBuff_a, 0, sizeof(auBuff_a));
	AsciiParser parse_ascii(buff_a);

	bool bExit = false;
	while (!bExit) {
		int c;

		Sleep(20);
		the_screen.refresh();

		u32TickCount_ms += 20;
		while ((c = TWETERM_iGetC()) >= 0)
		{
			bool bHandled = false;
			switch (c) {
			case 0x0c: // Ctrl+L
				the_screen.clear_screen();
				break;
			case 0x12: // Ctrl+R
				the_screen.force_refresh();
				break;
			case 0x0d: // CR (Enter key)
				//the_screen << (uint8)'\r';
				//the_screen << (uint8)'\n';
				the_screen << TWE::Endl; // IStreamOut interface allows IStreamSpecial object, like TWE::endl.
				break;
			case '~':
				the_screen << (wchar_t)0x88AB;
				break;
			case '!':
				the_screen << TermAttr(TERM_COLOR_BG_BLUE | TERM_COLOR_FG_WHITE | TERM_BOLD | TERM_REVERSE)
					<< TWE::printfmt("->%02X%c", 2, true ? '*' : ' ')
					<< TermAttr(TERM_ATTR_OFF);
				break;
			case 'q':
				fPrintf(the_screen, "\r\nexit screen test.");
				bExit = true;
				break;
			default:
				// put data as is.
				the_screen << (char_t)c;
				break;
			}

			parse_ascii << (char_t)c;
			if (parse_ascii) {
				fPrintf(the_screen, "\r\nPacket Info: len=%d", parse_ascii.length());
				the_screen << "\r\n-> ";
				parse_ascii >> the_screen; // output the format

				//auto pkt = TWEFMT::identify_packet_type(p, len);
				auto pkt = TWEFMT::identify_packet_type(parse_ascii.get_payload());

				// identified as PAL Packet 
				if (pkt == TWEFMT::E_PKT::PKT_PAL) {
					auto pPay = TWEFMT::newTwePacketPal(parse_ascii.get_payload());

					// check the kind of PAL sensor
					switch (pPay->u8palpcb) {
					case TWEFMT::E_PAL_PCB::MAG:
						if (true) { // for making a scope
							TWEFMT::PalMag mag;
							*pPay >> mag;

							if (mag.u32StoredMask) {
								fPrintf(the_screen, "\r\n-> MAG SW=%d mV=%04d", mag.u8MagStat, mag.u16Volt);
							}
						}
						break;

					case TWEFMT::E_PAL_PCB::AMB:
						if (true) {
							TWEFMT::PalAmb amb;
							*pPay >> amb;

							if (amb.u32StoredMask) {
								fPrintf(the_screen, "\r\n-> AMB T:%d H:%d L:%d mV=%04d", amb.i16Temp, amb.u16Humd, amb.u32Lumi, amb.u16Volt);
							}
						}
						break;
					}
				}
			}
		}
	}
}

#define SHORT_TO_BARY(s) (s&0xFF00) >> 8, s&0xff
namespace TWEFMT {
	uint8_t au8data_pal_mag1[] = {
		0x80, 0, 0, 0, // router
		0x63, // LQI=00
		0x12, 0x34, // SEQ
		0x80, 0x1A, 0x3B, 0x5C, // ID
		0x09, // LID
		0x80, // FIXED
		0x01, // MAG:0x01, AMB: 0x02, MOT:0x03
		0x02, // SENSOR COUNT

		// ENTRY1
		0x01, // uint16_t
		0x30, // voltage
		0x08, // power
		0x02, // 2bytes/sizeof(short) = 1 entry
		SHORT_TO_BARY(2500), // 2500mv

		// ENTRY2
		0x00, // uint8_t
		0x00, // hall ic
		0x00, // (no def)
		0x01, // 1entry
		0x02, // near S pole

		0x00, // CRC8 (need to be calculated)	
	}; // :80000000631234801A3B5C098001020130080209C40000000102E728


	spTwePacket test_twefmt() {
		printf("sizeof struct DataPal = %d\r\n", (int)sizeof(struct DataPal));
		printf("sizeof TwePacketPal = %d\r\n", (int)sizeof(TwePacketPal));

		// update last byte(save correct CRC8 on the tail)
		au8data_pal_mag1[sizeof(au8data_pal_mag1) - 1] =
			TWEUTILS::CRC8_u8Calc(au8data_pal_mag1, sizeof(au8data_pal_mag1) - 1);

		// object create
		std::shared_ptr<TwePacket> palobj1(new TwePacketPal()); // construct with derived class

		std::shared_ptr<TwePacket> palobj2;
		palobj2 = std::make_shared<TwePacketPal>(); // make_shared with derived class

		// identify obj
		E_PKT ePktType = identify_packet_type(au8data_pal_mag1, sizeof(au8data_pal_mag1));

		std::shared_ptr<TwePacket> palobj3; // NULL OBJECT

		if (ePktType == E_PKT::PKT_PAL) {
			auto palobj4 = newTwePacketPal(au8data_pal_mag1, sizeof(au8data_pal_mag1));
			//auto palobj4 = _newTwePacketPal();
			//palobj4->parse(au8data_pal_mag1, sizeof(au8data_pal_mag1));

			// copy object
			palobj3 = palobj4;
		} // palobj4 is destructed.

		// dynamic cast to PAL object.
		auto p_palobj3 = std::dynamic_pointer_cast<TwePacketPal>(palobj3);
		int len = p_palobj3->u8snsdatalen;
		if (len & 0x8000) {
			len &= 0x7FFF;
			for (int i = 0; i < len; i++) {
				printf("%02X ", p_palobj3->uptr_snsdata[i]);
			}
			printf("\r\n");
		}
		else {
			for (int i = 0; i < len; i++) {
				printf("%02X ", p_palobj3->au8snsdata[i]);
			}
			printf("\r\n");
		}

		// test operator >>
		PalMag mag;
		if (p_palobj3->u8palpcb == E_PAL_PCB::MAG) {
			*p_palobj3 >> mag;
			printf("MAG=%X volt=%d \r\n", mag.u8MagStat, mag.u16Volt);
		}

		return p_palobj3;
	}
}

void check_necessary_memory_in_stl() {
	fprintf(stdout, "\r\ncheck sizeof for stl objects:\r\n");

	std::vector<uint32_t> vect1;
	std::map<uint32_t, uint32_t> map1;
	std::list<uint32_t> list1;
	std::valarray<uint32_t> vary1;

	std::unique_ptr<uint32_t> aPtr1(new uint32_t(10));

	std::shared_ptr<std::vector<uint8_t>> pByteV;
	pByteV = std::make_shared<std::vector<uint8_t>>();

	std::tuple<uint8_t, uint8_t> tu_key;

	printf("vector     %d\r\n", (int)sizeof(vect1));
	printf("map        %d\r\n", (int)sizeof(map1));
	printf("list       %d\r\n", (int)sizeof(list1));
	printf("valarray   %d\r\n", (int)sizeof(vary1));
	printf("unique_ptr %d\r\n", (int)sizeof(aPtr1));
	printf("shared_ptr %d\r\n", (int)sizeof(pByteV));
	printf("tuple      %d\r\n", (int)sizeof(tu_key));

	/* resuts
	vector     16
	map        12
	list       12
	valarray   8
	unique_ptr 4
	shared_ptr 8
	tuple      2
	*/
	while (TWETERM_iGetC() < 0);
}

double func1(double f) {
	static double func1_var = 0.0;

	func1_var += f;
	return func1_var;
}

void test_print_float() {
	TWE_PutChar_STDIO obj_vputchar(stdout);

	obj_vputchar << "test_print_float()\n";

	double f = 314.159265;
	fPrintf(obj_vputchar, "%05.f\n", f);

	// obj_vputchar << TWE::printfmt("%f/%d/%c", f, (int)f, 0x31);
	obj_vputchar << TWE::printfmt("%f/%d/%c/%s\n", f, (int)f, 0x31, "abcd");
	obj_vputchar << TWE::printfmt("func %f", func1(1.2));

	while (TWETERM_iGetC() < 0);
}

void test_C_twestgs() {
	char msg[] = "abcdefg";
	TWE_PutChar_STDIO obj_vputchar(stdout);
	TWE_InputChar_CONIO obj_input_con;

	TWE_tsFILE fp;
	C_TWE_printf_support::s_init(&fp, &obj_vputchar, &obj_input_con);

	uint8_t c = TWE_CRC8_u8Calc((uint8_t*)msg, sizeof(msg));
	int i16Char;
	TWE_fprintf(&fp, "using TWE_fprintf in C lib: %02X, %.3f\n", c, 123.456789);
	TWE_fputs("...press q to proceed:", &fp);
	while (1) {
		i16Char = TWE_fgetc(&fp);
		if (i16Char == 'q') break;

		Sleep(20);
	}
	TWE_fputs("\r\n", &fp);
}

void test_parser() {
	static uint8_t auBuff_a[256];
	static uint8_t auBuff_b[256];

	SmplBuf_Byte buff_a(auBuff_a, 0, sizeof(auBuff_a));
	SmplBuf_Byte buff_b = SmplBuf_Byte(auBuff_b, 0, sizeof(auBuff_b));

	TWE_PutChar_STDIO obj_vputchar(stdout);
	TWE_PutHexDump_STDIO obj_hexdump(stdout);

	AsciiParser parse_ascii(buff_a);
	BinaryParser parse_binary(buff_b);

	parse_ascii.set_timeout(1000);

	const uint8_t au8str[] = "das:78811501758100003800284F000C02230000FFFFFFFFFF20\r\ndas:001122339A\r\nsadas:11223344550100\r\nasdas:00X :788115017581000038002785000C05220000FFFFFFFFFFE9\r\n";

#if 0
	fprintf(stdout, "\r\nTWEFMT test:\r\n");
	TWEFMT::spTwePacket obj = TWEFMT::test_twefmt();
	if (obj->get_type() == TWEFMT::E_PKT::PKT_PAL) {
		auto objpal = TWEFMT::castTwePacketPal(obj);
		printf("SER/LID = %08x/%02x\r\n", objpal->u32addr_src, objpal->u8addr_src);
	}
#endif

	{
		obj_vputchar << TWE::printfmt("10=%d, ", 10) << TWE::printfmt("11=%d\n", 11);
		//obj_vputchar << TWE::printfmt("%d%d%d%d%d%d%d%d%d%d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
		obj_vputchar << TWE::printfmt("%d", 12);
	}

	fprintf(stdout, "\r\nAscii mode test:\r\n");
	for (int i = 0; au8str[i] != 0; i++) {
		//parse_ascii._u8Parse(au8str[i]);
		if (parse_ascii << (char_t)(au8str[i])) {
			fputs("\r\n", stdout);
			parse_ascii >> obj_vputchar;

			// try to put as binary
			SmplBuf_Byte b;
			parse_ascii >> b; // works :)

			parse_binary << (char_t)(0xA5);
			parse_binary << (char_t)(0x5A);
			parse_binary << (char_t)b.length();
			uint8_t* p = b.data();
			uint8_t u8xor = 0;
			while (p != b.end().raw_ptr()) {
				u8xor ^= *p;
				parse_binary << (char_t)*p;
				p++;
			}
			parse_binary << (char_t)u8xor;

			// should be completed the format
			if (parse_binary) {
				parse_binary >> obj_hexdump;
			}
		}
	}

	fputs("\r\n", stdout);
	while (TWETERM_iGetC() < 0);
}

void test_parser2() {
	static uint8_t auBuff_a[256];
	SmplBuf_Byte buff_a(auBuff_a, 0, sizeof(auBuff_a));

	TWE_PutChar_STDIO obj_vputchar(stdout);

	AsciiParser parse_ascii(buff_a);

	bool bExit = false;
	while (!bExit) {
		int c;
		Sleep(20);
		u32TickCount_ms += 20;
		while ((c = TWETERM_iGetC()) >= 0) {
			// echo back
			if (c >= 0 && c < 0x7F) {
				TWETERM_iPutC(c);
			}

			// exit
			if (c == 'q') {
				bExit = true;
			}
			else {
				if (c >= 0) {
					if (parse_ascii << (char_t)c) { // works :)
						fputs("\r\n", stdout);
						parse_ascii >> obj_vputchar;

						fputs("\r\n", stdout);
						for (int i = 0; i < parse_ascii.length(); i++) {
							fprintf(stdout, "%x:", parse_ascii[i]);
						}
						fputs("\r\n", stdout);
					}
				}
			}
		}
	}
}

void test_fixed_queue() {
	fprintf(stdout, "\r\ntest the fixed size queue:\r\n");

	TWEUTILS::FixedQueue<char_t> cue(10);
	fprintf(stdout, "cue size = %d\n", (int)cue.size());

	for (int i = 0; i < 16; i++) {
		cue.push(char_t(i));
		// fprintf(stdout, "cue push %d = %d\n", i, res);
	}

	while (!cue.empty()) {
		char_t c = cue.front();
		cue.pop();

		fprintf(stdout, "cue pop = %d\n", c);
	}

	while (TWETERM_iGetC() < 0);
}


#include "payload_util.hpp"
void test_payload_util() {
	uint8_t u8Payload[] = "1234534abcde";

	struct {
		uint8 d1;
		uint32 d2;
		uint16 d3;
		uint8 d4[3];
		uint8 d5[100];

	} sDat;

	const uint8* ret;
	ret = TWEUTILS::expand_bytes(u8Payload, u8Payload + sizeof(u8Payload), sDat.d5);

	ret = TWEUTILS::expand_bytes(u8Payload, u8Payload + sizeof(u8Payload),
		sDat.d1, sDat.d2, sDat.d3, sDat.d4);

	memset(u8Payload, 0, sizeof(u8Payload));

	ret = TWEUTILS::pack_bytes(u8Payload, u8Payload + sizeof(u8Payload),
		sDat.d1, sDat.d2, sDat.d3, sDat.d4);

	while (TWETERM_iGetC() < 0);
}

/**********************************************************
 * test_retTwePacket_null_obj()
 *   refTwePacket() に empty のspTwePacketを与えたときの振る舞い
 **********************************************************/

void test_retTwePacket_null_obj()
{
	spTwePacket pkt_null;

	auto&& p = refTwePacket(pkt_null);
	if (p.get_type() == E_PKT::PKT_ERROR) {
		printf_("error");
	}

	spTwePacket pkt_null2;
	if (refTwePacket(pkt_null2).get_type() == E_PKT::PKT_ERROR) {
		printf_("error2");
	}
}

/**********************************************************
 * test_pkt_hist()
 *   spTwePacketのテスト (配列に保存)
 **********************************************************/

 // Store packet by IDs (1..7)
spTwePacket aPal[7];

struct _pkt_hist {
	spTwePacket _pkt[16]; // パケットの履歴
	int _i;

	_pkt_hist() : _i(-1), _pkt() {}
	void add_entry(spTwePacket p) { _pkt[++_i & 0xF] = p; }
	spTwePacket get_entry(int i) { return _pkt[(i + _i) & 0xF]; }
} pkt_hist;

// put a byte to parse
bool parse_a_byte(IParser& parser, char_t u8b, int i) {
	// parse.
	parser << u8b;

	// if complete parsing
	if (parser) {
		// EEPROM TEST
		auto&& p = parser.get_payload();

		// 1. identify the packet type
		auto pkt = identify_packet_type(parser.get_payload());

		if (pkt != E_PKT::PKT_ERROR) {
			// 2. generate the parser object.
			aPal[i] = newTwePacket(parser.get_payload().data(), (uint8_t)parser.get_payload().length(), pkt);
			pkt_hist.add_entry(aPal[i]);
			return true;
		}
	}

	return false;
}

void test_pkt_hist() {
	AsciiParser parser(512);

	uint8_t chrMsg1[] = ":800000008D0011810EE29A01808103113008020CE411300102048A00000001006163\r\n";
	uint8_t chrMsg_MOT1[] = ":80000000C30002810EE29A07808312113008020CE411300102044B150400060018FFE00410150401060010FFD80428150402060010FFD80410150403060020FFD003F8150404060000FFD00410150405060008FFC80400150406060018FFE80418150407060008FFD00410150408060018FFD80408150409060018FFC8042815040A060020FFD8042015040B060010FFD8041015040C060008FFD0040015040D060008FFD8042815040E060018FFE0041015040F060010FFD803F8A20E\r\n";
	uint8_t chrMsg_MOT2[] = ":80000000BA0030810EE29A07808312113008020CE41130010204EB15040006FF90FE4803A015040106FF88FE4003A015040206FF78FE4803B015040306FF88FE4803A815040406FF88FE3803C015040506FF80FE4003A815040606FF88FE4003A815040706FF78FE4003A015040806FF90FE3803B015040906FF90FE5003A815040A06FF78FE50039815040B06FF80FE38039015040C06FF80FE4803A815040D06FF90FE38039815040E06FF90FE4803A815040F06FF80FE4003B04079\r\n";
	//                                LqSequChild idLi80VeCt#1DsExLnData#2DsExLnDataSmSm
	//                        0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D
	uint8_t chrMsg_MOT0[] = ":80000000A8002F810EE29A07808302113008020CE41130010203CC3014\r\n";
	uint8_t chrMsg2[] = ":800000006F0010810EE29A01808103113008020CE411300102052C0000000101A59A\r\n";

	uint8_t chrAppTwe1[] = ":7881150175810000380026C9000C04220000FFFFFFFFFFA7\r\n";

	for (auto x : chrAppTwe1) parse_a_byte(parser, x, 0);
	// for (auto x : chrMsg1) parse_a_byte(parser, x, 0);
	//for (auto x : chrMsg_MOT1) parse_a_byte(parser, x, 1);
	for (auto x : chrMsg_MOT2) parse_a_byte(parser, x, 1);
	// for (auto x : chrMsg2) parse_a_byte(parser, x, 2);

	auto&& h0 = pkt_hist.get_entry(-1);
	auto&& h1 = pkt_hist.get_entry(-2);

	for (int i = 0; i < 7; i++) {
		auto&& x0 = aPal[i];

		if (x0) {
			if (x0 == E_PKT::PKT_PAL) {
				auto&& x = refTwePacketPal(x0);

				printf_("%d:PALPCB:%d LADDR:%d\n", i, x.u8palpcb, x.u8addr_src);
				if (x.u8palpcb == E_PAL_PCB::MOT) {
					auto p = x.get_PalMot();

					printf_("xyz = %d,%d,%d\n", p.i16X[0], p.i16Y[0], p.i16Z[0]);
				}
			}
			else if (x0 == E_PKT::PKT_TWELITE) {
				auto&& x = refTwePacketTwelite(x0);

				printf_("%d:TWELITE:LADDR:%d", x.u8addr_src);
			}
		}

		aPal[i] = spTwePacket();
	}
}


/// <summary>
/// test code (VisualStudio)
/// </summary>
/// <returns></returns>
int main_scratch() {
	_cputs("\033[2J\033[H"); // clear all screen and HOME
	test_payload_util();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	test_fixed_queue();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	check_necessary_memory_in_stl();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	test_C_twestgs();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	test_print_float();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	test_parser();

	//_cputs("\033[2J\033[H"); // clear all screen and HOME
	//test_parser2();

	_cputs("\033[2J\033[H"); // clear all screen and HOME
	console_test();

	return 0;
}


#endif // _MSC_VER