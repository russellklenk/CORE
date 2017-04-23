/*
 * CORE_asyncio.h: A single-file library for performing asynchronous socket,  
 * HTTP and disk I/O (along with some convenience routines for performing 
 * memory-mapped I/O.)
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_ASYNCIO_H__
#define __CORE_ASYNCIO_H__

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

#ifndef CORE_ASYNCIO_INVALID_FILE
#define CORE_ASYNCIO_INVALID_FILE         INVALID_HANDLE_VALUE
#endif

#ifndef CORE_ASYNCIO_INVALID_SOCKET
#define CORE_ASYNCIO_INVALID_SOCKET       INVALID_SOCKET
#endif

#ifndef CORE_ASYNCIO_INVALID_HTTP
#define CORE_ASYNCIO_INVALID_HTTP         NULL
#endif

/* Forward-declare types exported by the library */
union  _CORE_ASYNCIO_HANDLE;
struct _CORE_FILE_MAPPING;
struct _CORE_FILE_DATA;
struct _CORE_ASYNCIO_OPERATION;
struct _CORE_ASYNCIO_OPERATION_RESULT;
struct _CORE_ASYNCIO_RESULT;
struct _CORE_ASYNCIO_PROFILE;
struct _CORE_ASYNCIO_CONTEXT;
struct _CORE_ASYNCIO_REQUEST;
struct _CORE_ASYNCIO_REQUEST_POOL;
struct _CORE_ASYNCIO_REQUEST_POOL_INIT;

/* @summary Define the signature for the callback function invoked when an I/O operation has completed.
 * The callback runs on the I/O thread pool, and should perform a minimal amount of non-blocking work.
 * @param success This value is non-zero if the request executed successfully.
 * @param result Data describing the result of the I/O operation.
 * @param context Data describing the execution environment for the I/O operation. This data can be used to allocate and submit a chained I/O operation.
 * @param profile Data describing timing information for the I/O operation.
 * @return A chained I/O request to execute immediately on the same I/O thread, or NULL.
 */
typedef struct _CORE_ASYNCIO_REQUEST* (*CORE_AsyncIoCompletion_Func)
(
    int                           success,
    struct _CORE_ASYNCIO_RESULT   *result, 
    struct _CORE_ASYNCIO_CONTEXT *context, 
    struct _CORE_ASYNCIO_PROFILE *profile
);

/* @summary Define the signature for the callback function invoked to perform initialization for an I/O worker thread.
 * The callback should allocate any per-thread data it needs and return a pointer to that data in the thread_context parameter.
 * @param pool The I/O thread pool that owns the worker thread.
 * @param pool_context Opaque data supplied when the I/O pool was created.
 * @param thread_id The operating system identifier of the worker thread.
 * @param thread_context On return, the function should update this value to point to any data to be associated with the thread.
 * @return Zero if initialization completes successfully, or -1 if initialization failed.
 */
typedef int (*CORE_AsyncIoWorkerThreadInit_Func)
(
    struct _CORE_ASYNCIO_THREAD_POOL *pool, 
    uintptr_t                 pool_context,
    uint32_t                     thread_id,
    uintptr_t              *thread_context
);

/* @summary Define a union for storing the different types of handles the async I/O system can work with.
 */
typedef union _CORE_ASYNCIO_HANDLE {
    HINTERNET                          HttpRequest;               /* The WinHTTP request handle. */
    SOCKET                             Socket;                    /* The WinSock2 socket handle. */
    HANDLE                             File;                      /* The Win32 file handle. */
} CORE_ASYNCIO_HANDLE;

/* @summary Define the data associated with a memory-mapped file opened for read access.
 */
typedef struct _CORE_FILE_MAPPING {
    HANDLE                             Filedes;                   /* A valid file handle, or INVALID_HANDLE_VALUE. */
    HANDLE                             Filemap;                   /* A valid file mapping handle, or NULL. */
    int64_t                            FileSize;                  /* The size of the file, in bytes, at the time it was opened. */
    uint64_t                           Granularity;               /* The system VMM allocation granularity, in bytes. */
} CORE_FILE_MAPPING;

/* @summary Define the data associated with a region of a file loaded or mapped into memory.
 */
typedef struct _CORE_FILE_DATA {
    uint8_t                           *Buffer;                    /* The buffer containing the loaded file data. */
    void                              *MapPtr;                    /* The address returned by MapViewOfFile. */
    int64_t                            Offset;                    /* The offset of the data in this file data region from the start of the file. */
    int64_t                            DataSize;                  /* The number of bytes in Buffer or MapPtr that are valid to access. */
    uint32_t                           Flags;                     /* One or more values of the _CORE_FILE_DATA_FLAGS enumeration. */
} CORE_FILE_DATA;

/* @summary Define the data used to execute a low-level I/O operation. Not all data is used by all operations.
 */
typedef struct _CORE_ASYNCIO_OPERATION {
    CORE_ASYNCIO_HANDLE                Handle;                    /* The handle associated with the operation. */
    WCHAR                             *FilePath;                  /* A nul-terminated string specifying the file path. Used for file OPEN operations only. */
    HANDLE                             CompletionPort;            /* The I/O completion port to associate with the file or socket, or NULL. */
    OVERLAPPED                        *Overlapped;                /* The OVERLAPPED object to use for asynchronous I/O, or NULL to use synchronous I/O. */
    void                              *DataBuffer;                /* The source buffer specifying the data to write, the destination buffer for data being read, or a pointer to a structure specifying HTTP request attributes. */
    int64_t                            FileOffset;                /* The absolute byte offset within the file at which to perform the I/O operation. */
    int64_t                            PreallocationSize;         /* The desired size of the file, if the request HintFlags specifies CORE_ASYNCIO_HINT_FLAG_PREALLOCATE. */
    uint32_t                           TransferAmount;            /* The number of bytes to transfer. */
    uint32_t                           HintFlags;                 /* One or more _CORE_ASYNCIO_HINT_FLAGS. */
} CORE_ASYNCIO_OPERATION;

/* @summary Define the data returned by a low-level I/O operation.
 */
typedef struct _CORE_ASYNCIO_OPERATION_RESULT {
    CORE_ASYNCIO_HANDLE                Handle;                    /* The handle associated with or opened by the operation. */
    uintptr_t                          ContextData;               /* Opaque data associated with the request. */
    DWORD                              ResultCode;                /* The operating system result code. */
    uint32_t                           ResultFlags;               /* One or more _CORE_ASYNCIO_RESULT_FLAGS specifying whether the request completed successfully, etc. */
    uint32_t                           TransferAmount;            /* The number of bytes of data transferred. */
    uin32_t                            Reserved;                  /* Reserved for future use. Set to zero. */
} CORE_ASYNCIO_OPERATION_RESULT;

/* @summay Define the data associated with a background file I/O request.
 */
typedef struct _CORE_ASYNCIO_REQUEST_DATA_FILE {
    WCHAR                             *PathBuffer;                /* Pointer to a caller-managed buffer specifying the path of the file to open. */
    void                              *DataBuffer;                /* The caller-managed buffer from which data will be read or written. */
    int64_t                            DataAmount;                /* The number of bytes to transfer to or from the caller-managed data buffer. */
    int64_t                            BaseOffset;                /* The byte offset of the start of the operation from the satrt of the physical file. */
    int64_t                            FileOffset;                /* The byte offset of the start of the operation from the start of the logical file. */
} CORE_ASYNCIO_REQUEST_DATA_FILE;

/* @summary Define the data associated with a background I/O request.
 * CORE_ASYNCIO_REQUEST_MAX_DATA defines the maximum size of the request-specific data that can be associated with the request.
 */
#ifndef CORE_ASYNCIO_REQUEST_MAX_DATA
#define CORE_ASYNCIO_REQUEST_MAX_DATA  64
#endif
typedef struct _CORE_ASYNCIO_REQUEST {
    #define NDATA CORE_ASYNCIO_REQUEST_MAX_DATA
    struct _CORE_ASYNCIO_REQUEST      *NextRequest;               /* The next node in the request list, or NULL if this is the tail node. */
    struct _CORE_ASYNCIO_REQUEST      *PrevRequest;               /* The previous node in the request list, or NULL if this is the head node. */
    struct _CORE_ASYNCIO_REQUEST_POOL *RequestPool;               /* The _CORE_ASYNCIO_REQUEST_POOL from which the request was allocated. */
    int32_t                            RequestType;               /* One of _CORE_ASYNCIO_REQUEST_TYPE specifying the type of the request. */
    int32_t                            RequestState;              /* One of _CORE_ASYNCIO_REQUEST_STATE specifying the state of the request. */
    OVERLAPPED                         Overlapped;                /* The OVERLAPPED instance associated with the asyncronous request. */
    CORE_ASYNCIO_HANDLE                Handle;                    /* The handle associated with the operation. */
    uintptr_t                          UserContext;               /* Opaque data to be passed through to the completion callback. */
    CORE_AsyncIoCompletion_Func        CompleteCallback;          /* The callback to invoke when the I/O operation completes. */
    uint64_t                           SubmitTime;                /* The timestamp, in ticks, at which the I/O request was submitted. */
    uint64_t                           LaunchTime;                /* The timestamp, in ticks, at which the I/O request began executing. */
    uint64_t                           FinishTime;                /* The timestamp, in ticks, at which the I/O request finished executing. */
    uint32_t                           HintFlags;                 /* One or more _CORE_ASYNCIO_HINT_FLAGS. */
    uint32_t                           Reserved;                  /* Reserved for future use. Set to zero. */
    uint8_t                            LocalData[NDATA];          /* Data associated with the request. The type of data depends on the type of request. */
    #undef  NDATA
} CORE_ASYNCIO_REQUEST;

/* @summary Define the data returned from a background I/O request through the completion callback.
 */
typedef struct _CORE_ASYNCIO_RESULT {
    int32_t                            RequestType;               /* One of _CORE_ASYNCIO_REQUEST_TYPE specifying the type of the request. */
    uint32_t                           ResultCode;                /* ERROR_SUCCESS or another operating system result code indicating the result of the operation. */
    uintptr_t                          UserContext;               /* Opaque data associated with the request and supplied by the requestor on submission. */
    void                              *ResultData;                /* A pointer to the data associated with the operation. The type of data depends on the type of request. */
    CORE_ASYNCIO_HANDLE                Handle;                    /* The handle associated with the operation. */
} CORE_ASYNCIO_RESULT;

/* @summary Define the execution environment data passed back to an I/O completion callback.
 * This data can be used to allocate and initializa a chained request.
 */
typedef struct _CORE_ASYNCIO_CONTEXT {
    struct _CORE_ASYNCIO_REQUEST_POOL *RequestPool;               /* The I/O request pool from which the I/O request was allocated. */
    struct _CORE_ASYNCIO_THREAD_POOL  *ThreadPool;                /* The thread pool that's handling the I/O request. */
} CORE_ASYNCIO_CONTEXT;

/* @summary Define the timing data tracked for each I/O operation.
 */
typedef struct _CORE_ASYNCIO_PROFILE {
    uint64_t                           QueueDelay;                /* The time, in nanoseconds, between when the I/O request was submitted and when it started executing. */
    uint64_t                           ExecutionTime;             /* The time, in nanoseconds, that the I/O request spent executing. */
    uint32_t                           ThreadId;                  /* The operating system identifier of the thread that executed the I/O request. */
} CORE_ASYNCIO_PROFILE;

/* @summary Define the data associated with a pool of background I/O requests. 
 * Each thread that can submit I/O requests typically maintains its own request pool.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL {
    struct _CORE_ASYNCIO_REQUEST      *LiveRequest;               /* Pointer to the first live request, or NULL if no requests are active. */
    struct _CORE_ASYNCIO_REQUEST      *FreeRequest;               /* Pointer to the first free request, or NULL if no requests are available. */
    struct _CORE_ASYNCIO_REQUEST      *RequestPool;               /* An array of PoolCapacity I/O request objects. This is the raw storage underlying the live and free lists. */
    void                              *PoolMemory;                /* A pointer to the memory that the pool uses for storage. */
    uint64_t                           PoolMemorySize;            /* The number of bytes allocated to the pool storage. */
    uint32_t                           PoolCapacity;              /* The maximum number of requests that can be allocated from the pool. */
    uint32_t                           OwningThread;              /* The operating system identifier of the thread that owns the pool. */
    CRITICAL_SECTION                   ListLock;                  /* Lock protecting live and free lists. These may be accessed concurrently by a submitting thread and a worker thread. */
} CORE_ASYNCIO_REQUEST_POOL;

/* @summary Define the configuration data used to create an I/O request pool.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL_INIT {
    uint32_t                           PoolCapacity;              /* The maximum number of requests that can be allocated from the pool. */
    uint32_t                           OwningThread;              /* The operating system identifier of the thread that will own the pool. */
    void                              *PoolMemory;                /* The memory that the pool uses for I/O request storage. */
    uint64_t                           PoolMemorySize;            /* The size of the PoolMemory block, in bytes. This value can be determined for a given pool capacity using CORE_QueryAsyncIoRequestPoolMemorySize. */
} CORE_ASYNCIO_REQUEST_POOL_INIT;

/* @summary Define the data associated with a pool of background I/O worker threads.
 */
typedef struct _CORE_ASYNCIO_THREAD_POOL {
    unsigned int                      *OSThreadIds;               /* An array of WorkerCount operating system thread identifiers, of which ActiveThreads entries are valid. */
    HANDLE                            *OSThreadHandle;            /* An array of WorkerCount operating system thread handles, of which ActiveThreads entries are valid.*/
    HANDLE                            *WorkerReady;               /* An array of WorkerCount manual-reset event handles, of which ActiveThreads entries are valid. */
    HANDLE                            *WorkerError;               /* An array of WorkerCount manual-reset event handles, of which ActiveThreads entries are valid. */
    HANDLE                             CompletionPort;            /* The I/O completion port to be monitored by all threads in the pool. */
    HANDLE                             TerminateSignal;           /* A manual-reset event to be signaled by the application when the pool should terminate. */
    uintptr_t                          ContextData;               /* Opaque data associated with the thread pool and available to all worker threads. */
    uint32_t                           ActiveThreads;             /* The number of currently active threads in the pool. */
    uint32_t                           WorkerCount;               /* The maximum number of active worker threads in the pool. */
    void                              *PoolMemory;                /* The owner-managed memory block used for all storage within the pool. */
    uint64_t                           PoolMemorySize;            /* The size of the pool memory block, in bytes. This value can be determined for a given pool capacity using CORE_QueryAsyncIoThreadPoolMemorySize. */
} CORE_ASYNCIO_THREAD_POOL;

/* @summary Define the configuration data used to create an I/O worker thread pool.
 */
typedef struct _CORE_ASYNCIO_THREAD_POOL_INIT {
    CORE_AsyncIoWorkerThreadInit_Func  ThreadInitFunc;            /* The callback function to invoke to create any thread-local data. */
    uintptr_t                          PoolContext;               /* Opaque data associated with the thread pool and available to all worker threads. */
    void                              *PoolMemory;                /* The owner-managed memory block used for all storage within the pool. */
    uint64_t                           PoolMemorySize;            /* The size of the pool memory block, in bytes. This value can be determined for a given pool capacity using CORE_QueryAsyncIoThreadPoolMemorySize. */
    uint32_t                           WorkerCount;               /* The number of worker threads in the pool. */
} CORE_ASYNCIO_THREAD_POOL_INIT;

/* @summary Define the supported types of asynchronous I/O requests.
 */
typedef enum _CORE_ASYNCIO_REQUEST_TYPE {
    CORE_ASYNCIO_REQUEST_TYPE_NOOP                   =  0,        /* Ignore the operation and pass through the data unchanged. */
    CORE_ASYNCIO_REQUEST_TYPE_OPEN_FILE              =  1,        /* Asynchronously open a file. */
    CORE_ASYNCIO_REQUEST_TYPE_READ_FILE              =  2,        /* Issue an explicit asynchronous file read. */
    CORE_ASYNCIO_REQUEST_TYPE_WRITE_FILE             =  3,        /* Issue an explicit asynchronous file write. */
    CORE_ASYNCIO_REQUEST_TYPE_FLUSH_FILE             =  4,        /* Issue an explicit asynchronous file flush. */
    CORE_ASYNCIO_REQUEST_TYPE_CLOSE_FILE             =  5,        /* Asynchronously close a file. */
} CORE_ASYNCIO_REQUEST_TYPE;

/* @summary Define the states of an asynchronous I/O request.
 */
typedef enum _CORE_ASYNCIO_REQUEST_STATE {
    CORE_ASYNCIO_REQUEST_STATE_CHAINED               =  0,        /* The I/O request has been submitted as a chained request, which executes immediately and is not queued. */
    CORE_ASYNCIO_REQUEST_STATE_SUBMITTED             =  1,        /* The I/O request has been submitted, but not yet launched. */
    CORE_ASYNCIO_REQUEST_STATE_LAUNCHED              =  2,        /* The I/O request has been picked up by a worker thread and is executing. */
    CORE_ASYNCIO_REQUEST_STATE_COMPLETED             =  3,        /* The I/O request has been completed. */
} CORE_ASYNCIO_REQUEST_STATE;

/* @summary Define hint flags that can be used to optimize asynchronous I/O operations.
 */
typedef enum _CORE_ASYNCIO_HINT_FLAGS {
    CORE_ASYNCIO_HINT_FLAGS_NONE                     = (0 <<  0), /* No I/O hints are specified. Use the default behavior appropriate for the operation. */
    CORE_ASYNCIO_HINT_FLAG_READ                      = (1 <<  0), /* Read operations will be issued against the file. */
    CORE_ASYNCIO_HINT_FLAG_WRITE                     = (1 <<  1), /* Write operations will be issued against the file. */
    CORE_ASYNCIO_HINT_FLAG_OVERWRITE                 = (1 <<  2), /* The existing file contents should be discarded. */
    CORE_ASYNCIO_HINT_FLAG_PREALLOCATE               = (1 <<  3), /* Preallocate the file to the specified size. */
    CORE_ASYNCIO_HINT_FLAG_SEQUENTIAL                = (1 <<  4), /* Optimize for sequential file access when performing cached/buffered I/O. */
    CORE_ASYNCIO_HINT_FLAG_UNCACHED                  = (1 <<  5), /* File I/O should bypass the operating system page cache. The source/destination buffer must be aligned to a sector boundary and have a size that's an even multiple of the disk sector size. */
    CORE_ASYNCIO_HINT_FLAG_WRITE_THROUGH             = (1 <<  6), /* Writes should be immediately flushed to disk. */
    CORE_ASYNCIO_HINT_FLAG_TEMPORARY                 = (1 <<  7), /* The file is temporary, and will be deleted when the file handle is closed. */
} CORE_ASYNCIO_HINT_FLAGS;

/* @summary Define flags specifying the allocation attributes of a file region.
 */
typedef enum _CORE_ASYNCIO_FILE_DATA_FLAGS {
    CORE_ASYNCIO_FILE_DATA_FLAGS_NONE                = (0 <<  0), /* The FILE_DATA object is invalid. */
    CORE_ASYNCIO_FILE_DATA_FLAG_MAPPED_REGION        = (1 <<  0), /* The FILE_DATA object represents a mapped region of a file. */
} CORE_ASYNCIO_FILE_DATA_FLAGS;

/* @summary Define the flags indicating how an I/O operation completed.
 */
typedef enum _CORE_ASYNCIO_RESULT_FLAGS {
    CORE_ASYNCIO_RESULT_FLAGS_NONE                   = (0 <<  0), /* No flags are specified. */
    CORE_ASYNCIO_RESULT_FLAG_SUCCESS                 = (1 <<  0), /* The I/O operation completed successfully. */
    CORE_ASYNCIO_RESULT_FLAG_COMPLETED_SYNCHRONOUSLY = (1 <<  1), /* The I/O operation completed synchronously. */
} CORE_ASYNCIO_RESULT_FLAGS;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

/* @summary Calculate the amount of memory required to back an I/O request pool of the specified size.
 * @param max_requests The maximum number of requests that can be allocated from the pool.
 * @return The size, in bytes, required to create an I/O request pool of the specified size.
 */
CORE_API(size_t)
CORE_QueryAsyncIoRequestPoolMemorySize
(
    size_t max_requests
);

/* @summary Create an I/O request pool.
 * @param pool The CORE_ASYNCIO_REQUEST_POOL to initialize.
 * @param init Data used to configure the request pool.
 * @return Zero if the pool is created successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_InitIoRequestPool
(
    CORE_ASYNCIO_REQUEST_POOL      *pool, 
    CORE_ASYNCIO_REQUEST_POOL_INIT *init
);

/* @summary Acquire an I/O request object from a pool.
 * @param pool The I/O request pool from which the request object should be acquired.
 * @return The I/O request object, or NULL if the pool has no available requests.
 */
CORE_API(CORE_ASYNCIO_REQUEST*)
CORE_AcquireIoRequest
(
    CORE_ASYNCIO_REQUEST_POOL *pool
);

/* @summary Return an I/O request object to the pool it was acquired from.
 * @param request The I/O request object to return.
 */
CORE_API(void)
CORE_ReturnIoRequest
(
    CORE_ASYNCIO_REQUEST   *request
);

/* @summary Calculate the amount of memory required to launch an I/O thread pool with the specified number of worker threads.
 * @param worker_count The number of worker threads in the pool.
 * @return The number of bytes of memory required to launch an I/O thread pool with the specified number of workers.
 */
CORE_API(size_t)
CORE_QueryAsyncIoThreadPoolMemorySize
(
    size_t worker_count
);

/* @summary Initialize and launch a pool of I/O worker threads.
 * @param pool The CORE_ASYNCIO_THREAD_POOL to initialize.
 * @param init Data used to configure the I/O thread pool.
 * @return Zero if the pool is initialized and launched successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_LaunchIoThreadPool
(
    CORE_ASYNCIO_THREAD_POOL      *pool, 
    CORE_ASYNCIO_THREAD_POOL_INIT *init
);

/* @summary Stop all threads in an I/O worker pool.
 * @param pool The CORE_ASYNCIO_THREAD_POOL to terminate.
 */
CORE_API(void)
CORE_TerminateIoThreadPool
(
    CORE_ASYNCIO_THREAD_POOL *pool
);

/* @summary Submit an asynchronous I/O request.
 * @param pool The CORE_ASYNCIO_THREAD_POOL that will execute the request.
 * @param request The I/O request to submit. Any data associated with the request must remain valid until the request completes.
 * @return Zero if the request is submitted successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_SubmitIoRequest
(
    CORE_ASYNCIO_THREAD_POOL *pool, 
    CORE_ASYNCIO_REQUEST  *request
);

#endif /* __CORE_ASYNCIO_H__ */

#ifdef CORE_ASYNCIO_IMPLEMENTATION

/* @summary Define the data passed to an I/O worker thread on startup.
 */
typedef struct _CORE__ASYNCIO_THREAD_INIT {
    struct _CORE_ASYNCIO_THREAD_POOL  *ThreadPool;                /* The I/O thread pool that owns the worker thread. */
    CORE_AsyncIoWorkerThreadInit_Func  ThreadInit;                /* The callback function to invoke to create any thread-local data. */
    HANDLE                             ReadySignal;               /* A manual-reset event to be signaled by the worker when it has successfully completed initialization and is ready to run. */
    HANDLE                             ErrorSignal;               /* A manual-reset event to be signaled by the worker before it terminates when it encounters a fatal error. */
    HANDLE                             TerminateSignal;           /* A manual-reset event to be signaled by the application when the worker should terminate. */
    HANDLE                             CompletionPort;            /* The I/O completion port to be monitored by the thread for incoming events. */
    uintptr_t                          PoolContext;               /* Opaque data associated with the I/O thread pool. */
} CORE__ASYNCIO_THREAD_INIT;

#endif /* CORE_ASYNCIO_IMPLEMENTATION */

