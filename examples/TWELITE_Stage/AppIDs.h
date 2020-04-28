#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/**
 * @enum	E_APP_ID
 *
 * @brief	Values that represent Application Identifiers
 * 			If changed, you must edit str_appnames[] in menu.cpp.
 */
enum class E_APP_ID {
    ROOT_MENU = 0,
    CONSOLE,
    TWELITE,
    PAL,
    GLANCE,
    _APPS_END_,
    FIRM_PROG,
    INTERACTIVE,
    _UTILS_END_,
    SETTINGS,
#ifndef ESP32
    SELECT_PORT,
#endif
    _END_,
};


const char STR_APPNAMES_STRLEN = 64;
extern const char str_appnames[int(E_APP_ID::_END_)][STR_APPNAMES_STRLEN];

int appid_to_slotid(int appid);