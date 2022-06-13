#pragma once

// M5_SCREEN_HIRES: internal buffer size 1:640x480 0:320x240
#if !defined(ESP32)
# ifndef M5_SCREEN_HIRES
#  define M5_SCREEN_HIRES 1
# endif
#endif

// Use game controller as default
// - if set 0, activate by command arg "-J"
#if !defined(MWM5_USE_GAMECONTROLLER)
#  define MWM5_USE_GAMECONTROLLER 0
#endif

// Separate rendering and application loop by threads.
// if set 1, use separate thread for application loop. In result,
// the period of application loop will be less affected by the
// screen refresh rate other than 60fps.
// NOTE: EXPERIMENTAL SO FAR, NEEDS TO CHECK RESOURCE ACCESS 
//       CONFLICTS BETWEEN APP THREAD AND RENDERING THREAD.
//       (e.g. Input FIFO, UART FIFO)
#if !defined(MWM5_SDL2_USE_MULTITHREAD_RENDER)
# define MWM5_SDL2_USE_MULTITHREAD_RENDER 1
#endif

// USE SDL_MutexXXX(), otherwise std::mutex.
#if !defined(MWM5_USE_SDL2_MUTEX)
# define MWM5_USE_SDL2_MUTEX 0
#endif
