#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(__linux)
#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

/// <summary>
/// put char to console
/// </summary>
int TWETERM_iPutC(int c);

/// <summary>
/// get char from keyboard
/// </summary>
int TWETERM_iGetC();

/// <summary>
/// conio output class
/// </summary>
class TWE_PutChar_CONIO : public TWE::IStreamOut {
public:
	TWE_PutChar_CONIO() {}
	TWE::IStreamOut& operator ()(char_t c);
};

/// <summary>
/// conio input class
/// </summary>
class TWE_GetChar_CONIO : public TWE::IStreamIn {
public:
	int get_a_byte();
};

/// <summary>
/// windows console based terminal.
/// </summary>
class TWETerm_LinuxConsole : public TWETERM::ITerm {
	static bool _b_term_controlled;
public:
	TWETerm_LinuxConsole(uint8_t u8c, uint8_t u8l);
	~TWETerm_LinuxConsole();
	void setup();
	void refresh();
	void close_term();

	static bool is_term_controlled() { return _b_term_controlled; }
};

/// <summary>
/// reinit_state console
/// </summary>
void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut* winconsole, TWE::IStreamIn* winkeyb);

#endif // _MSC_VER