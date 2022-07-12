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
    GRAPH,
    SMPL_VIEWER,
    COMMANDER,
    _APPS_END_,
    FIRM_PROG,
    INTERACTIVE,
    _UTILS_END_,
    SETTINGS,
#ifndef ESP32
    SELECT_PORT,
    MANUAL,
#endif
    _END_,
    _NEXT_APP_ = -1,
};

// if exiting with next_app=FIRM_PROG, this exit_id would show last build menu.
const int EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD = 0x1001;

// name table
const char STR_APPNAMES_STRLEN = 64;
extern const char str_appnames[int(E_APP_ID::_END_)][TWE::LANG_CT][STR_APPNAMES_STRLEN];
extern const wchar_t str_appurls[int(E_APP_ID::_END_)][TWE::LANG_CT][256]; // ref urls

// find twesetting's slot ID from E_APP_ID
int appid_to_slotid(int appid);