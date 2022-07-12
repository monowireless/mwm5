/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "App_RootMenu.hpp"
#include "App_Twelite.hpp"
#include "App_Glancer.hpp"
#include "App_Commander.hpp"
#include "App_Graph.hpp"
#include "App_FirmProg.hpp"
#include "App_Interactive.hpp"
#include "App_Console.hpp"
#include "App_Settings.hpp"
#include "App_Manual.hpp"

#ifdef ESP32
# define IDF_UART
# include <EEPROM.h>
# include <driver/dac.h>
# include <PS2Keyboard.h> // https://www.pjrc.com/teensy/td_libs_PS2Keyboard.html
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include "App_SelectPort.hpp"
#endif

#include "common.h"

static int s_change_app(TWE::APP_MGR& the_app, int n_appsel, int prev_app, int exit_id);
void update_serial_keyb_input(bool);
static void s_check_other_input();

void update_settings();

#ifndef ESP32
static void s_check_clipboard();
#endif

void setup() {
	// init lcd, serial, not init sd card
	M5.begin(true, true, true, true); // bool LCDEnable, bool SDEnable, bool SerialEnable, bool I2CEnable

	// Power Control
	M5.Power.begin(); // power control enabled

#ifdef ESP32
	// shut up the speaker!
	dac_output_disable(DAC_CHANNEL_1);
	ledcDetachPin(25);

	// set Digital pins
	pinMode(2, INPUT_PULLUP);

	// EEPROM
	EEPROM.begin(4096);

#else
	the_clip.begin(); // setup clipboard.
#endif

	// this procedure should follow after M5.begin();
#ifdef IDF_UART
	Serial2_IDF.begin(MWM5_DEFAULT_BAUD, 1024, 512);
#else
	Serial2.begin(MWM5_DEFAULT_BAUD, SERIAL_8N1, 16, 17);
	Serial2.setRxBufferSize(1536);
#endif

	// the preferences 
	
	update_settings();

	// init the keyboard
	{
		void* keymap = nullptr;
#ifdef ESP32
		if (sAppData.u8_TWESTG_STAGE_KEYBOARD_LAYOUT == 0) keymap = (void*)&PS2Keymap_US;
		if (sAppData.u8_TWESTG_STAGE_KEYBOARD_LAYOUT == 1) keymap = (void*)&PS2Keymap_JP;
#endif
		the_keyboard.setup(keymap);
	}

	// setup uart input queue (reserve 512bytes)
	the_uart_queue.setup(512);

	// init the firmware programmer.
	twe_prog.setup();

	// launch menu app
	the_app.setup(s_change_app);

#if defined(ESP32)
	the_app.new_app<App_RootMenu>();
#else
	the_app.new_app<App_SelectPort>(int(E_APP_ID::SELECT_PORT)); // firstly, load the FTDI port selection.
#endif
}

// the main loop.
void loop() {
	// update M5
	M5.update();

	// update serial queue
	update_serial_keyb_input(false);

	// update hardware button
	s_check_other_input();

	// update keyboard
	the_keyboard.update();

	// call app's loop()
	the_app.loop();

#ifndef ESP32
	// clipboard check
	s_check_clipboard();
#endif
}

/**
 * @fn	uint32_t change_baud(uint32_t baud)
 *
 * @brief	Change baud
 *          Note: must call twe_prog.set_baud_app() as well if performing twe_prog, like twe_prog.reset().
 *          TODO: should change at twe_prog object???
 *
 * @param	baud	The baud.
 *
 * @returns	An uint32_t.
 */
uint32_t change_baud(uint32_t baud) {
#ifdef IDF_UART
# warning "Serial2_IDF changing baud may not be implemented!"
	Serial2_IDF.begin(baud);
#else
# ifndef ESP32
	Serial2.set_baudrate(baud); // e.g. FTDI, /dev/ttyS0, ...
# else
# warning "Serial2 changing baud may not be implemented!"
	Serial2.begin(baud);
# endif
#endif

	return baud;
}

/**
 * @fn	static int s_change_app(TWE::APP_MGR& the_app, int n, int prev_app, int exit_id)
 *
 * @brief	Change application
 *
 * @param [in,out]	the_app 	The application management class object.
 * @param 		  	n_appsel	The requested app id.
 * @param 		  	prev_app	The previous(exiting) app id.
 * @param 		  	exit_id 	The exit code of the previous(exiting) app.
 *
 * @returns	        app id of switched app.
 */
static int s_change_app(TWE::APP_MGR& the_app, int n_appsel, int prev_app, int exit_id) {
	switch (E_APP_ID(n_appsel)) {
	case E_APP_ID::TWELITE:
		the_app.new_app<App_TweLite>();
		break;

	case E_APP_ID::SMPL_VIEWER:
		the_app.new_app<App_Glancer>();
		break;

	case E_APP_ID::COMMANDER:
		the_app.new_app<App_Commander>();
		break;

	case E_APP_ID::GRAPH:
		the_app.new_app<App_Graph>(exit_id);
		break;

	case E_APP_ID::FIRM_PROG:
		the_app.new_app<App_FirmProg>(exit_id);
		break;

	case E_APP_ID::INTERACTIVE:
		the_app.new_app<App_Interactive>();
		break;

	case E_APP_ID::CONSOLE:
		the_app.new_app<App_Console>();
		break;

	case E_APP_ID::SETTINGS:
		the_app.new_app<App_Settings>();
		break;

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
	case E_APP_ID::SELECT_PORT:
		the_app.new_app<App_SelectPort>(prev_app);
		break;

	case E_APP_ID::MANUAL:
		the_app.new_app<App_Manual>(prev_app);
		break;
#endif

	case E_APP_ID::_NEXT_APP_: // -1
	case E_APP_ID::ROOT_MENU: // should be 0
	default:
		the_app.new_app<App_RootMenu>(exit_id);

		return 0;
		break;
	}

	return n_appsel;
}

/**
 * query app title and help descriptions.
 * 
 * \param n_appsel
 * \param b_title
 * \return 
 */
const wchar_t* query_app_launch_message(int n_appsel, bool b_title) {
	#define __QUERY_A_L_M_CASE(x) return b_title ? x::APP_DESC<x>::get_TITLE_LONG() : x::APP_DESC<x>::get_LAUNCH_MSG()

	switch (E_APP_ID(n_appsel)) {
	case E_APP_ID::TWELITE: __QUERY_A_L_M_CASE(App_TweLite); break;
	case E_APP_ID::SMPL_VIEWER: __QUERY_A_L_M_CASE(App_Glancer); break;
	case E_APP_ID::COMMANDER: __QUERY_A_L_M_CASE(App_Commander); break;
	case E_APP_ID::GRAPH: __QUERY_A_L_M_CASE(App_Graph); break;
	case E_APP_ID::FIRM_PROG: __QUERY_A_L_M_CASE(App_FirmProg);	break;
	case E_APP_ID::INTERACTIVE: __QUERY_A_L_M_CASE(App_Interactive); break;
	case E_APP_ID::CONSOLE: __QUERY_A_L_M_CASE(App_Console); break;
	case E_APP_ID::SETTINGS: __QUERY_A_L_M_CASE(App_Settings);
#ifndef ESP32
	case E_APP_ID::SELECT_PORT: __QUERY_A_L_M_CASE(App_SelectPort); break;
	case E_APP_ID::MANUAL: __QUERY_A_L_M_CASE(App_Manual); break;
#endif
	default:
		return L"";
	}

	#undef __QUERY_A_L_M_CASE
}

void update_serial_keyb_input(bool force_update = false) {
	while(the_sys_keyboard.available()) {
		int c = the_sys_keyboard.get_a_byte();
		if (c >= 0) {
#ifndef ESP32
#if 1

			// sys console key input -> TWELITE via Serial2
			WrtTWE << char_t(c);
#else
			// sys console key input -> input buffer of Serial2
			the_uart_queue.push(c);
#endif
#else
			// for ESP32, system message will displayed on Serial.
			; // just discard it!
#endif
		}
	}
	
#ifdef IDF_UART
	// UART2 : connected to TWE
	while (Serial2_IDF.available()) {
		int c = Serial2_IDF.read();
		if (c >= 0) the_uart_queue.push(c);
	}
#else
# ifndef ESP32
	if (force_update) { // grab actual serial data here
		while (Serial2.update()) {
			// UART2 : connected to TWE
			while (Serial2.available()) {
				int c = Serial2.read();
				if (c >= 0) the_uart_queue.push(c);
			}
		}
	}
# endif
	// UART2 : connected to TWE
	while (Serial2.available()) {
		int c = Serial2.read();
		if (c >= 0) the_uart_queue.push(c);
	}
#endif
}

static void s_check_other_input() {
	// button operation
	if (M5.BtnA.wasReleased()) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_A);
	}
	else if (M5.BtnB.wasReleased()) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_B);
	}
	else if (M5.BtnC.wasReleased()) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_C);
	}
	else if (M5.BtnA.wasReleasefor(700)) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_A_LONG);
	}
	else if (M5.BtnB.wasReleasefor(700)) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_B_LONG);
	}
	else if (M5.BtnC.wasReleasefor(700)) {
		the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_C_LONG);
	}
}

#ifndef ESP32
static void s_check_clipboard() {
	// clip board
	TWEUTILS::Unicode_UTF8Converter uc;
	while (the_clip.paste.available()) {
		int c = the_clip.paste.read();
		wchar_t u = (c != -1) ? uc(uint8_t(c)) : 0;

		// limited to ASCII chars
		if (u <= 0xFF) {
#if 1
			the_keyboard.push(c);
#else
# if defined(_MSC_VER)
#  pragma message("CLIPBOARD PASTE TO UART QUEUE!")
# else
#  warning "CLIPBOARD PASTE TO UART QUEUE!"
# endif
			the_uart_queue.push(c); // for debug
#endif
		}
	}

	// check clip board copy request
	if (the_clip.copy.available()) {
		if (auto&& p = the_app.query_appobj()) {
			if (ITerm* ptrm = reinterpret_cast<ITerm*>(p->get_appobj())) {
				ITerm& trm = *ptrm;

				SmplBuf_ByteSL<3072> l_buff;

				trm >> l_buff;
				the_clip.copy.copy_to_clip((const char*)l_buff.c_str());
			}
		}
	}
}

#endif

/**
 * @fn	void update_settings()
 *
 * @brief	Load the saved settings and apply it.
 */
void update_settings() {
#ifndef ESP32
	the_settings_menu.begin(appid_to_slotid(int(E_APP_ID::FIRM_PROG)));
	the_cwd.set_mwsdk_env(
		sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS
		, sAppData.u8_TWESTG_STAGE_APPWRT_FORCE_DISABLE_LTO
	);

	the_settings_menu.begin(0);
	push_window_event(
		SDL2_USERCODE_CREATE_BYTE(SDL2_USERCODE_CHANGE_SCREEN_SIZE, sAppData.u8_TWESTG_STAGE_SCREEN_MODE >> 4),
		nullptr, nullptr);
	push_window_event(
		SDL2_USERCODE_CREATE_BYTE(SDL2_USERCODE_CHANGE_SCREEN_RENDER, sAppData.u8_TWESTG_STAGE_SCREEN_MODE & 0xF),
		nullptr, nullptr);
#else
	the_settings_menu.begin(0);
#endif
}