/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// include all headers

#pragma once

#include "twecommon.h"
#include "tweutils.h"
#include "tweserial.h"
#include "tweprintf.h"
#include "twecrc8.h"

#include "twesercmd_gen.h"
#include "twesercmd_plus3.h"
#include "tweinputstring.h"
#include "twestring.h"

#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_cmd.h"
#include "twesettings_validator.h"

#include "twesettings_std_defsets.h"
#include "tweinteractive.h"
#include "tweinteractive_defmenus.h"
#include "tweinteractive_settings.h"
#include "tweinteractive_nvmutils.h"

#include "twesettings_callbacks.h"

#include "twesysutils.h"
#include "twenvm.h"

#ifdef _MSC_VER
#include "twesettings_weak.h"
#endif
