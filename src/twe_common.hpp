#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

// for edition Arduino codes on VS (intelisense)
#if defined(CODE_ARDUINO) && defined(_MSC_VER)
#pragma message ("CODE_ARDUINO defined, just for coding... remove when building.")
#define ARDUINO
#endif

// check if it's under 64bit system or 32bit
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) &&     !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define IS64BIT 1
#else
#define IS32BIT 1
#endif

// define firstly prior to other headers inclusion.
#if __GNUC__
#define _GLIBCXX_DEQUE_BUF_SIZE 32	// smaller chunk for `deque' in GCC, 
									// the default 512bytes are too big for embedded systems.
#endif

// identify the architecture
#ifdef _MSC_VER
// Visual Studion
#elif defined(ARDUINO)
// ARDUINO
//#include <Arduino.h>
//#include <M5stack.h>
#endif

// type definition
#include <stdint.h> // for type name
typedef char char_t;

#ifndef NULL
#define NULL nullptr
#endif

// use RTTI or not NOTE: DEFAULT compiler option is -no-rtti for ESP32.
#if defined(_MSC_VER) && !defined(ARDUINO)
// #define TWE_USE_RTTI // not use RTTI so far
#elif defined(ARDUINO) && defined(ESP32)
#undef TWE_USE_RTTI // you can enable it (but you need to edit platform.txt)
#endif
