#include <mwm5.h>
#include <M5Stack.h>

static TWETerm_M5_Console scr(40, 24, { 0, 0, 320, 240 }, M5);

void setup() {
    // init M5
    M5.begin(true, false, true, false);

    // set serial port
    Serial2.setRxBufferSize(512);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);

    // create terminal
    TWEFONT::createFontShinonome16(10, 0, 0);
    scr.set_font(10);

    scr.set_color(ALMOST_WHITE, color565(90, 0, 50));
}

int ct;
void loop() {
    M5.update();

    if (M5.BtnA.wasReleased()) {
        scr << "Button A (" << ct++ << ")" << crlf;
    }
    if (M5.BtnB.wasReleased()) {
        scr << "Button B (" << ct++ << ")" << crlf;
    }
    if (M5.BtnC.wasReleased()) {
        scr << "Button C (" << ct++ << ")" << crlf;
    }

    // refresh terminal
    scr.refresh();
}
