/**
 * @file
 * @brief Header file for MSVC compiles
**/

#ifndef __msvc_h
#define __msvc_h

#if defined(TARGET_COMPILER_VC)

#include <io.h>
#include <math.h>

#define fileno _fileno
#define itoa _itoa
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strdup _strdup
#define stricmp _stricmp
#define strlwr _strlwr
#define strncasecmp _strnicmp
#define strnicmp _strnicmp
#define ftruncate _chsize
#define putenv _putenv

// No va_copy in MSVC
#if !defined(va_copy)
#define va_copy(dst, src) \
   ((void) memcpy(&(dst), &(src), sizeof(va_list)))
#endif

#pragma warning(disable : 4290)
#pragma warning(disable : 4351)
// bool -> int
#pragma warning(disable : 4800)

// struct vs class XXX: fix these some day!
#pragma warning(disable : 4099)

// truncating conversions XXX: fix these too!
#pragma warning(disable : 4244)


inline double round(double x)
{
    if (x >= 0.0)
        return floor(x + 0.5);
    else
        return ceil(x - 0.5);
}

#endif /* defined(TARGET_COMPILER_VC) */

#endif
