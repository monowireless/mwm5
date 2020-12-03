/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_file.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_sys.hpp"

#include "twe_utils_simplebuffer.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_firmprog.hpp"

#include "serial_srv_pipe.hpp"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#undef _DEBUG_THIS_FILE
#ifdef _DEBUG_THIS_FILE
#define DEBUG_FMT(fmt, ...) fprintf(stderr, "\033[32m" fmt "\033[0m", __VA_ARGS__)
#define DEBUG_MSG(msg) fprintf(stderr, "\033[32m" msg "\033[0m")
#else
#define DEBUG_FMT(fmt, ...) 
#define DEBUG_MSG(msg) 
#endif

extern "C" volatile uint32_t u32TickCount_ms;
volatile uint32_t u32TickCount_ms;

extern "C" const char *twesettings_save_filepath;
const char *twesettings_save_filepath = NULL;

using namespace TWE;
using namespace TWEUTILS;
using namespace TWESERCMD;

SerialSrvPipe Serial2("../sersrv_ftdi/sersrv_ftdi.command");
TweModCtlSerialSrvPipe modctl_ser2(Serial2);
TweProg TWE::twe_prog(new TweBlProtocol<SerialSrvPipe, TweModCtlSerialSrvPipe>(Serial2, modctl_ser2));

//// Ascii parser
AsciiParser parse_ascii(512);
AsciiParser format_ascii(512);

// set console as raw mode
struct CONSOLE {
    struct termios CookedTermIos; // cooked mode
    struct termios RawTermIos; // raw mode
    CONSOLE() {
		// save the intial state of terminal
		tcgetattr(STDIN_FILENO, &CookedTermIos);

		// create RAW mode terminal
		RawTermIos = CookedTermIos;
		cfmakeraw(&RawTermIos);
		RawTermIos.c_oflag |= OPOST;

		// set stdin as RAW mode 
		tcsetattr(STDIN_FILENO, TCSANOW, &RawTermIos);
		
		// for nonblocking getchar()
		fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    }

    ~CONSOLE() {
        tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
    }
};

void show_help_message() {
    printf("\n[COMMANDS]");
    printf("\n   l       : list devices");
    printf("\n   0,1,2,3 : open device by listed number");
    printf("\n   c       : close device");
    printf("\n   r       : reset TWELITE");
    printf("\n   p       : set program mode and get module info");
    printf("\n   q       : exit program");
    printf("\nInput command key --> ");
}

void show_devices() {
    Serial2.list_devices();

    printf("\nquery serial devices...");                    
    if (Serial2.ser_count == 0) {
        printf("\n..NO SERIAL PORT FOUND...");
    } else {
        for (int i = 0; i < Serial2.ser_count; i++) {
            printf("\n%d: %s(%s)", i, Serial2.ser_desc[i], Serial2.ser_devname[i]);
        } 
    }
}

void cb_protocol(
	TweProg::E_ST_TWEBLP cmd,
	int req_or_resp,
	TWE::APIRET evarg,
	TWEUTILS::SmplBuf_Byte& payl,
	void* pobj
) {
	const char strcmd[][16] = {
		"NONE",
		"CONNECT",
		"IDENTIFY_FLASH",
		"SELECT_FLASH",
		"READ_CHIPID",
		"READ_MAC_CUSTOM",
		"FINISH",
		"FINISH_ERROR"
	};

    if (req_or_resp == TweProg::EVENT_NEW_STATE) {
        printf("\n%s>", strcmd[(int)cmd]);

        for (auto x : payl) {
            printf("%02X", x);
        }

        printf("%s", (evarg ? "ok" : "ng"));
    }
    else {
        printf("<");
        for (auto x : payl) {
            printf("%02X", x);
        }

        printf("%s", (evarg ? "ok" : "ng"));
    }
}

void setup() {
    //setvbuf(stdout, NULL, _IONBF, 0); // stop buffering of STDOUT
    //fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // no blocking for STDIN
    
    // show_help_message();
    printf("\n--- sersrv test program ---");
    show_devices();
    printf("\n\nPress Ctrl+A to Command Input.");

	twe_prog.add_cb(cb_protocol, nullptr);
}

bool about_to_close = false;
bool wait_cmd = false;
bool b_protocol = false;
void loop() {
    Serial2.update();
    while (Serial2.available()) {
        int b = Serial2.read();
        if (b == -1) break;
        
        if (!wait_cmd) DEBUG_FMT(">%02X", b);

        if (b_protocol) {
            if (twe_prog.process_input(b)) {
                b_protocol = false;

                // finished or error
                auto stat = twe_prog.get_state();

                if (stat == TweProg::E_ST_TWEBLP::FINISH) {
                    printf("\n***PROTOCOL SUCCESS!***");

                    {
                        const char names_modtype[][16] = {
                            "NONE",
                            "\033[44mBLUE\033[0m",
                            "\033[41mRED\033[0m"
                        };

                        // display info
                        printf("\nSERIAL#=\033[7m%07X\033[0m TWELITE=%s\n"
                            , twe_prog.module_info.serial_number & 0x0FFFFFFF
                            , names_modtype[(int)twe_prog.module_info.type]
                        );
                    }
                }
                else {
                    printf("\n***PROTOCOL ERROR!***");
                }
            }
        } else {
            if (!wait_cmd) putchar(b);
        }
    }
    fflush(stdout);

    while (1) {
        int b = getchar();
        if (b == -1) break;
        b = b & 0xff;

        DEBUG_FMT("<%02X", b);

        if (!wait_cmd) {
            if (b == 0x01) { // Ctrl+A
                wait_cmd = true;
                show_help_message();
            } else {
                putchar(b);
            }
        } else {
            switch (b) {
                case 0x01: 
                    putchar(b);
                break;
                
                case 'l': // list devices
                    show_devices();
                break;

                // open the device
                case '0': case '1': case '2': case '3':
                {
                    unsigned i = b - '0';
                    
                    if (i < Serial2.ser_count) {
                        unsigned i = b - '0';
                        printf("\nopen(): ");
                        int ret = Serial2.open(Serial2.ser_devname[i]);
                        printf(" %s = %d", Serial2.ser_devname[i], ret);
                    }   
                }
                break;

                case '9': // reopen
                {
                    Serial2.reopen();
                }
                break;

                case 'c':
                {
                    Serial2.close();
                    printf("\nclose()");
                }
                break;

                case 'r':
                {
                    twe_prog.reset_module();
                    printf("\nmodule_reset()");
                }
                break;

                case 'p':
                {
                    b_protocol = true;

                    // clear buffer
                    twe_prog.reset_hold_module(); // hold module (to stop serial message)
                    Serial2.update();
                    while(Serial2.available()) {
                        (void)Serial2.read();
                    }

                    twe_prog.begin(TweProg::BL_PROTOCOL_GET_MODULE_INFO);
                }
                break;
    
                case 'q':
                {
                    Serial2.close();
                    about_to_close = true;
                    return;
                }
            }
            
            wait_cmd = false;
        }
    }
}

int main() {
    CONSOLE cons;

    setup();    
    for(;;) {
        loop();
        if (about_to_close) break;
        delay(10);
    }
    return 0;
}