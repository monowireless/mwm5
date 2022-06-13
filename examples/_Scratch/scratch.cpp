#include <mwm5.h>
#include <M5Stack.h>


#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <SQLiteCpp/SQLiteCpp.h>

#if M5_SCREEN_HIRES == 0
static TWETerm_M5_Console scr(40, 24, { 0, 0, 320, 240 }, M5);
#else M5_SCREEN_HIRES == 1
static TWETerm_M5_Console scr(80, 32, { 0, 0, 640, 480 }, M5);
#endif

void setup() {
    // init M5
    M5.begin(true, false, true, false);

    // configure secondary serial port
    Serial2.setRxBufferSize(512); // Rx buffer
    Serial2.begin(115200, SERIAL_8N1, 16, 17); // baud 115200

    // configure terminal (font and color)
    TWEFONT::createFontShinonome16(10, 0, 0); // create with font id = 10.
    scr.set_font(10); // assign font to the terminal
    scr.set_color(ALMOST_WHITE, color565(90, 0, 50)); // set color

    scr.refresh();

    scr << "--- Hello _Scratch Sample ---";
    scr(0, 6); // move line 6
}

int ct; // display counters
void loop() {
    M5.update();

    // keyboard handling.
    do {
        int c = the_keyboard.read();
        int16_t cur_c = 0, cur_l = 0;

        // unhandled events
        switch (c) {
        case -1: break;

        case KeyInput::KEY_ENTER:
            scr.clear();
            scr(0, 6); // move home position
            break;

        case KeyInput::KEY_BUTTON_A:
            scr.get_cursor_pos(cur_c, cur_l);
            scr << "\033[2;1H\033[KButton A (" << ct++ << ")" << format(" - %d", millis());
            scr(cur_c, cur_l); // restore cursor
            break;

        case KeyInput::KEY_BUTTON_B:
            scr.get_cursor_pos(cur_c, cur_l);
            scr << "\033[3;1H\033[KButton B (" << ct++ << ")" << format(" - %d", millis());
            scr(cur_c, cur_l); // restore cursor
            break;

        case KeyInput::KEY_BUTTON_C:
            scr.get_cursor_pos(cur_c, cur_l);
            scr << "\033[4;1H\033[KButton C (" << ct++ << ")" << format(" - %d", millis());
            scr(cur_c, cur_l); // restore cursor
            break;

        default:
            if (c >= 0x20 && c < 0x7f) scr << char_t(c);
        }

    } while (the_keyboard.available());

    // refresh terminal
    scr.refresh();
}
