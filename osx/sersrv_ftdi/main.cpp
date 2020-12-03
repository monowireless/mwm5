/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "serial_ftdi.hpp"
#include "twe_sys.hpp"
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

extern "C" volatile uint32_t u32TickCount_ms;
volatile uint32_t u32TickCount_ms;

extern void setup();
extern void loop();

static bool _about_to_exit = false;
static int _exit_code = 0;

void exit_app(int exit_code) {
    _about_to_exit = true;
    _exit_code = exit_code;
}

int main () {
    setvbuf(stdout, NULL, _IONBF, 0); // stop buffering of STDOUT
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // no blocking for STDIN

    setup();

    for(;;) {
        loop();
        loop();
        loop();
        loop();
        if (_about_to_exit) break;
        delay(2);
    }

    return _exit_code;
}
