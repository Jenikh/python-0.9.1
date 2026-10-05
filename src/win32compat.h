/* Small compatibility layer for the 1991 sources when built with MinGW. */
#ifndef PYTHON091_WIN32COMPAT_H
#define PYTHON091_WIN32COMPAT_H

/* The old source uses 'byte' as an identifier. */
#define byte win32_compat_byte
#include <windows.h>
#undef byte
#include <io.h>

/* Windows headers use these names for unrelated purposes. */
#undef INCREF
#undef DECREF
#undef IN
#undef IS

#ifndef isatty
#define isatty _isatty
#endif

#ifndef fileno
#define fileno _fileno
#endif

#ifndef sleep
#define sleep(seconds) Sleep((DWORD)((seconds) * 1000))
#endif

#endif
