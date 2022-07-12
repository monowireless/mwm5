/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Commander.hpp"

#include <algorithm>
#ifdef min
# undef min
#endif

template<>
const wchar_t* App_Commander::APP_DESC<App_Commander>::TITLE_LONG[] =
{
	L"Commander コマンダー (命令送信)",
	L"Commander (send an order to a child device)",
};

template<>
const wchar_t* App_Commander::APP_DESC<App_Commander>::LAUNCH_MSG[] = {
	//....+....1....+....2....+....3....+....4| // 16dots 40cols
	L"Commander コマンダーは、子機側デバイスに""\r\n"
	L"対して指令を送ります。""\r\n"
	L"""\r\n"
	L"Commanderから指令を送る親機にはApp_Wings""\r\n"
	L"を書き込みます。c" "\r\n"
,
	L"Commander will send an order to child devices.\r\n"
	L"""\r\n"
	L"App_Wings is written to the parent machine that sends commands from the Commander. "
	L"Application ID and Channel are set the same for App_Wings and the child device.\r\n"
};

// color table
static uint16_t COLTBL_MAIN[8] = {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE, // color565(127, 127, 255), // BLUE,
	MAGENTA, //color565(255, 0, 142), // MAGENTA,
	CYAN,
	ALMOST_WHITE
};


void App_Commander::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(get_APP_ID()));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// save layout
	layout.the_screen = the_screen.get_draw_area();
	layout.the_screen_b = the_screen_b.get_draw_area();
	layout.b_saved = true;

	// button navigation
	set_nav_bar();

	// add tab
	_tabs.add(L"---", &App_Commander::hndr_SCR_OPEN);
	_tabs.add(L"TWELITE", &App_Commander::hndr_SCR_TWELITE80);
	_tabs.add(L"NOTICE", &App_Commander::hndr_SCR_NOTICE1);
	_tabs.update_view();

	// put a init message
	set_title_bar(PAGE_ID::PAGE_OPEN);
}

void App_Commander::loop() {
	// tab event handling
	_tabs.check_events();
	
	// peek keyevent and perform common event handling.
	// if the event is not for this controls, APP_HNDLR will take care of.
	bool b_loop = true;
	do {
		int c = the_keyboard.peek_a_byte();
		
		if (c == KeyInput::KEY_ESC || KeyInput::is_mouse_right_up(c)) {
			// handle double ESC/double right click
			static uint32_t t_last;
			uint32_t t_now = millis();

			if (t_now - t_last < STAGE_DOUBLE_ESC_EXIT_TIMEOUT) {
				the_app.exit(get_APP_ID());
			}
			else {
				b_loop = false;
			}
			t_last = t_now; 
		}
		else if (c == KeyInput::KEY_BUTTON_A_LONG) {
			the_app.exit(get_APP_ID());
		}
		else if (c == KeyInput::KEY_BUTTON_C_LONG) {
			twe_prog.reset_module();
		}
		else if (c == 't') {
			_tabs.select(PAGE_ID::PAGE_TWELITE80);
		}
		else if (c == 'n') {
			_tabs.select(PAGE_ID::PAGE_NOTICE01);
		}
		else {
			b_loop = false; // message is not used.
		}
			
		if (b_loop) { // still looping
			the_keyboard.get_a_byte(); // skip this byte
			c = the_keyboard.peek_a_byte(); // next byte
		}
	} while (b_loop);

	// loop by APP_HNDLR
	APP_HNDLR::loop();

	// LCD update
	screen_refresh();
}

void App_Commander::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

#if M5_SCREEN_HIRES == 0
	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font
	TWEFONT::createFontMP12(10, 0, 0); // MP11 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font

	the_screen_t.set_font(11);
	the_screen_tab.set_font(1);
	the_screen.set_font(10);
	the_screen_b.set_font(1);
	the_screen_c.set_font(1);

#elif M5_SCREEN_HIRES == 1
	font_IDs.main = 11;
	font_IDs.smaller = 14;
	font_IDs.medium = 15;
	font_IDs.tiny = 1;

	TWEFONT::createFontShinonome16_full(font_IDs.main, 4, 2); // MP11 font

	TWEFONT::createFontMP10_std(font_IDs.tiny, 0, 0);
	TWEFONT::createFontMP12_std(font_IDs.smaller, 0, 0);
	TWEFONT::createFontShinonome14(font_IDs.medium, 1, 0);

	TWEFONT::createFontShinonome16(13, 0, 0, /* TWEFONT::U32_OPT_FONT_YOKOBAI */ 0); // TOP
	TWEFONT::createFontShinonome16(10, 0, 0); // TAB
	TWEFONT::createFontMP10_std(12, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI); // NAVBAR
	
	the_screen.set_font(font_IDs.main);
	the_screen_b.set_font(font_IDs.smaller);

	the_screen_t.set_font(13);
	the_screen_tab.set_font(10);
	the_screen_c.set_font(12);
#endif
	
	// main screen area
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.force_refresh();

	// tab area
	the_screen_tab.set_color(default_bg_color, color565(0x80, 0x80, 0x80));
	the_screen_tab.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen_tab.set_color_table(COLTBL_MAIN);
	the_screen_tab.force_refresh();

	// bottom area (sub screen)
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.force_refresh();

	// top area
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();

	// nav bar area
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();
}

void App_Commander::screen_layout_opening() {
	if (layout.b_saved) {
		// full screen clear
		the_screen.set_draw_area(layout.the_screen);
		the_screen_b.set_draw_area(layout.the_screen_b);
		the_screen_b.set_font(font_IDs.medium);
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		the_screen.force_refresh();
		the_screen_b.force_refresh();
	}
}

void App_Commander::screen_layout_apps() {
	Rect r = layout.the_screen;
	Rect r_b = layout.the_screen_b;

	// move boundary by 100pix
	r.h += 80;
	r_b.h -= 80;
	r_b.y += 80;

	the_screen_b.set_draw_area(r_b);
	the_screen_b.set_font(font_IDs.smaller);
	the_screen_b.clear_screen();
	the_screen_b.force_refresh();

	the_screen.set_draw_area(r);
	the_screen.clear_screen();
	the_screen.force_refresh();
}

// screen refresh timing (every 32ms)
void App_Commander::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_tab.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}

// button navigation
void App_Commander::set_nav_bar() {
	the_screen_c.clear_screen();

	//e_screen_c <<        "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << MLSLW(L"     --/長押:戻る            --/--                --/ﾘｾｯﾄ",
						  L"     --/Long:Back            --/--                --/RST");
}


// set title bar
void App_Commander::set_title_bar(int page_id) {
	const wchar_t* title = MLSLW(L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｺﾏﾝﾀﾞｰ\033[0m",
								 L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Commander\033[0m");

	the_screen_t.clear_screen();

	switch (page_id) {
	case PAGE_ID::PAGE_OPEN:
		the_screen_t << title << ":---"; break;
	case PAGE_ID::PAGE_TWELITE80:
		the_screen_t << title << ":TWELITE"; break;
	case PAGE_ID::PAGE_NOTICE01:
		the_screen_t << title << ":NOTICE"; break;
	default:
		the_screen_t << title; break;
	}
}
