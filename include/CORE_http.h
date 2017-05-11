/*
 * CORE_http.h: A single-file library for performing asynchronous HTTP operations.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_HTTP_H__
#define __CORE_HTTP_H__

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* @summary Retrieve the alignment of a particular type, in bytes.
 * @param _type A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignOf
#define CORE_AlignOf(_type)                                                    \
    __alignof(_type)
#endif

/* @summary Align a non-zero size up to the nearest even multiple of a given power-of-two.
 * @param _quantity is the size value to align up.
 * @param _alignment is the desired power-of-two alignment.
 */
#ifndef CORE_AlignUp
#define CORE_AlignUp(_quantity, _alignment)                                    \
    (((_quantity) + ((_alignment)-1)) & ~((_alignment)-1))
#endif

/* @summary For a given address, return the address aligned for a particular type.
 * @param _address The unaligned address.
 * @param _type A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignFor
#define CORE_AlignFor(_address, _type)                                         \
    ((void*)(((uint8_t*)(_address)) + ((((__alignof(_type))-1)) & ~((__alignof(_type))-1))))
#endif

/* Forward-declare types exported by the library */
struct _CORE_HTTP_SYSTEM;
struct _CORE_HTTP_SYSTEM_INIT;
struct _CORE_HTTP_CONNECTION_POOL; /* per-thread */
struct _CORE_HTTP_CONNECTION_POOL_INIT;
struct _CORE_HTTP_SYSTEM_PROXY_CONFIG;
struct _CORE_HTTP_SYSTEM_TIMEOUTS;

/* @summary Define the data associated with an HTTP proxy configuration.
 */
typedef struct _CORE_HTTP_SYSTEM_PROXY_CONFIG {
    WCHAR                         *ProxyName;                /* A nul-terminated string specifying the proxy to use for requests, or NULL to use the default or no proxy. */
    WCHAR                         *ProxyBypass;              /* A nul-terminated string of semicolon-delimited hostnames or IP addresses to bypass the proxy. */
    int32_t                        ProxyType;                /* One of _CORE_HTTP_PROXY_TYPE specifying the proxy configuration to use. */
} CORE_HTTP_SYSTEM_PROXY_CONFIG;

/* @summary Define the data used to configure the timeout values for various parts of the request.
 */
typedef struct _CORE_HTTP_SYSTEM_TIMEOUTS {
    int32_t                        DnsResolveTimeout;        /* The timeout, in milliseconds, for DNS resolution. */
    int32_t                        ConnectTimeout;           /* The timeout, in milliseconds, for establishing a connection to a server. */
    int32_t                        ReceiveTimeout;           /* The timeout, in milliseconds, for socket receive operations. */
    int32_t                        TransmitTimeout;          /* The timeout, in milliseconds, for socket send operations. */
} CORE_HTTP_SYSTEM_TIMEOUTS;

/* @summary Define the data associated with the asynchronous HTTP request system.
 * The HTTP request system maintains a pool of worker threads for processing HTTP requests.
 */
typedef struct _CORE_HTTP_SYSTEM {
    HINTERNET                      WinHttpSession;           /* The WinHTTP session handle. */
    HANDLE                         SessionClosed;            /* A manual-reset event that becomes signaled when the session handle is closed. */
    size_t                         ThreadCount;              /* The number of worker threads in the HTTP system thread pool. */
    uint32_t                       UsageFlags;               /* One or more flags from the _CORE_HTTP_SYSTEM_FLAGS enumeration. */
    CORE_HTTP_SYSTEM_PROXY_CONFIG  DefaultProxy;             /* The system default proxy configuration. */
    CORE_HTTP_SYSTEM_PROXY_CONFIG  CurrentUserProxy;         /* The proxy configuration for the current user. */
    CORE_HTTP_SYSTEM_TIMEOUTS      DefaultTimeouts;          /* The configured timeout values for the system. The timeouts may be overridden per-request. */
} CORE_HTTP_SYSTEM;

/* @summary Define the data used to initialize and configure the HTTP library on the system.
 */
typedef struct _CORE_HTTP_SYSTEM_INIT {
    size_t                         ThreadCount;              /* The number of worker threads used for processing HTTP requests and responses. */
    uint32_t                       UsageFlags;               /* One or more flags from the _CORE_HTTP_SYSTEM_FLAGS enumeration. */
    int32_t                        ProxyType;                /* One of _CORE_HTTP_PROXY_TYPE specifying the type of proxy to use. If this value is CORE_HTTP_PROXY_TYPE_NAMED, specify the proxy configuration. */
    CORE_HTTP_SYSTEM_TIMEOUTS      Timeouts;                 /* The timeout values used for various portions of request execution. */
    CORE_HTTP_SYSTEM_PROXY_CONFIG *ProxyConfig;              /* The optional proxy configuration to use. Set to NULL unless ProxyType is CORE_HTTP_PROXY_TYPE_NAMED. */
} CORE_HTTP_SYSTEM_INIT;

/* @summary Define the supported proxy usage types.
 */
typedef enum _CORE_HTTP_PROXY_TYPE {
    CORE_HTTP_PROXY_TYPE_DEFAULT         =  0,               /* Use the proxy configured by the system in the registry, or attempt to auto-discover the proxy settings. */
    CORE_HTTP_PROXY_TYPE_DISABLED        =  1,               /* Do not use any proxy. */
    CORE_HTTP_PROXY_TYPE_NAMED           =  2,               /* The proxy is specified by name. */
    CORE_HTTP_PROXY_TYPE_AUTO_CONFIGURE  =  3,               /* The proxy configuration specifies the auto-configuration URL. */
} CORE_HTTP_PROXY_TYPE;

/* @summary Define the usage flags that control global HTTP system behavior.
 */
typedef enum _CORE_HTTP_SYSTEM_FLAGS {
    CORE_HTTP_SYSTEM_FLAGS_NONE          = (0UL <<  0),      /* No special usage flags are specified; use the default configuration. */
    CORE_HTTP_SYSTEM_FLAG_ENABLE_TRACING = (1UL <<  0),      /* Enable WinHTTP event tracing for debugging and packet sniffing. */
} CORE_HTTP_SYSTEM_FLAGS;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Retrieve the default system proxy configuration.
 * @param config The proxy configuration information to populate. Use CORE_DeleteHttpProxyConfig to free the returned memory.
 * @return Zero if the proxy configuration is successfully retrieved, or -1 if an error occurred.
 */
CORE_API(int)
CORE_QueryHttpSystemDefaultProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
);

/* @summary Retrieve the system proxy configuration for the current user.
 * @param config The proxy configuration information to populate. Use CORE_DeleteHttpProxyConfig to free the returned memory.
 * @return Zero if the proxy configuration is successfully retrieved, or -1 if an error occurred.
 */
CORE_API(int)
CORE_QueryHttpCurrentUserProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
);

/* @summary Free the resources allocated for a proxy configuration returned by CORE_QueryHttp*ProxyConfig.
 * @param config The proxy configuration information to delete.
 */
CORE_API(void)
CORE_DeleteHttpProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
);

/* @summary Initialize an HTTP timeout structure with the default values.
 * @param timeouts The timeouts to initialize with default values.
 */
CORE_API(void)
CORE_InitHttpSystemTimeouts
(
    CORE_HTTP_SYSTEM_TIMEOUTS *timeouts
);

/* @summary Initialize the HTTP system and spawn the pool of worker threads.
 * @param http The CORE_HTTP_SYSTEM to initialize.
 * @param init Information used to configure the HTTP system.
 * @return Zero if the HTTP system is successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_InitHttpSystem
(
    CORE_HTTP_SYSTEM      *http, 
    CORE_HTTP_SYSTEM_INIT *init
);

/* @summary Shut down the HTTP system and free associated resources.
 * @param http The HTTP system to shut down.
 */
CORE_API(void)
CORE_ShutdownHttpSystem
(
    CORE_HTTP_SYSTEM *http
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_HTTP_H__ */

#ifdef CORE_HTTP_IMPLEMENTATION

static void CALLBACK
CORE__WinHttpStatusCallback
(
    HINTERNET   handle, 
    DWORD_PTR  context, 
    DWORD       status,
    LPVOID status_info,
    DWORD    info_size
)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(status);
    UNREFERENCED_PARAMETER(status_info);
    UNREFERENCED_PARAMETER(info_size);
}

CORE_API(int)
CORE_QueryHttpSystemDefaultProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
)
{
    WINHTTP_PROXY_INFO info;
    if (WinHttpGetDefaultProxyConfiguration(&info))
    {   /* the proxy configuration was successfully retrieved */
        ZeroMemory(config, sizeof(CORE_HTTP_SYSTEM_PROXY_CONFIG));
        config->ProxyName   = info.lpszProxy;
        config->ProxyBypass = info.lpszProxyBypass;
        switch (info.dwAccessType)
        {
            case WINHTTP_ACCESS_TYPE_NO_PROXY:
                { config->ProxyType = CORE_HTTP_PROXY_TYPE_DISABLED;
                } break;
            case WINHTTP_ACCESS_TYPE_DEFAULT_PROXY:
                { config->ProxyType = CORE_HTTP_PROXY_TYPE_DEFAULT;
                } break;
            case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
                { config->ProxyType = CORE_HTTP_PROXY_TYPE_NAMED;
                } break;
            default:
                { assert(false && "Unsupported proxy type");
                  config->ProxyType = CORE_HTTP_PROXY_TYPE_DISABLED;
                } break;
        }
        return  0;
    }
    else
    {   /* failed to retrieve the proxy configuration from the registry - see GetLastError */
        ZeroMemory(config, sizeof(CORE_HTTP_SYSTEM_PROXY_CONFIG));
        return -1;
    }
}

CORE_API(int)
CORE_QueryHttpCurrentUserProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG info;
    if (WinHttpGetIEProxyConfigForCurrentUser(&info))
    {   /* the proxy configuration was successfully retrieved */
        ZeroMemory(config, sizeof(CORE_HTTP_SYSTEM_PROXY_CONFIG));
        if (info.fAutoDetect)
        {   /* set to automatically detect settings; use the proxy URL or auto-config URL */
            if (info.lpszProxy != NULL)
            {
                config->ProxyName = info.lpszProxy;
                config->ProxyType = CORE_HTTP_PROXY_TYPE_NAMED;
            }
            else
            {
                config->ProxyName = info.lpszAutoConfigUrl;
                config->ProxyType = CORE_HTTP_PROXY_TYPE_AUTO_CONFIGURE;
            }
            config->ProxyBypass = info.lpszProxyBypass;
        }
        else
        {   /* the proxy is manually configured */
            if (info.lpszProxy != NULL)
            {
                config->ProxyName = info.lpszProxy;
                config->ProxyType = CORE_HTTP_PROXY_TYPE_NAMED;
            }
            else
            {   /* no proxy is configured */
                config->ProxyName = NULL;
                config->ProxyType = CORE_HTTP_PROXY_TYPE_DISABLED;
            }
            config->ProxyBypass = info.lpszProxyBypass;
        }
        /* prevent a memory leak if one of the strings is unused */
        if (info.lpszAutoConfigUrl != NULL && config->ProxyName != info.lpszAutoConfigUrl)
            GlobalFree(info.lpszAutoConfigUrl);
        if (info.lpszProxy != NULL && config->ProxyName != info.lpszProxy)
            GlobalFree(info.lpszProxy);
        return  0;
    }
    else
    {   /* failed to retrieve the proxy configuration - see GetLastError */
        ZeroMemory(config, sizeof(CORE_HTTP_SYSTEM_PROXY_CONFIG));
        return -1;
    }
}

CORE_API(void)
CORE_DeleteHttpProxyConfig
(
    CORE_HTTP_SYSTEM_PROXY_CONFIG *config
)
{
    if (config != NULL)
    {
        if (config->ProxyName != NULL)
            GlobalFree(config->ProxyName);
        if (config->ProxyBypass != NULL)
            GlobalFree(config->ProxyBypass);
        ZeroMemory(config, sizeof(CORE_HTTP_SYSTEM_PROXY_CONFIG));
    }
}

CORE_API(void)
CORE_InitHttpSystemTimeouts
(
    CORE_HTTP_SYSTEM_TIMEOUTS *timeouts
)
{
    timeouts->DnsResolveTimeout = 30 * 1000; /* 30 seconds */
    timeouts->ConnectTimeout    = 60 * 1000; /* 60 seconds */
    timeouts->ReceiveTimeout    = 30 * 1000; /* 30 seconds */
    timeouts->TransmitTimeout   = 30 * 1000; /* 30 seconds */
}

CORE_API(int)
CORE_InitHttpSystem
(
    CORE_HTTP_SYSTEM      *http, 
    CORE_HTTP_SYSTEM_INIT *init
)
{
    HANDLE    close_evt = NULL;
    HINTERNET   session = NULL;
    WCHAR   *proxy_name = WINHTTP_NO_PROXY_NAME;
    WCHAR *proxy_bypass = WINHTTP_NO_PROXY_BYPASS;
    DWORD        access = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

    if (init->UsageFlags & CORE_HTTP_SYSTEM_FLAG_ENABLE_TRACING)
    {   /* enable tracing for all HTTP requests submitted through the system */
        BOOL enable_tracing = TRUE;
        (void) WinHttpSetOption(NULL, WINHTTP_OPTION_ENABLETRACING, &enable_tracing, sizeof(BOOL));
    }
    else
    {   /* disable tracing for the session */
        BOOL enable_tracing = FALSE;
        (void) WinHttpSetOption(NULL, WINHTTP_OPTION_ENABLETRACING, &enable_tracing, sizeof(BOOL));
    }
    if (init->ThreadCount != 0)
    {   /* specify the number of worker threads to use */
        ULONG thread_count = (ULONG) init->ThreadCount;
        (void) WinHttpSetOption(NULL, WINHTTP_OPTION_WORKER_THREAD_COUNT, &thread_count, sizeof(ULONG));
    }
    switch (init->ProxyType)
    {
        case CORE_HTTP_PROXY_TYPE_DEFAULT:
            { access       = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
            } break;
        case CORE_HTTP_PROXY_TYPE_DISABLED:
            { access       = WINHTTP_ACCESS_TYPE_NO_PROXY;
            } break;
        case CORE_HTTP_PROXY_TYPE_NAMED:
            { assert(init->ProxyConfig != NULL);
              access       = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
              proxy_name   = init->ProxyConfig->ProxyName;
              proxy_bypass = init->ProxyConfig->ProxyBypass;
            } break;
        default:
            { assert(false && "Unsupported proxy type");
            } goto cleanup_and_fail;
    }
    /* open the WinHttp session and set all of the global options */
    if ((session = WinHttpOpen(L"CORE/1.0", access, proxy_name, proxy_bypass, WINHTTP_FLAG_ASYNC)) == NULL)
        goto cleanup_and_fail;
    if ((close_evt = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
        goto cleanup_and_fail;
    if (!WinHttpSetOption(session, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &close_evt, sizeof(HANDLE)))
        goto cleanup_and_fail;
    if (!WinHttpSetOption(session, WINHTTP_OPTION_RESOLVE_TIMEOUT, &init->Timeouts.DnsResolveTimeout, sizeof(int32_t)))
        goto cleanup_and_fail;
    if (!WinHttpSetOption(session, WINHTTP_OPTION_CONNECT_TIMEOUT, &init->Timeouts.ConnectTimeout, sizeof(int32_t)))
        goto cleanup_and_fail;
    if (!WinHttpSetOption(session, WINHTTP_OPTION_RECEIVE_TIMEOUT, &init->Timeouts.ReceiveTimeout, sizeof(int32_t)))
        goto cleanup_and_fail;
    if (!WinHttpSetOption(session, WINHTTP_OPTION_SEND_TIMEOUT   , &init->Timeouts.TransmitTimeout, sizeof(int32_t)))
        goto cleanup_and_fail;
    /* initialize the CORE_HTTP_SYSTEM object */
    if (CORE_QueryHttpSystemDefaultProxyConfig(&http->DefaultProxy) < 0)
        goto cleanup_and_fail;
    if (CORE_QueryHttpCurrentUserProxyConfig(&http->CurrentUserProxy) < 0)
    {
        CORE_DeleteHttpProxyConfig(&http->DefaultProxy);
        goto cleanup_and_fail;
    }
    http->WinHttpSession  = session;
    http->SessionClosed   = close_evt;
    http->ThreadCount     = init->ThreadCount;
    http->UsageFlags      = init->UsageFlags;
    CopyMemory(&http->DefaultTimeouts, &init->Timeouts, sizeof(CORE_HTTP_SYSTEM_TIMEOUTS));
    /* install the status callback; we're prepared to receive notifications */
    if (WinHttpSetStatusCallback(session, CORE__WinHttpStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, (DWORD_PTR) NULL) == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        CORE_DeleteHttpProxyConfig(&http->CurrentUserProxy);
        CORE_DeleteHttpProxyConfig(&http->DefaultProxy);
        goto cleanup_and_fail;
    }
    return 0;

cleanup_and_fail:
    if (close_evt != NULL) CloseHandle(close_evt);
    if (session != NULL) WinHttpCloseHandle(session);
    ZeroMemory(http, sizeof(CORE_HTTP_SYSTEM));
    return -1;
}

CORE_API(void)
CORE_ShutdownHttpSystem
(
    CORE_HTTP_SYSTEM *http
)
{
    if (http->WinHttpSession != NULL)
    {   /* close the session handle - note that callbacks may still be received */
        WinHttpCloseHandle(http->WinHttpSession);
    }
    if (http->SessionClosed != NULL)
    {   /* wait for the session handle to actually be closed */
        WaitForSingleObject(http->SessionClosed, INFINITE);
        CloseHandle(http->SessionClosed);
    }
    /* at this point, resources can be freed - no more callbacks will occur */
    if (http->DefaultProxy.ProxyName != NULL)
    {
        GlobalFree(http->DefaultProxy.ProxyName);
    }
    if (http->DefaultProxy.ProxyBypass != NULL)
    {
        GlobalFree(http->DefaultProxy.ProxyBypass);
    }
    if (http->CurrentUserProxy.ProxyName != NULL)
    {
        GlobalFree(http->CurrentUserProxy.ProxyName);
    }
    if (http->CurrentUserProxy.ProxyBypass != NULL)
    {
        GlobalFree(http->CurrentUserProxy.ProxyBypass);
    }
    ZeroMemory(http, sizeof(CORE_HTTP_SYSTEM));
}

#endif /* CORE_HTTP_IMPLEMENTATION */

