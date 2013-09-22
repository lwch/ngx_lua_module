#ifndef _NGX_LUA_DEBUG_H_
#define _NGX_LUA_DEBUG_H_

#include <stdio.h>

#if defined(DDEBUG) && (DDEBUG)
#   if (NGX_HAVE_VARIADIC_MACROS)
#       define dbg(...) do { \
                            fprintf(stderr, "func: %s\n", __func__); \
                            fprintf(stderr, __VA_ARGS__); \
                            fprintf(stderr, "\nfile: %s line: %d.\n", __FILE__, __LINE__); \
                        } while (0)
#   else
void dbg(const char* fmt, ...)
{
    va_list l;
    va_start(l, fmt);
    vfprintf(stderr, fmt, l);
    va_end(l);
}
#   endif
#else
#   if (NGX_HAVE_VARIADIC_MACROS)
#       define dbg(...)
#   else
void dbg(const char* fmt, ...)
{
}
#   endif
#endif

#endif

