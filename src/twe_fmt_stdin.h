#pragma once

#if defined(TWE_STDINOUT_ONLY)
# ifdef TWE_HAS_MILLIS
extern uint32_t millis();
# else
#define millis() (0)
# endif
#else
#include "twe_sys.hpp"
#endif