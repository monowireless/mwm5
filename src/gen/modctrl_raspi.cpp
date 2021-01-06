/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(MWM5_BUILD_RASPI)
#include "modctrl_raspi.hpp"

#include "twe_sys.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <filesystem>

static bool export_ports(int port) {
    char port_txt[16];
    char port_dev_txt[256];
    snprintf(port_txt, sizeof(port_txt), "%d", port);
    snprintf(port_dev_txt, sizeof(port_dev_txt), "/sys/class/gpio/gpio%s", port_txt);

    bool ret = true;
    
    if (!std::filesystem::exists(port_dev_txt)) {
        int fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd == -1) ret = false;
        if (ret && (write(fd, port_txt, 2) != 2)) ret = false;
        if (fd != -1) {
            fsync(fd);
            close(fd);
        }
    }

    return ret;
}

static bool set_port_dir(int port, int dir_is_output) {
    char port_dev_txt[256];
    snprintf(port_dev_txt, sizeof(port_dev_txt), "/sys/class/gpio/gpio%d/direction", port);

    export_ports(port);

    int ret = true;

    if (std::filesystem::exists(port_dev_txt)) {
        int fd = open(port_dev_txt, O_WRONLY);
        if (fd == -1) ret = false;
        const char *dir_txt = dir_is_output ? "out" : "in";
        int dir_txt_len = dir_is_output ? 3 : 2;
        if (ret && (write(fd, dir_txt, dir_txt_len) != dir_txt_len)) ret = false;
        if (fd != -1) {
            fsync(fd);
            close(fd);
        }
    }

    return ret;
}

static bool set_port_out(int port, int hi_lo) { // 1 as high, 0 as lo
    char port_dev_txt[256];
    snprintf(port_dev_txt, sizeof(port_dev_txt), "/sys/class/gpio/gpio%d/value", port);

    export_ports(port);

    int ret = true;

    if (std::filesystem::exists(port_dev_txt)) {
        int fd = open(port_dev_txt, O_WRONLY);
        if (fd == -1) ret = false;
        const char *dir_txt = hi_lo ? "1" : "0";
        if (ret && (write(fd, dir_txt, 1) != 1)) ret = false;
        if (fd != -1) {
            fsync(fd);
            close(fd);
        }
    }

    return ret;
}

void TweModCtlRaspi::setup() {
    // RESET (output, high)
    export_ports(_port_rst);
    delay(100); // not sure, this delay is mandate or not...
    set_port_dir(_port_rst, 1); // output
    delay(100);
    set_port_out(_port_rst, 1); // set high
    delay(100);

    // PGM (output, high)
    export_ports(_port_pgm);
    delay(100);
    set_port_dir(_port_pgm, 1); // output
    delay(100);
    set_port_out(_port_pgm, 1); // set high
    delay(100);

    // SET PIN (set output when needed)
    export_ports(_port_set);
    delay(100);
    set_port_dir(_port_set, 0); // input
    delay(100);
}

void TweModCtlRaspi::begin() {
}

bool TweModCtlRaspi::reset(bool bHold) {
    set_port_out(_port_rst, 0); // RST=LO

    if (!bHold) {
        delay(20);
        set_port_out(_port_rst, 1); // RST=HI
    }

    return true;
}

bool TweModCtlRaspi::setpin(bool bSet) {
    if (bSet) set_port_dir(_port_set, 1); // SET=OUT
    set_port_out(_port_set, bSet ? 0 : 1); // SET=HIGH,LO
    if (!bSet) set_port_dir(_port_set, 0); // SET=INPUT
    
	return true;
}

bool TweModCtlRaspi::prog() {
    set_port_out(_port_pgm, 0); // PGM=LO
    set_port_out(_port_rst, 0); // RST=LO
    delay(50);
    set_port_out(_port_rst, 1); // RST=HIGH
    delay(200);
    set_port_out(_port_pgm, 1); // PGM=HIGH
    
    return true;
}

#endif