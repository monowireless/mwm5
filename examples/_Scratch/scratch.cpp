#include <mwm5.h>
#include <M5Stack.h>

static TWETerm_M5_Console scr(40, 24, { 0, 0, 320, 240 }, M5);

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
}

int ct; // display counters
void loop() {
    M5.update();

    // when released the button A .. C, print a one line message.
    if (M5.BtnA.wasReleased()) {
        scr << "\033[2;1HButton A (" << ct++ << ")";
    }
    if (M5.BtnB.wasReleased()) {
        scr << "\033[3;1HButton B (" << ct++ << ")";
    }
    if (M5.BtnC.wasReleased()) {
        scr << "\033[4;1HButton C (" << ct++ << ")";
    }

    // refresh terminal
    scr.refresh();
}
