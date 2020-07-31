/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */
#pragma once

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

#include "version_weak.h"

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
	bool _builtin_term;
	bool _prog_mode;
public:
	TWETerm_WinConsole(uint8_t u8c, uint8_t u8l) : ITerm(u8c, u8l), _builtin_term(false), _prog_mode(true) {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode = 0;
		GetConsoleMode(hOut, &dwMode);
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);

		SetConsoleTitleA(MWM5_APP_NAME);
		
		SMALL_RECT rct;
		rct.Left = 0;
		rct.Right = u8c;
		rct.Top = 0;
		rct.Bottom = u8l;
		SetConsoleWindowInfo(hOut, TRUE, &rct);
	}

	void setup() {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode = 0;
		GetConsoleMode(hOut, &dwMode);
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	}

	void close_term();

	void refresh();

public:
	// set true to activate built-in term.
	bool set_term_mode(bool mode) {
		_builtin_term = mode;
	}

	// override () method to choose built-in term or output as-is.
	TWE::IStreamOut& operator ()(char_t c) {
		if (_builtin_term)
			return ITerm::operator()(c);
		else {
			_putch(c);
			return *this;
		}
	}
};

/// <summary>
/// reinit_state console
/// </summary>
void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut* winconsole, TWE::IStreamIn* winkeyb);

#endif // _MSC_VER