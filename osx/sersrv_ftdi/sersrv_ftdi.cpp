/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <cstdio>
#include <sys/uio.h>
#include <unistd.h>

extern void exit_app(int);

#undef _DEBUG_THIS_FILE
#ifdef _DEBUG_THIS_FILE
#define DEBUG_FMT(fmt, ...) fprintf(stderr, "\033[7m" fmt "\033[0m", __VA_ARGS__)
#define DEBUG_MSG(msg) fprintf(stderr, "\033[7m" msg "\033[0m")
#else
#define DEBUG_FMT(fmt, ...) 
#define DEBUG_MSG(msg) 
#endif

//// MWM5 related code
#include "twe_common.hpp"
#include "twe_sys.hpp"

#include "twe_utils_simplebuffer.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"

#include "serial_common.hpp"
#include "serial_ftdi.hpp"
#include "modctrl_ftdi.hpp"
#include "twe_firmprog.hpp"

using namespace TWE;
using namespace TWESERCMD;
using namespace TWEUTILS;

//// serial
SerialFtdi Serial2;
TweModCtlFTDI obj_ftdi(Serial2);

//// Ascii parser
AsciiParser parse_ascii(512);
AsciiParser format_ascii(512);

bool b_esc_commands = false;
SERSRV_ESC esc_handler;

struct SERIAL_OP {
    void respond(uint8_t *b, uint8_t *e) {
        format_ascii.set_payload(b, e);

        // prepare output data message
        SmplBuf_ByteSL<1024> buffout_fmt;
        buffout_fmt << format_ascii;
        
        DEBUG_MSG("<");
        putchar(0xc2);
        putchar(0xab);
        for (auto x : buffout_fmt) {
            DEBUG_FMT("%c", x);
            putchar(x);
        }
        putchar(0xc2);
        putchar(0xbb);
        DEBUG_MSG(">");
    }

    void do_listdev(const uint8_t *b, const uint8_t *e) {
        Serial2.list_devices();
        
        DEBUG_FMT("{FTDI devices=%d}", Serial2.ser_count);
        for (int i = 0; i < Serial2.ser_count; i++) {
            DEBUG_FMT("{%d:%s}", i, Serial2.ser_devname[i]);
        }

        SmplBuf_ByteSL<512> buff;
        buff << char_t(SERSRV_CMD::LIST_DEVS);
        buff << char_t(Serial2.ser_count);
        for (int i = 0; i < Serial2.ser_count; i++) {
            uint8_t name[10];
            uint8_t desc[16];

            strncpy((char_t*)name, Serial2.ser_devname[i], sizeof(name));
            strncpy((char_t*)desc, Serial2.ser_desc[i], sizeof(desc));

            buff << name;
            buff << desc;
        }

        respond(buff.begin().raw_ptr(), buff.end().raw_ptr());
    }

    void do_open(const uint8_t *b, const uint8_t *e) {
        bool ret = false;
        Serial2.list_devices();

        if (*b == 0) { // open by index
            int idx = *(b+1);
            if (idx < Serial2.ser_count) {
                ret = Serial2.open(Serial2.ser_devname[idx]);
                DEBUG_FMT("{Open[%d] %s=%d}", idx, Serial2.ser_devname[idx], ret);
            } else {
                DEBUG_MSG("{Open IDX error}");
            }
        } else
        if (*b == 1) { // open by name
            char name[10];
            strncpy(name, (const char*)(b + 1), sizeof(name));
            if (Serial2.is_opened()) Serial2.close();
            ret = Serial2.open(name);

            DEBUG_FMT("{Open %s=%d,%x}", name, ret, Serial2.get_handle());
        }

        uint8_t resp[6], *q = resp;
        S_OCTET(q, SERSRV_CMD::OPEN);
        S_OCTET(q, ret);
        S_DWORD(q, uint32_t(Serial2.get_handle()));
        respond(resp, resp + sizeof(resp));
    }

    void do_close(const uint8_t *b, const uint8_t *e) {
        Serial2.close();

        uint8_t resp[2];
        resp[0] = SERSRV_CMD::CLOSE;
        resp[1] = true;
        respond(resp, resp + sizeof(resp));
    }

    void do_set_baud(const uint8_t *b, const uint8_t *e) {
        bool ret = false;
        
        if (b + 4 >= e) {
            uint32_t baud = G_DWORD(b);
            ret = Serial2.set_baudrate(baud);
            DEBUG_FMT("SETBAUD=%d", baud);
        }

        uint8_t resp[2];
        resp[0] = SERSRV_CMD::SET_BAUD;
        resp[1] = ret;
        respond(resp, resp + sizeof(resp));
    }

    void do_modctl_reset(const uint8_t *b, const uint8_t *e) {
        uint8_t bHold = G_OCTET(b);
        bool ret = obj_ftdi.reset(bHold);

        uint8_t resp[2];
        resp[0] = SERSRV_CMD::MODCTL_RESET;
        resp[1] = ret;
        respond(resp, resp + sizeof(resp));
    }

    void do_modctl_prog(const uint8_t *b, const uint8_t *e) {
        bool ret = obj_ftdi.prog();

        uint8_t resp[2];
        resp[0] = SERSRV_CMD::MODCTL_PROG;
        resp[1] = ret;
        respond(resp, resp + sizeof(resp));
    }

    void do_modctl_setpin(const uint8_t *b, const uint8_t *e) {
        uint8_t bSet = G_OCTET(b);
        bool ret = obj_ftdi.setpin(bSet);

        uint8_t resp[2];
        resp[0] = SERSRV_CMD::MODCTL_SETPIN;
        resp[1] = ret;
        respond(resp, resp + sizeof(resp));
    }

    void do_unknown_response() {
        uint8_t resp[2];
        resp[0] = 0xFF;
        resp[1] = 0x00;
        respond(resp, resp + sizeof(resp));
    }

    void do_command(TWEUTILS::SmplBuf_Byte& cmd) {
        const uint8_t* b = cmd.begin().raw_ptr() + 1;
        const uint8_t* e = cmd.end().raw_ptr();
        switch(cmd[0]) {
            case SERSRV_CMD::LIST_DEVS: do_listdev(b, e); break;
            case SERSRV_CMD::OPEN: do_open(b, e); break;
            case SERSRV_CMD::CLOSE: do_close(b, e); break;
            case SERSRV_CMD::SET_BAUD: do_set_baud(b, e); break;
            case SERSRV_CMD::MODCTL_RESET: do_modctl_reset(b, e); break;
            case SERSRV_CMD::MODCTL_PROG: do_modctl_prog(b, e); break;
            case SERSRV_CMD::MODCTL_SETPIN: do_modctl_setpin(b, e); break;
            default: do_unknown_response(); break;
        }
    }
} serial_op;

//// the setup()
void setup() {
}

void loop() {
    // read from char
	int nSer2 = Serial2.update();

    while (Serial2.available()) {
        int c = Serial2.read();
        if (c >= 0) {
            if (c == 0xC2) putchar(c);
            putchar(c);
        }
    }

    // read from stdin
    for(;;) {
        uint8_t b;
        size_t n = read(0, &b, 1);

        if (n == 1) {
            switch(esc_handler(b)) {
                case SERSRV_ESC::E_STATE_READ:
                    while(esc_handler.available()) {
                        uint8_t c = (uint8_t)esc_handler.read();
                        if (b_esc_commands) {
                            // handle commands
                            parse_ascii << char_t(c);
                            if (parse_ascii) {
                                // EEPROM TEST
		                        auto&& p = parse_ascii.get_payload();

                                DEBUG_MSG("{:");
                                for (auto x : p) {
                                    DEBUG_FMT("%02X", x);
                                }
                                DEBUG_MSG("}");

                                serial_op.do_command(p);
                            }
                        } else {
                            // normal -> put serial message
                            Serial2.write(&c, 1);
                            DEBUG_FMT("[%02X]", c);
                        }
                    }
                break;

                case SERSRV_ESC::E_STATE_ESC_IN:
                    b_esc_commands = true;
                    parse_ascii.reinit();
                    
                    DEBUG_MSG("<ESCIN>");
                break;

                case SERSRV_ESC::E_STATE_ESC_OUT:
                    b_esc_commands = false;
                    DEBUG_MSG("<ESCOUT>");
                break;
            }
        } else {
            break;
        }
    }
}
