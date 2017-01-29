#ifndef __WIN32_UTIL_H
#define __WIN32_UTIL_H

#include <windows.h>
#include <stdint.h>

inline int32_t Win32_GetRectWidth(RECT rect) 
{
    return rect.right - rect.left;
}

inline int32_t Win32_GetRectHeight(RECT rect)
{
    return rect.bottom - rect.top;   
}

#endif