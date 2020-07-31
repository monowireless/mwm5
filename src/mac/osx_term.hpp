#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(__APPLE__)
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
class TWETerm_MacConsole : public TWETERM::ITerm {
	bool _builtin_term;

public:
	TWETerm_MacConsole(uint8_t u8c, uint8_t u8l);
	~TWETerm_MacConsole();
	void setup();
	void refresh();
	void close_term();

	// set true to activate built-in term.
	bool set_term_mode(bool mode) {
		_builtin_term = mode;
		return mode;
	}

	// override () method to choose built-in term or output as-is.
	TWE::IStreamOut& operator ()(char_t c) {
		if (_builtin_term)
			return ITerm::operator()(c);
		else {
			putchar(c);
			return *this;
		}
	}
};

/// <summary>
/// reinit_state console
/// </summary>
void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut* winconsole, TWE::IStreamIn* winkeyb);

#endif // _MSC_VER