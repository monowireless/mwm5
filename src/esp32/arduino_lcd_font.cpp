/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_utils.hpp"

#if defined(ARDUINO) && defined(ESP32)
#include <M5Stack.h>
#include "twe_font.hpp"

#undef DEBUGSER

namespace TWEFONT {
	/// <summary>
	/// draw a char in LCD (M5stack)
	/// 
	/// Note: setWindow() does not allow bigger area (maybe > 256bytes).
	///       it should be set row by row when drawing a bigger char.
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="c">drawing character (UNICODE)</param>
	/// <param name="color"></param>
	/// <param name="bg"></param>
	/// <param name="opt">
	///   MASK 0x01 : render bold style.
	/// </param>
	/// <returns></returns>
	int16_t drawChar(const FontDef& font, int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t opt) {
		// check if default font. 
		if (font.is_default()) {
			// (maybe this code is unexpected)

			if (TWEUTILS::Unicode_isSingleWidth(c) && c < 0x80) {
				M5.Lcd.drawChar(x, y, c, color, bg, 1);
			}
			else {
				M5.Lcd.drawChar(x, y, '#', bg, color, 1); // write something
			}

			return font.get_width();
		}

		// check bold option
		bool bold = ((opt & 0x01) == 0x01); // if true, draw with bold style
		bool cursor = ((opt & 0x02) == 0x02); // draw cursor
		int cursor_rows = font.height / 2 - 1; // cursor start row

#ifdef DEBUGSER
		Serial.printf("(%d:%04x", font.get_font_code(), c);
#endif

		if (TWEUTILS::Unicode_isSingleWidth(c)) {
			// render single width chars

			// find font data (only assuming 8bit width)
			const uint8_t* p = &font.font_latin1[0];
			if (c < 0x80) p = &font.font_latin1[font.data_rows * c];
			else if (c < 0xa0) p = p; // out of range
			else if (c < 0x100) p = font.font_latin1_ex ? &font.font_latin1_ex[font.data_rows * (c - 0xA0)] : p; // latin1 ex
			else if (c < 0xFF60) p = p; // out of range
			else if (c < 0xFFA0) p = font.font_jisx201 ? &font.font_jisx201[font.data_rows * (c - 0xFF60)] : p; // hankaku kana
			else p = p; // out of range


			if (!(font.opt & U32_OPT_FONT_TATEBAI || font.opt & U32_OPT_FONT_YOKOBAI)) {
				// do render
				M5.Lcd.startWrite();

				// DO NOT USE setWindow() for whole character, but set to one row.
				// M5.Lcd.setWindow(x, y, x + font.width + font.w_space - 1, y + font.height + font.h_space); // y is actual font height, x is font width -1

				int i;
				int w = font.width + font.w_space;
				uint16_t col;
				for (i = 0; i < font.data_rows && i < font.height; i++) {
					M5.Lcd.setWindow(x, y + i, x + w - 1, y + i + 1); // y is actual font height, x is font width -1

					uint8_t pat = p[i];
					if (bold) pat = pat | (pat >> 1);

					int j;
					for (j = 0; j < font.width; j++, pat <<= 1) {
						uint8_t bit = (pat & 0x80);
						
						if (cursor && i > cursor_rows) {
							col = bit ? BLACK : RED;
						} 
						else {
							col = bit ? color : bg;
						}
						tft_Write_16(col);
					}

					if (cursor && i > cursor_rows) { col = RED;	}
					else {	col = bg; }
					for (; j < font.width + font.w_space; j++) {
						tft_Write_16(col);
					}
				}

				// blank line (bottom), if drawing additional line gap.
				col = cursor ? RED : bg;
				for (; i < font.height + font.h_space; i++) {
					M5.Lcd.setWindow(x, y + i, x + w - 1, y + i + 1); // y is actual font height, x is font width -1
					for (int j = 0; j < font.width + font.w_space; j++) {
						tft_Write_16(col);
					}
				}

				M5.Lcd.endWrite();
			}
			else {
				// Double size.
				bool yokobai = (font.opt & U32_OPT_FONT_YOKOBAI);
				bool tatebai = (font.opt & U32_OPT_FONT_TATEBAI);

				int w = 0;
				if (yokobai) w = (font.width + font.w_space) * 2;
				else w = (font.width + font.w_space);

				int h = 0;
				if (tatebai) h = font.height * 2;
				else h = font.height;

				// do render
				M5.Lcd.startWrite();
				int i;
				int nLine = 0;
				uint16_t col1, col2;
				for (i = 0; i < font.height; i++) {
					int tate_ct = tatebai ? 2 : 1;
					while (tate_ct--) {
						M5.Lcd.setWindow(x, y + nLine, x + w - 1, y + nLine + 1); // y is actual font height, x is font width -1

						uint8_t pat = (i < font.data_rows) ? p[i] : 0;
						uint16_t last_bit = 0;

						int j;
						for (j = 0; j < font.width; j++, pat <<= 1) {
							uint8_t bit = (pat & 0x80);

							if (cursor && i > cursor_rows) {
								col2 = bit ? BLACK : RED;
								col1 = (bold && last_bit ? BLACK : col2);
							}
							else {
								col2 = bit ? color : bg;
								col1 = (bold && last_bit ? color : col2);
							}

							tft_Write_16(col1);
							if (yokobai) {
								tft_Write_16(col2);
							}
							last_bit = bit;
						}

						if (cursor && i > cursor_rows) {
							col1 = cursor ? RED : bg;
							col2 = col1;
						}
						else {
							col1 = bg;
							col2 = col1;
						}
						for (; j < (font.width + font.w_space); j++) {
							tft_Write_16(col1);
							if (yokobai) {
								tft_Write_16(col2);
							}
						}
						nLine++;
					}
				}

				// blank line (bottom)
				col1 = cursor ? RED : bg;
				col2 = col1;
				for (; i < font.height + font.h_space; i++) {
					M5.Lcd.setWindow(x, y + nLine, x + w - 1, y + nLine + 1); // y is actual font height, x is font width -1
					for (int j = 0; j < font.width + font.w_space; j++) {
						tft_Write_16(col1);
						if (yokobai) {
							tft_Write_16(col2);
						}
					}
					nLine++;
				}
				M5.Lcd.endWrite();
			}
			
#ifdef DEBUGSER
			Serial.printf(")", c);
#endif

			return font.get_width();
		}
		else {
			// check wide char font data presence.
			if (font.font_wide == nullptr) return 0; // nurupo check

			// find font data (only assuming 16bit width)
			const uint8_t* p = font.font_wide_missing;
			int idx = font.find_font_index(c); // find index to unicode bitmap data by charcode.
			if (idx >= 0) p = &font.font_wide[idx * font.data_rows * 2]; // find from the table

#ifdef DEBUGSER
			Serial.printf("->%d)", idx);
#endif
			if (!(font.opt & U32_OPT_FONT_TATEBAI || font.opt & U32_OPT_FONT_YOKOBAI)) {
				// do render
				M5.Lcd.startWrite();
				M5.Lcd.setWindow(x, y, x + ((font.width + font.w_space) * 2 - 1), y + font.height + font.h_space); // y is actual font height, x is font width -1
				int i;
				uint16_t col;
				for (i = 0;  i < font.height; i++) {
					uint16_t pat = i < font.data_rows ? ((p[i*2] << 8) | p[i*2+1]) : 0; // if you need >16dot font, you need edit here to expand bit width.
					if (bold) pat = pat | (pat >> 1);
					int j;
					for (j = 0; j < font.width * 2; j++, pat <<= 1) {
						uint16_t bit = (pat & 0x8000);

						if (cursor && i > cursor_rows) {
							col = bit ? BLACK : RED;
						}
						else {
							col = bit ? color : bg;
						}
						tft_Write_16(col);
					}

					if (cursor && i > cursor_rows) {	col = RED; }
					else { col = bg; }
					for (; j < (font.width + font.w_space) * 2; j++) {
						tft_Write_16(col);
					}
				}
				// blank line (bottom)
				col = cursor ? RED : bg;
				for (; i < font.height + font.h_space; i++) {
					for (int j = 0; j < (font.width + font.w_space) * 2; j++) {
						tft_Write_16(col);
					}
				}
				M5.Lcd.endWrite();
			}
			else {
				bool yokobai = (font.opt & U32_OPT_FONT_YOKOBAI);
				bool tatebai = (font.opt & U32_OPT_FONT_TATEBAI);
				
				int w = 0;
				if (yokobai) w = (font.width + font.w_space) * 4;
				else w = (font.width + font.w_space) * 2;

				int h = 0;
				if (tatebai) h = font.height * 2;
				else h = font.height;

				// do render
				M5.Lcd.startWrite();
				int i;
				int nLine = 0;

				uint16_t col1, col2;
				for (i = 0; i < font.height; i++) {
					int tate_ct = tatebai ? 2 : 1;
					while (tate_ct--) {
						M5.Lcd.setWindow(x, y + nLine, x + w - 1, y + nLine + 1); // y is actual font height, x is font width -1

						uint16_t pat = (i < font.data_rows) ? ((p[i * 2] << 8) | p[i * 2 + 1]) : 0;
						uint16_t last_bit = 0;

						int j;
						for (j = 0; j < font.width * 2; j++, pat <<= 1) {
							uint16_t bit = (pat & 0x8000);

							if (cursor && i > cursor_rows) {
								col2 = bit ? BLACK : RED;
								col1 = (bold && last_bit ? BLACK : col2);
							}
							else {
								col2 = bit ? color : bg;
								col1 = (bold && last_bit ? color : col2);
							}

							tft_Write_16(col1);
							if (yokobai) {
								tft_Write_16(col2);
							}
							last_bit = bit;
						}

						if (cursor && i > cursor_rows) {
							col1 = cursor ? RED : bg;
							col2 = col1;
						}
						else {
							col1 = bg;
							col2 = col1;
						}
						for (; j < (font.width + font.w_space) * 2; j++) {
							tft_Write_16(col1);
							if (yokobai) {
								tft_Write_16(col2);
							}
						}
						nLine++;
					}
				}
				// blank line (bottom)
				for (; i < font.height + font.h_space; i++) {
					M5.Lcd.setWindow(x, y + nLine, x + w - 1, y + nLine + 1); // y is actual font height, x is font width -1
					col1 = cursor ? RED : bg;
					col2 = col1;
					for (int j = 0; j < (font.width + font.w_space) * 2; j++) {
						tft_Write_16(col1);
						if (yokobai) {
							tft_Write_16(col2);
						}
					}
					nLine++;
				}
				M5.Lcd.endWrite();
			}

			return font.get_width() * 2;
		}
	}


	/// <summary>
	/// draw multi chars
	/// </summary>
	/// <param name="font"></param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="str"></param>
	/// <param name="color"></param>
	/// <param name="bg"></param>
	/// <param name="opt"></param>
	/// <returns>width of drawing string.</returns>
	int16_t drawChar(const FontDef& font, int32_t x, int32_t y, uint16_t* str, uint32_t color, uint32_t bg, uint8_t opt) {
		TWEUTILS::Unicode_UTF8Converter utf8conv;
		int32_t x_now = x;

		// utf-8 converter
		for (uint16_t* p = str; *p != 0; p++) {
			uint8_t font_w = font.get_width(*p);

			drawChar(font, x_now, y, *p, color, bg, opt);
			x_now += font_w;
		}

		return (int16_t)(x_now - x);
	}

	/// <summary>
	/// draw multi chars
	/// </summary>
	/// <param name="font"></param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="str"></param>
	/// <param name="color"></param>
	/// <param name="bg"></param>
	/// <param name="opt"></param>
	/// <returns>witdh of drawing string.</returns>
	int16_t drawChar(const FontDef& font, int32_t x, int32_t y, const char* str, uint32_t color, uint32_t bg, uint8_t opt) {
		TWEUTILS::Unicode_UTF8Converter utf8conv;
		int32_t x_now = x;

		// utf-8 converter
		for (uint8_t* p = (uint8_t*)str; *p != 0; p++) {
			uint16_t wc = utf8conv(*p);
			if (wc) {
				uint8_t font_w = font.get_width(wc);

				drawChar(font, x_now, y, wc, color, bg, opt);
				x_now += font_w;
			}
		}

		return (int16_t)(x_now - x);
	}

} // TWEFONT


#endif // ARDUINO