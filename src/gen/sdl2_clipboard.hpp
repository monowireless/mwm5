#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_utils_fixedque.hpp"
#include "twe_stream.hpp"

namespace TWE {
    class twe_clipboard {
    public:
        class _copy {
            bool _breq;    
        public:
            void copy_to_clip(const char *str);
            bool available() { return _breq; }
            void request_to_copy() { _breq = true;  }
        } copy;

        class _paste {
            bool _breq;
            TWEUTILS::InputQueue<uint8_t> _que;
        public:
            _paste() : _que(2048), _breq(false) {}
            virtual void past_from_clip();
            bool available() { return _que.available(); }
            int read() { return _que.pop_front(); }
        } paste;

        void begin() {}

        twe_clipboard() : copy(), paste() {}
    };

    extern twe_clipboard the_clip;
}

#endif