#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef ESP32 // for EPS32 (PRG=GPIO5, RST=GPIO26)
#include "twe_common.hpp"
#include "modctl_common.hpp"

class TweModCtlESP32 : public TweModCtlCommon {
	static const uint8_t PIN_SET = 5;
	static const uint8_t PIN_PGM = 2;
	static const uint8_t PIN_RST = 26;

public:
	TweModCtlESP32() {}

	void setup();

	void begin();

	/**
	 * @fn	bool TweModCtlESP32::reset();
	 *
	 * @brief	Resets the module
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	bool reset(bool bHold=false);



	/**
	 * @fn	bool TweModCtlESP32::setpin(bool nSet);
	 *
	 * @brief	Control set pin
	 *
	 * @param	nSet	True to set (LOW)
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	bool setpin(bool nSet);


	/**
	 * @fn	bool TweModCtlESP32::prog();
	 *
	 * @brief	Set the module program mode.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	bool prog();
};
#endif