#pragma once

/* Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "sdl2_config.h"

#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
#include <SDL.h>
#include <SDL_thread.h>
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 0
#include <thread>
#include <mutex>
#endif

namespace TWE {
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1

	// critical section using SDL2 API
	class LockGuard {
	public:
		using mutex_type = SDL_mutex*;

		explicit LockGuard(mutex_type& m) : _mtx(m) {
			Lock(_mtx);
		}

		~LockGuard() noexcept {
			Unlock(_mtx);
		}

		explicit operator bool() { return true; }

		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;
	private:

		mutex_type& _mtx;
	};

	static inline bool Lock_Mutex(LockGuard::mutex_type& m) {
		SDL_LockMutex(m);
		return true;
	}

	static inline bool Unlock_Mutex(LockGuard::mutex_type& m) {
		SDL_UnlockMutex(m);
		return true;
	}
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 0

	// critical section using C++11
	class LockGuard {
	public:
		using mutex_type = std::mutex;

		LockGuard(mutex_type& m) : _mtx(m) {
			_mtx.lock();
		}

		~LockGuard() noexcept {
			_mtx.unlock();
		}

		explicit operator bool() { return true; }

		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;

	private:
		mutex_type& _mtx;
	};

	static inline bool Lock_Mutex(LockGuard::mutex_type& m) {
		m.lock();
		return true;
	}

	static inline bool Unlock_Mutex(LockGuard::mutex_type& m) {
#pragma warning(push) // for VC++, disable C26110
#pragma warning(disable: 26110)
		m.unlock();
#pragma warning(pop)
		return true;
	}
#else
	// no critical section (dummy, returns always true)
	<template typename T> static bool LockGuard(T& x) { return true;  }
#endif
}