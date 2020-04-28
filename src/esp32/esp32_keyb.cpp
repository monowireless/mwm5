/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef ESP32 // for EPS32 (PRG=GPIO5, RST=GPIO26)

#undef _DEBUG_MESSAGE
#ifdef _DEBUG_MESSAGE
#define DBGOUT(...) Serial.printf(__VA_ARGS__) 
#else
#define DBGOUT(msg,...)
#endif

#include "twe_common.hpp"
#include "twe_cui_keyboard.hpp"

#include "esp32_common.h"

#include <PS2Keyboard.h> // https://www.pjrc.com/teensy/td_libs_PS2Keyboard.html
static const uint8_t KEYBOARD_INT = 35;
static const uint8_t KEYBOARD_DATA = 36;

#define I2C_KEY
#ifdef I2C_KEY
static const uint8_t  KEYBOARD_I2C_ADDR_CARDKB = 0x5F;
static const uint8_t  KEYBOARD_I2C_ADDR_FACES = 0x08;
static const uint8_t  KEYBOARD_INT_FACES = 5;
static uint8_t KEYBOARD_I2C_ADDR = 0;
#endif

namespace TWECUI {
	class KeyInput_ESP32 : public KeyInput {
	PS2Keyboard _ps2key;

	public:
		KeyInput_ESP32() : _ps2key(), KeyInput() {}

	public: // override method of KeyInput
		void setup(void* arg = nullptr) {
			// PS2 Keyboard
			pinMode(KEYBOARD_INT, INPUT);
			digitalWrite(KEYBOARD_INT, HIGH);
			// choose keymap (default is JP)
			const PS2Keymap_t* p_keym = &PS2Keymap_JP;
			if (arg != nullptr) {
				p_keym = (const PS2Keymap_t*)arg;
			}
			const PS2Keymap_t& keymap = *p_keym;

			// begins!
			_ps2key.begin(KEYBOARD_DATA, KEYBOARD_INT, keymap);

#ifdef I2C_KEY
			DBGOUT("Wire.begin()\n");
			Wire.begin(); // wire needs to begin.
			// probe the port
			
			if (!KEYBOARD_I2C_ADDR) {
				// try external keyboard
				Wire.beginTransmission(KEYBOARD_I2C_ADDR_CARDKB);
				if (Wire.endTransmission() == 0) {
					KEYBOARD_I2C_ADDR = KEYBOARD_I2C_ADDR_CARDKB;
					DBGOUT("Wire Probe = %X\n", KEYBOARD_I2C_ADDR);
				}
			}

			if (!KEYBOARD_I2C_ADDR) {
				// try external keyboard
				Wire.beginTransmission(KEYBOARD_I2C_ADDR_FACES);
				if (Wire.endTransmission() == 0) {
					KEYBOARD_I2C_ADDR = KEYBOARD_I2C_ADDR_FACES;
					pinMode(KEYBOARD_INT_FACES, INPUT_PULLUP);
					_g_twemodctl_esp32_no_set_control = true; // don't control ESP32.
					
					DBGOUT("Wire Probe = %X\n", KEYBOARD_I2C_ADDR);
				}
			}
#endif
		}

		void update() {
			while (_ps2key.available()) {
				// read the next key
				int c = _ps2key.read();


				// push read byte into an internal queue.
				if (c >= 0 && c <= 0xFF) {
					switch(c) {
						case 0x8A:
						case 0x8B:
						case 0x8C:
						case 0x8D:
							c += 0x100;
							break;
						default:
							break;
					}

					KeyInput::push(c);
				}
			}

#ifdef I2C_KEY // Genuine I2C key
			if (KEYBOARD_I2C_ADDR == KEYBOARD_I2C_ADDR_CARDKB) {
				Wire.requestFrom(KEYBOARD_I2C_ADDR_CARDKB, uint8_t(1));  // request 1 byte from keyboard

				while (Wire.available()) {
					KeyInput::keyinput_type c = Wire.read();       // receive a byte as character
					if (c != 0) {
						DBGOUT("[%02x]", c);

						switch (c) {
						case 0xb5: c = KeyInput::KEY_UP; break; // UP
						case 0xb6: c = KeyInput::KEY_DOWN; break; // DOWN
						case 0xb7: c = KeyInput::KEY_RIGHT; break; // RIGHT(FWD)
						case 0xb4: c = KeyInput::KEY_LEFT; break; // LEFT(BWD)
						}

						if (c != 0) KeyInput::push(c);
					}
				}
			} else if (KEYBOARD_I2C_ADDR == KEYBOARD_I2C_ADDR_FACES) {
				Wire.requestFrom(KEYBOARD_I2C_ADDR_FACES, uint8_t(1));  // request 1 byte from keyboard

				//if (digitalRead(KEYBOARD_INT_FACES) == LOW) {
					while (Wire.available()) {
						KeyInput::keyinput_type c = Wire.read();       // receive a byte as character
						if (c != 0) {
							DBGOUT("[%02x]", c);

							switch (c) {
							case 0xb7: case 0xa1: c = KeyInput::KEY_UP; break; // UPT
							case 0xc0: case 0xab: c = KeyInput::KEY_DOWN; break; // DOWN
							case 0xc1: case 0xac: c = KeyInput::KEY_RIGHT; break; // RIGHT(FWD)
							case 0xbf: case 0xaa: c = KeyInput::KEY_LEFT; break; // LEFT(BWD)
							case 0xaf: c = KeyInput::KEY_ESC; break; // ALT+0 (ESC)
							case 0x91: c = '1'; break; // ALT+1 -> 1
							case 0x92: c = '2'; break;
							case 0x93: c = '3'; break;
							case 0x9b: c = '4'; break;
							case 0x9c: c = '5'; break;
							case 0x9d: c = '6'; break;
							case 0xa5: c = '7'; break;
							case 0xa6: c = '8'; break;
							case 0xa7: c = '9'; break;
							}

							if (c != 0) KeyInput::push(c);
						}
					}
				//}
			}
#endif
		}
	} the_keyboard_EPS32;
}

namespace TWE {
	extern TWECUI::KeyInput& the_keyboard = TWECUI::the_keyboard_EPS32;
	TWECUI::KeyInput the_sys_keyboard(128);
}

#endif