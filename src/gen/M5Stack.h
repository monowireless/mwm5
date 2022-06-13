#pragma once

#include "twe_sdl_m5.h"

// SDL2 related operation
extern void push_window_event(int32_t code, void* data1, void* data2);

#define SDL2_USERCODE_MASK 0x7F000000
#define IS_SDL2_USERCODE(c,d) (((c) & SDL2_USERCODE_MASK) == (((d) & SDL2_USERCODE_MASK)))
#define SDL2_USERCODE_CHANGE_SCREEN_SIZE   0x01000000L
#define SDL2_USERCODE_CHANGE_SCREEN_RENDER 0x02000000L

#define SDL2_USERCODE_CREATE_BYTES_3(c,b1,b2,b3) ((c) | ((b3 & 0xFF) << 16) | ((b2 & 0xFF) << 8) | ((b1 & 0xFF)))
#define SDL2_USERCODE_CREATE_BYTE(c,b1) ((c) | ((b1 & 0xFF)))
#define SDL2_USERCODE_GET_BYTE1(d) (d & 0xFF)
#define SDL2_USERCODE_GET_BYTE2(d) (((d) >> 8) & 0xFF)
#define SDL2_USERCODE_GET_BYTE3(d) (((d) >> 16) & 0xFF)

// M5_SCREEN_HIRES: internal buffer size 1:640x480 0:320x240
#include "sdl2_config.h"