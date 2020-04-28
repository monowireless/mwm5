/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */
#pragma once

#ifdef _MSC_VER
#include <stdio.h>
#include <conio.h>
#include <windows.h>

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
	inline TWE::IStreamOut& operator ()(char_t c) {
		_putch(c);
		return *this;
	}
};

/// <summary>
/// conio input class
/// </summary>
class TWE_GetChar_CONIO : public TWE::IStreamIn {
public:
	inline int get_a_byte() {
		if (_kbhit()) return _getch(); else return -1;
	}
};

/// <summary>
/// windows console based terminal.
/// </summary>
class TWETerm_WinConsole : public TWETERM::ITerm {
public:
	TWETerm_WinConsole(uint8_t u8c, uint8_t u8l, TWETERM::SimpBuf_GChar* pAryLines, TWETERM::GChar* pBuff) :
		ITerm(u8c, u8l, pAryLines, pBuff) {}

	void refresh();
};

/// <summary>
/// init console
/// </summary>
void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut* winconsole, TWE::IStreamIn* winkeyb);

#endif // _MSC_VER