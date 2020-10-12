/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_serial.hpp"
#include "esp32_uart.hpp"

#include "esp32_modctrl.hpp"

#ifdef ESP32 // for windows, others (not ESP32)
uint8_t _g_twemodctl_esp32_no_set_control = 0;

void TweModCtlESP32::setup() {
	pinMode(PIN_RST, OUTPUT);
	digitalWrite(PIN_RST, HIGH);
	
	pinMode(PIN_PGM, OUTPUT);
	digitalWrite(PIN_PGM, HIGH);
	//dacWrite(25, 0);
}

bool TweModCtlESP32::reset(bool bHold) {
	digitalWrite(PIN_RST, LOW);

	if (!bHold) {
		delay(20);

		digitalWrite(PIN_RST, HIGH);
	}

	return true;
}

bool TweModCtlESP32::setpin(bool bSet) {
	if (!_g_twemodctl_esp32_no_set_control) {
		if (bSet) {
			pinMode(PIN_SET, OUTPUT);
			digitalWrite(PIN_SET, LOW);
		}
		else {
			digitalWrite(PIN_SET, HIGH);
			pinMode(PIN_SET, INPUT_PULLUP);
		}
	}
}

bool TweModCtlESP32::prog() {
	pinMode(PIN_PGM, OUTPUT);

	digitalWrite(PIN_PGM, LOW);
	digitalWrite(PIN_RST, LOW);

	delay(20);

	digitalWrite(PIN_RST, HIGH);

	delay(20);

	digitalWrite(PIN_PGM, HIGH);

	pinMode(PIN_PGM, INPUT_PULLUP);
	// dacWrite(25, 0); // suppres speaker noise

	delay(120); // at lease 100ms wait here to wait bootloader startup.

	return true;
}

// TWE TweProg instance, MOD CONTROL and BOOTLOADER PROTOCOL object
#include "twe_firmprog.hpp"
using namespace TWE;

static TweModCtlESP32 obj_modctl;
TweProg TWE::twe_prog(new TweBlProtocol<HardwareSerial, TweModCtlESP32>(Serial2, obj_modctl));

/**
 * @fn	void _gen_modctrl_for_esp32_uart()
 *
 * @brief	install new IDF uart based BL into twe_prog.
 */
void _gen_modctrl_for_esp32_uart() {
	twe_prog._new_bl(new TweBlProtocol<SerialESP32, TweModCtlESP32>(Serial2_IDF, obj_modctl));
}
#endif