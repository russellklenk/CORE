/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implement the entry point and main loop of the HTTP test app.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Define some useful macros for specifying common resource sizes.
#ifndef Kilobytes
    #define Kilobytes(x)                            (size_t((x)) * size_t(1024))
#endif
#ifndef Megabytes
    #define Megabytes(x)                            (size_t((x)) * size_t(1024) * size_t(1024))
#endif
#ifndef Gigabytes
    #define Gigabytes(x)                            (size_t((x)) * size_t(1024) * size_t(1024) * size_t(1024))
#endif

/// @summary Define macros for controlling compiler inlining.
#ifndef never_inline
    #define never_inline                            __declspec(noinline)
#endif
#ifndef force_inline
    #define force_inline                            __forceinline
#endif

/// @summary Helper macro to align a size value up to the next even multiple of a given power-of-two.
#ifndef align_up
    #define align_up(x, a)                          ((x) == 0) ? (a) : (((x) + ((a)-1)) & ~((a)-1))
#endif

/// @summary Helper macro to write a message to stdout.
#ifndef ConsoleOutput
    #ifndef NO_CONSOLE_OUTPUT
        #define ConsoleOutput(fmt_str, ...)         _ftprintf(stdout, _T(fmt_str), __VA_ARGS__)
    #else
        #define ConsoleOutput(fmt_str, ...)         
    #endif
#endif

/// @summary Helper macro to write a message to stderr.
#ifndef ConsoleError
    #ifndef NO_CONSOLE_OUTPUT
        #define ConsoleError(fmt_str, ...)          _ftprintf(stderr, _T(fmt_str), __VA_ARGS__)
    #else
        #define ConsoleError(fmt_str, ...)          
    #endif
#endif

#define CORE_HTTP_IMPLEMENTATION

/*////////////////
//   Includes   //
////////////////*/
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <process.h>

#include <tchar.h>
#include <Windows.h>
#include <Winhttp.h>

#include "CORE_http.h"

/*//////////////////
//   Data Types   //
//////////////////*/

/*///////////////
//   Globals   //
///////////////*/

/*//////////////////////////
//   Internal Functions   //
//////////////////////////*/

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Implement the entry point of the application.
/// @param argc The number of arguments passed on the command line.
/// @param argv An array of @a argc zero-terminated strings specifying the command-line arguments.
/// @return Zero if the function completes successfully, or non-zero otherwise.
int 
main
(
    int    argc, 
    char **argv
)
{
    CORE_HTTP_SYSTEM      http;
    CORE_HTTP_SYSTEM_INIT http_init;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    http_init.ThreadCount = 4;
    http_init.UsageFlags  = CORE_HTTP_SYSTEM_FLAG_ENABLE_TRACING;
    http_init.ProxyType   = CORE_HTTP_PROXY_TYPE_DEFAULT;
    http_init.ProxyConfig = NULL;
    CORE_InitHttpSystemTimeouts(&http_init.Timeouts);
    if (CORE_InitHttpSystem(&http, &http_init) < 0)
    {
        ConsoleError("ERROR: Failed to initialize HTTP system (%08X).\n", GetLastError());
        return 1;
    }
    ConsoleOutput("Successfully initialized HTTP system.\n");
    CORE_ShutdownHttpSystem(&http);
    ConsoleOutput("Successfully shut down HTTP system.\n");
    return 0;
}


