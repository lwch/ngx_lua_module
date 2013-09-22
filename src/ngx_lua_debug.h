#ifndef _NGX_LUA_DEBUG_H_
#define _NGX_LUA_DEBUG_H_

#include <stdio.h>

#if defined(NGX_DEBUG) && (NGX_DEBUG)
#   if (NGX_HAVE_C99_VARIADIC_MACROS)
#       define dbg(...) do { \
                            fprintf(stderr, "func: %s\n", __func__); \
                            fprintf(stderr, "\033[31m"); \
                            fprintf(stderr, __VA_ARGS__); \
                            fprintf(stderr, "\033[0m"); \
                            fprintf(stderr, "file: %s line: %d.\n", __FILE__, __LINE__); \
                        } while (0)
#   else
inline void dbg(const char* fmt, ...)
{
    va_list l;
    fprintf(stderr, "func: %s\n", __func__);
    fprintf(stderr, "\033[31m");
    va_start(l, fmt);
    vfprintf(stderr, fmt, l);
    va_end(l);
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "file: %s line: %d.\n", __FILE__, __LINE__);
}
#   endif
#else
#   if (NGX_HAVE_C99_VARIADIC_MACROS)
#       define dbg(...)
#   else
inline void dbg(const char* fmt, ...)
{
}
#   endif
#endif

#endif

