/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#ifdef USE_CSETTINGS

#include "twe_csettings.hpp"

#undef DEBUG_ARD
#ifdef DEBUG_ARD
#include <Arduino.h>
#endif

/// <summary>
/// init the TWE_tsFILE structure.
/// </summary>
/// <param name="fp">the TWE_tsFILE for TWE_fprintf(), etc.</param>
/// <param name="pStrm">Output stream</param>
/// <param name="pInpt">Input stream</param>
void C_TWE_printf_support::s_init(struct _tsTWETERM_FILE* fp, TWE::IStreamOut* pStrm, TWE::IStreamIn* pInpt) {
	fp->vpContext_output = (void*)pStrm;
	fp->vpContext_input = (void*)pInpt;
	fp->fp_flush = s_flush;
	fp->fp_getc = s_getc;
	fp->fp_putc = s_putc;
	fp->fp_puts = s_puts;
}

/// <summary>
/// get one byte from Strem Input.
/// </summary>
/// <param name="fp"></param>
/// <returns></returns>
int C_TWE_printf_support::s_getc(struct _tsTWETERM_FILE* fp) {
	TWE::IStreamIn* pIn = (TWE::IStreamIn *)fp->vpContext_input;
	if (pIn != nullptr) {
		int c = pIn->get_a_byte();
#ifdef DEBUG_ARD
		if (c>=0) Serial.printf("<%02x>", c);
#endif
		return c;
	}
	else {
		return - 1;
	}
}


/// <summary>
/// put a byte to Stream Output.
/// </summary>
/// <param name="c">a byte</param>
/// <param name="fp"></param>
/// <returns></returns>
int C_TWE_printf_support::s_putc(int c, struct _tsTWETERM_FILE* fp) {
	TWE::IStreamOut* pStream = (TWE::IStreamOut*)(fp->vpContext_output);
	if (pStream != nullptr) {
		*pStream << (char_t)c;

		return 1;
	}
	return 0;
}

/// <summary>
/// put string to Stream Output.
/// </summary>
/// <param name="s">string</param>
/// <param name="fp"></param>
/// <returns></returns>
int C_TWE_printf_support::s_puts(const char* s, struct _tsTWETERM_FILE* fp) {
	TWE::IStreamOut* pStream = (TWE::IStreamOut*)(fp->vpContext_output);
	if (pStream != nullptr) {
		int ct = 0;
		while (*s != 0) {
			*pStream << (char_t)*s;
			s++;
			ct++;
		}
		return ct;
	}
	else {
		return 0;
	}
}

/// <summary>
/// flush the stream.
/// </summary>
/// <param name="fp"></param>
/// <returns></returns>
void C_TWE_printf_support::s_flush(struct _tsTWETERM_FILE* fp) {
	return; // so far not implemented.
}

#endif