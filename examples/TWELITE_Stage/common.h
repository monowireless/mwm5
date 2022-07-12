#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */


#include <mwm5.h>

#include "AppIDs.h"
#include "menu_defs.h"
#include "menu.hpp"

extern const wchar_t* query_app_launch_message(int n_appsel, bool b_title = false);
extern uint32_t change_baud(uint32_t baud);

// TIMEOUT WHEN double ESC or double R-Click is performed.
#define STAGE_DOUBLE_ESC_EXIT_TIMEOUT 500
