#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twelite.hpp"

// this header sets all namespaces,
// if you don't prefer, use #include <twelite.hpp> instead.
using namespace TWE;
using namespace TWEUTILS;
using namespace TWESERCMD;
using namespace TWEFMT;
using namespace TWETERM;

#ifdef ARDUINO
using namespace TWEARD;
#endif
