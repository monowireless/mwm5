/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
	/// <summary>
	/// max font register entries.
	/// </summary>
	#define FONTDEF_MAXSIZE 8

	/// <summary>
	/// font table users can register, up to FONTDEF_MAXSIZE.
	/// </summary>
	static FontDef fonttbl[FONTDEF_MAXSIZE];

	/// <summary>
	/// the singleton default instance.
	/// </summary>
	static FontDef defaultFont(0x00, true); // default font instance

	/// <summary>
	/// set default font.
	/// </summary>
	/// <param name="font"></param>
	static void s_setDefaultFont(FontDef& font) {
		font.font_code = 0;

#if defined(ARDUINO) && defined(ESP32)
		font.font_name = "Built-in 8x6 Lcd font(default)";
		font.width = 6;
		font.height = 8;
#else
		font.font_name = "(default)";
		font.width = 6;
		font.height = 8;
#endif
	}

	/// <summary>
	/// set THIS as default font.
	/// </summary>
	/// <returns></returns>
	FontDef& FontDef::set_default() {
		s_setDefaultFont(*this);
		return *this;
	}

	/// <summary>
	/// find slot for ID.
	///   - find existing slot, return existing.
	///   - neither, return defaultFont.
	/// </summary>
	/// <param name="id">id to find.</param>
	/// <returns></returns>
	const FontDef& queryFont(uint8_t id) {
		if (id == 0) {
			return defaultFont;
		}

		for (int i = 0; i < FONTDEF_MAXSIZE; i++) {
			FontDef& f = fonttbl[i];

			if (f.font_code == id) {
				return f;
			}
		}

		return defaultFont; // not found
	}

	/// <summary>
	/// query font data, used in createFontXXX().
	/// </summary>
	/// <param name="id">id(dont code) to be registered.</param>
	/// <returns>nullptr if not found, or pointer to data entry.</returns>
	FontDef* _queryFont(uint8_t id) {
		for (int i = 0; i < FONTDEF_MAXSIZE; i++) {
			FontDef* f = &fonttbl[i];

			if (f->font_code == id) {
				return f;
			}
		}

		for (int i = 0; i < FONTDEF_MAXSIZE; i++) {
			FontDef* f = &fonttbl[i];
			if (f->font_code == 0x00) { // blank slot
				return f;
			}
		}

		return nullptr;
	}

#if 0 // debug purpose
	/// <summary>
	/// debug function
	/// </summary>
	void listFont() {
		for (int i = 0; i < FONTDEF_MAXSIZE; i++) {
			FontDef& f = fonttbl[i];
			Serial.printf("\nFONT i:%d = id:%d w:%d h:%d", i, f.get_font_code(), f.width,f.height);
		}
		FontDef& f = defaultFont;
		Serial.printf("\nFONT def = id:%d w:%d h:%d", f.get_font_code(), f.width, f.height);
	}

	/// <summary>
	/// debug function
	/// </summary>
	void initFont() {
		fonttbl[2].font_code = 15;
	}
#endif


} // TWEFONT
