/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// #if !defined(MWM5_SERIAL_NO_FTDI) && (defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__))
#if defined(__APPLE__) //&& defined(MWM5_SERIAL_NO_FTDI) 

#include "serial_srv_pipe.hpp"
#include "twe_utils_simplebuffer.hpp"

using namespace TWE;
using namespace TWEUTILS;

int SerialSrvPipe::_update() {
    int ct = 0;
    while (_pipe.available()) {
        if (!_que.is_full()) {
            int c = _pipe.getchar();
            if (c == -1) break;
            c &= 0xff;

            switch(esc_handler(c)) {
                case SERSRV_ESC::E_STATE_READ:
                {
                    while(esc_handler.available()) {
                        uint8_t c = (uint8_t)esc_handler.read();
                        _buf[ct] = c;
                        _que.push(c);
                        ct++;
                    }
                }
                break;
            }
            
            if (ct >= (sizeof(_buf) - 4)) break; // stop here
        } else break;
    }

    _buf_len = ct;
    return ct;
}

bool SerialSrvPipe::_wait_respond() {
    uint32_t ts = millis();
    parse_ascii.reinit();
    
    for(;;) {
        if (_pipe.available()) {
            int c = _pipe.getchar();
            if (c == -1) continue;
            c &= 0xff;

            switch(esc_handler(c)) {
            case SERSRV_ESC::E_STATE_READ:
                while(esc_handler.available()) {
                    uint8_t c = (uint8_t)esc_handler.read();
                    if (b_esc_commands) {
                        // handle commands
                        if (!parse_ascii.is_complete()) {
                            parse_ascii << char_t(c);
                        }
                    } else {
                        // normal -> put serial message
                        _que.push(c);
                    }
                }
            break;

            case SERSRV_ESC::E_STATE_ESC_IN:
                b_esc_commands = true;
                parse_ascii.reinit();
            break;

            case SERSRV_ESC::E_STATE_ESC_OUT:
                if (b_esc_commands) {
                    return parse_ascii.is_complete();
                }
                b_esc_commands = false;
            break;
            }
        }
        if (millis() - ts > 300) return false; // timeout   
    }
}

void SerialSrvPipe::_send_cmd(uint8_t *b, uint8_t *e) {
    SmplBuf_ByteSL<256> buff;
    
    // ESC IN
    //_pipe.putchar(0x20);
    _pipe.putchar(0xC2);
    _pipe.putchar(0xAB);

    // send by ASCII
    format_ascii.set_payload(b, e);
    buff << format_ascii;
    for(auto x : buff) _pipe.putchar(x);

    // fprintf(stderr, "\033[31m%s\033[0m", buff.c_str());
        
    // ESC OUT
    _pipe.putchar(0xC2);
    _pipe.putchar(0xBB);
}

int SerialSrvPipe::_write(const uint8_t* p, int len) {
    int ct = 0;
    if (len == 0 || p == nullptr) return 0;

#if 0
    if (p) {
        while(len > 0) {
            int c = *p++;
            ct++;

            if (c == 0xC2) {
                _pipe.putchar(c);
            }
            _pipe.putchar(c);

            --len;
        }   
    }
#else
    // write by 256bytes chunk
    uint8_t buf[512]; // more buffer then 256 bytes (because 0xC2 is doubled)
    for (int k = 0; k < (len - 1) / 256 + 1; k++) {
        int len2 = 0;
        if (p == nullptr) return 0;

        for (int i = k*256; i < (k+1)*256 && i < len; i++) {
            int c = p[i];
            buf[len2++] = c;
            if (c == 0xC2) buf[len2++] = c; // C2 -> C2 C2 (for ESC SEQ)
        }
        ct += (int)_pipe.write(buf, len2);
    }
#endif
    return ct;
}

bool SerialSrvPipe::_open(const char* devname) {
    uint8_t name[10];
    SmplBuf_ByteSL<128> buff;

    strncpy((char*)name, devname, sizeof(name));
    buff << uint8_t(SERSRV_CMD::OPEN);
    buff << uint8_t(1); // by name
    buff << name; // add name
    _send_cmd(buff.begin().raw_ptr(), buff.end().raw_ptr());

    delay(10);

    bool res = false;
    uint32_t session_id;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        const uint8_t *p = payl.cbegin().raw_ptr();
        if (payl[0] == SERSRV_CMD::OPEN) {
            res = G_OCTET(p);
            session_id = G_DWORD(p);
        }
    }

    if (res) {
        strncpy((char*)_devname, devname, sizeof(_devname));
        _session_id = int(session_id);
    } else {
        _devname[0] = 0;
        _session_id = -1;
    }
    
    return res;
}

void SerialSrvPipe::_close() {
    uint8_t buff[] = { uint8_t(SERSRV_CMD::CLOSE) };
    _send_cmd(buff, buff + sizeof(buff));

    _devname[0] = 0;

    // handle response
    bool res = false;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::CLOSE) {
            res = payl[1];
        }
    }
    //return res;
}

int SerialSrvPipe::_list_devices(bool append_entry) {
    int nStored = 0;

    uint8_t buff[] = { uint8_t(SERSRV_CMD::LIST_DEVS) };
    _send_cmd(buff, buff + sizeof(buff));

    delay(10);

    if (_wait_respond()) {
        auto&& payl= parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::LIST_DEVS) {
            uint8_t count = payl[1];
            if (!append_entry) ser_count = 0; 
            for(int i = 0; i < count; i++) {
                strncpy(ser_devname[ser_count + nStored]
                        , (const char*)payl.begin().raw_ptr() + 2 + i * 26, 10);
                strncpy(ser_desc[ser_count + nStored]
                        , (const char*)payl.begin().raw_ptr() + 2 + i * 26 + 10, 16);
                nStored++;
            }
            ser_count += nStored;
        }
    } 

    return nStored;
}

bool SerialSrvPipe::_set_baudrate(int baud) {
    uint8_t buff[5], *q = buff;
    S_OCTET(q, SERSRV_CMD::SET_BAUD);
    S_DWORD(q, uint32_t(baud));

    _send_cmd(buff, buff + sizeof(buff));

    // handle response
    bool res = false;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::SET_BAUD) {
            res = payl[1];
        }
    }
    return res;
}

bool SerialSrvPipe::modctl_reset(bool bHold) {
    uint8_t buff[2], *q = buff;
    S_OCTET(q, SERSRV_CMD::MODCTL_RESET);
    S_OCTET(q, bHold);
    
    _send_cmd(buff, buff + sizeof(buff));

    // handle response
    bool res = false;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::MODCTL_RESET) {
            res = payl[1];
        }
    }
    return res;
}

bool SerialSrvPipe::modctl_setpin(bool bSetPin) {
    uint8_t buff[2], *q = buff;
    S_OCTET(q, SERSRV_CMD::MODCTL_SETPIN);
    S_OCTET(q, bSetPin);
    
    _send_cmd(buff, buff + sizeof(buff));

    // handle response
    bool res = false;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::MODCTL_SETPIN) {
            res = payl[1];
        }
    }
    return res;
}

bool SerialSrvPipe::modctl_prog() {
    uint8_t buff[2], *q = buff;
    S_OCTET(q, SERSRV_CMD::MODCTL_PROG);
    S_OCTET(q, 0);
    
    _send_cmd(buff, buff + sizeof(buff));

    // handle response
    bool res = false;
    if(_wait_respond()) {
        auto&& payl = parse_ascii.get_payload();
        if (payl[0] == SERSRV_CMD::MODCTL_PROG) {
            res = payl[1];
        }
    }
    return res;
}
#endif // APPLE & NO FTDI