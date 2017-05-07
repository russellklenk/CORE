/*
 * CORE_asyncio.h: A single-file library for performing asynchronous disk I/O.
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

/* @summary Define the maximum amount of data, in bytes, that can be associated with an I/O request.
 */
#ifndef CORE_ASYNCIO_REQUEST_MAX_DATA
#define CORE_ASYNCIO_REQUEST_MAX_DATA     64
#endif

/* @summary Define the value used to represent an invalid file handle.
 */
#ifndef CORE_ASYNCIO_INVALID_FILE
#define CORE_ASYNCIO_INVALID_FILE         INVALID_HANDLE_VALUE
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
union  _CORE_ASYNCIO_HANDLE;
struct _CORE_ASYNCIO_RESULT;
struct _CORE_ASYNCIO_REQUEST;
struct _CORE_ASYNCIO_REQUEST_POOL;
struct _CORE_ASYNCIO_REQUEST_POOL_INIT;
struct _CORE_ASYNCIO_THREAD_POOL;
struct _CORE_ASYNCIO_THREAD_POOL_INIT;

/* @summary Define the signature for the callback function invoked when an I/O operation has completed.
 * The callback runs on the I/O thread pool, and should perform a minimal amount of non-blocking work.
 * @param request The request object defining the I/O operation.
 * @param result Data describing the result of the I/O operation.
 * @param success This value is non-zero if the request executed successfully.
 * @return A chained I/O request to execute immediately on the same I/O thread, or NULL.
 */
typedef struct _CORE_ASYNCIO_REQUEST* (*CORE_AsyncIoCompletion_Func)
(
    struct _CORE_ASYNCIO_REQUEST *request,
    struct _CORE_ASYNCIO_RESULT   *result,
    int                           success
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

/* @summary Define the data associated with a region of a file mapped into memory.
 */
typedef struct _CORE_FILE_REGION {
    uint8_t                           *MapPtr;                    /* The address returned by MapViewOfFile. */
    int64_t                            Offset;                    /* The offset of the data in this file data region from the start of the file. */
    int64_t                            DataSize;                  /* The number of bytes starting from MapPtr that are valid to access. */
} CORE_FILE_REGION;

/* @summary Define the data returned from a background I/O request through the completion callback.
 * Enough data is returned that it is possible to return a chained I/O request to be executed immediately.
 */
typedef struct _CORE_ASYNCIO_RESULT {
    uint64_t                           QueueDelay;                /* The time, in nanoseconds, between when the I/O request was submitted and when it started executing. */
    uint64_t                           ExecutionTime;             /* The amount of time, in nanoseconds, that it took to actually execute the I/O operation. */
    struct _CORE_ASYNCIO_THREAD_POOL  *IoThreadPool;              /* The I/O thread pool on which the request was executed. */
    struct _CORE_ASYNCIO_REQUEST_POOL *RequestPool;               /* The I/O request pool from which the request was allocated. */
    void                              *RequestData;               /* A pointer to the data associated with the operation. The type of data depends on the type of request. */
    uintptr_t                          PoolContext;               /* Opaque data associated with the I/O thread pool. */
    uintptr_t                          UserContext;               /* Opaque data associated with the request and supplied by the requestor on submission. */
    uint32_t                           ResultCode;                /* ERROR_SUCCESS or another operating system result code indicating the result of the operation. */
    uint32_t                           BytesTransferred;          /* The number of bytes transferred during the I/O operation. */
} CORE_ASYNCIO_RESULT;

/* @summary Define the data that must be specified to open a file.
 */
typedef struct _CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA {
    WCHAR                             *FilePath;                  /* Pointer to a caller-managed buffer specifying the path of the file to open. */
    int64_t                            FileSize;                  /* On input, if the file should be preallocated, this value specifies the file size in bytes. On output, this value specifies the file size in bytes. */
    uint32_t                           HintFlags;                 /* One or more _CORE_ASYNCIO_HINT_FLAGS specifying how the file will be accessed. */
    uint32_t                           Alignment;                 /* On input, set to zero. On output, this value is set to the required alignment for unbuffered I/O. */
} CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA;

/* @summary Define the data that must be specified to read from a file.
 */
typedef struct _CORE_ASYNCIO_FILE_READ_REQUEST_DATA {
    void                              *DataBuffer;                /* The caller managed data buffer to which data will be written. */
    size_t                             BufferOffset;              /* The byte offset at which to begin writing data to the buffer. */
    int64_t                            DataAmount;                /* The number of bytes to read from the file. */
    int64_t                            BaseOffset;                /* The byte offset of the start of the operation from the start of the physical file. */
    int64_t                            FileOffset;                /* The byte offset of the start of the operation from the start of the logical file. */
} CORE_ASYNCIO_FILE_READ_REQUEST_DATA;

/* @summary Define the data that must be specified to write data to a file.
 */
typedef struct _CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA {
    void                              *DataBuffer;                /* The caller managed data buffer from which data will be read. */
    size_t                             BufferOffset;              /* The byte offset at which to begin reading data from the buffer. */
    size_t                             DataAmount;                /* The number of bytes to write to the file. */
    int64_t                            BaseOffset;                /* The byte offset of the start of the operation from the start of the physical file. */
    int64_t                            FileOffset;                /* The byte offset of the start of the operation from the start of the logical file. */
} CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA;

/* @summary Define the data associated with an asynchronous I/O request.
 * Request objects are allocated from a pool that is typically thread-local.
 */
typedef struct _CORE_ASYNCIO_REQUEST {
    #define DATA_SIZE                  CORE_ASYNCIO_REQUEST_MAX_DATA
    struct _CORE_ASYNCIO_REQUEST      *NextRequest;               /* The next node in the request list, or NULL if this is the tail node. */
    struct _CORE_ASYNCIO_REQUEST_POOL *RequestPool;               /* The _CORE_ASYNCIO_REQUEST_POOL from which the request was allocated. */
    CORE_AsyncIoCompletion_Func        CompleteCallback;          /* The callback to invoke when the I/O operation completes. */
    CORE_ASYNCIO_HANDLE                Handle;                    /* The handle associated with the operation. */
    uintptr_t                          UserContext;               /* Opaque data to be passed through to the completion callback. */
    int32_t                            RequestType;               /* One of _CORE_ASYNCIO_REQUEST_TYPE specifying the type of the request. */
    int32_t                            RequestState;              /* One of _CORE_ASYNCIO_REQUEST_STATE specifying the state of the request. */
    int64_t                            SubmitTime;                /* The timestamp, in ticks, at which the I/O request was submitted. */
    int64_t                            LaunchTime;                /* The timestamp, in ticks, at which the I/O request was dequeued. */
    uint8_t                            RequestData[DATA_SIZE];    /* Storage for request type-specific data associated with the request. */
    OVERLAPPED                         Overlapped;                /* The OVERLAPPED instance associated used by the request. */
    #undef  DATA_SIZE
} CORE_ASYNCIO_REQUEST;

/* @summary Define the data associated with a pool of background I/O requests. 
 * Each thread that can submit I/O requests typically maintains its own request pool.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL {
    struct _CORE_ASYNCIO_REQUEST      *FreeList;                  /* Pointer to the first free request, or NULL if no requests are available. */
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

/* @summary Provide a default worker thread initialization function. The thread_context argument is set to zero.
 * @param pool The I/O thread pool that owns the worker thread.
 * @param pool_context Opaque data supplied when the I/O pool was created.
 * @param thread_id The operating system identifier of the worker thread.
 * @param thread_context On return, the function should update this value to point to any data to be associated with the thread.
 * @return Zero if initialization completes successfully, or -1 if initialization failed.
 */
CORE_API(int)
CORE_AsyncIoWorkerThreadInitDefault
(
    CORE_ASYNCIO_THREAD_POOL *pool, 
    uintptr_t         pool_context, 
    uint32_t             thread_id, 
    uintptr_t      *thread_context
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

/* @summary Allocate host memory with the correct size and alignment for an instance of a given type from a memory arena.
 * @param _arena The CORE__ASYNCIO_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__AsyncIoMemoryArenaAllocateType
#define CORE__AsyncIoMemoryArenaAllocateType(_arena, _type)                    \
    ((_type*) CORE__AsyncIoMemoryArenaAllocateHost((_arena), sizeof(_type), __alignof(_type)))
#endif

/* @summary Allocate memory with the correct size and alignment for an array of instance of a given type from a memory arena.
 * @param _arena The CORE__ASYNCIO_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the array.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__AsyncIoMemoryArenaAllocateArray
#define CORE__AsyncIoMemoryArenaAllocateArray(_arena, _type, _count)           \
    ((_type*) CORE__AsyncIoMemoryArenaAllocateHost((_arena), sizeof(_type) * (_count), __alignof(_type)))
#endif

/* @summary Define a special value posted to a thread when it should shutdown.
 */
#ifndef CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN
#define CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN    ~((ULONG_PTR)0)
#endif

/* @summary Define the data associated with an internal memory arena allocator.
 */
typedef struct _CORE__ASYNCIO_ARENA {
    uint8_t                           *BaseAddress;               /* The base address of the memory range. */
    size_t                             MemorySize;                /* The size of the memory block, in bytes. */
    size_t                             NextOffset;                /* The offset of the next available address. */
} CORE__ASYNCIO_ARENA;

/* @summary Define the type used to mark a location within a memory arena.
 */
typedef size_t CORE__ASYNCIO_ARENA_MARKER;

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

/* @summary Define the data returned from the completion of an I/O request.
 */
typedef struct _CORE__ASYNCIO_COMPLETION {
    DWORD                              ResultCode;                /* The operating system result code returned by the operation. */
    DWORD                              BytesTransferred;          /* The total number of bytes transferred during the operation. */
    DWORD                              WSAFlags;                  /* The output flags value if the operation was a socket or datagram read, otherwise zero.*/
    int                                Success;                   /* The overall success status of the operation, where non-zero indicates success and zero indicates failure. */
} CORE__ASYNCIO_COMPLETION;

/* @summary Initialize a memory arena allocator around an externally-managed memory block.
 * @param arena The memory arena allocator to initialize.
 * @param memory A pointer to the start of the memory block to sub-allocate from.
 * @param memory_size The size of the memory block, in bytes.
 */
static void
CORE__AsyncIoInitMemoryArena
(
    CORE__ASYNCIO_ARENA *arena, 
    void               *memory,
    size_t         memory_size
)
{
    arena->BaseAddress =(uint8_t*) memory;
    arena->MemorySize  = memory_size;
    arena->NextOffset  = 0;
}

/* @summary Sub-allocate memory from an arena.
 * @param arena The memory arena from which the memory will be allocated.
 * @param size The minimum number of bytes to allocate.
 * @param alignment The required alignment of the returned address, in bytes. This value must be zero, or a power-of-two.
 * @return A pointer to the start of a memory block of at least size bytes, or NULL.
 */
static void*
CORE__AsyncIoMemoryArenaAllocateHost
(
    CORE__ASYNCIO_ARENA *arena, 
    size_t                size, 
    size_t           alignment
)
{
    uintptr_t    base_address = (uintptr_t) arena->BaseAddress + arena->NextOffset;
    uintptr_t aligned_address =  CORE_AlignUp(base_address, alignment);
    size_t        align_bytes =  aligned_address - base_address;
    size_t        alloc_bytes =  size + align_bytes;
    size_t         new_offset =  arena->NextOffset + alloc_bytes;
    if (new_offset < arena->MemorySize)
    {   /* the arena can satisfy the allocation */
        arena->NextOffset = new_offset;
        return (void*) aligned_address;
    }
    else
    {   /* the arena cannot satisfy the allocation */
        return NULL;
    }
}

/* @summary Retrieve a marker that can be used to reset a memory arena back to a given point.
 * @param arena The memory arena allocator to query.
 * @return A marker value that can roll back the allocator to its state at the time of the call, invalidating all allocations made from that point forward.
 */
static CORE__ASYNCIO_ARENA_MARKER
CORE__AsyncIoMemoryArenaMark
(
    CORE__ASYNCIO_ARENA *arena
)
{
    return arena->NextOffset;
}

/* @summary Roll back a memory arena allocator to a given point in time.
 * @param arena The memory arena allocator to roll back.
 * @param marker A marker obtained by a prior call to the ArenaMark function, or 0 to invalidate all existing allocations made from the arena.
 */
static void
CORE__AsyncIoResetMemoryArenaToMarker
(
    CORE__ASYNCIO_ARENA        *arena,
    CORE__ASYNCIO_ARENA_MARKER marker
)
{   assert(marker <= arena->NextOffset);
    arena->NextOffset = marker;
}

/* @summary Invalidate all allocations made from a memory arena.
 * @param arena The memory arena allocator to roll back.
 */
static void
CORE__AsyncIoResetMemoryArena
(
    CORE__ASYNCIO_ARENA *arena
)
{
    arena->NextOffset = 0;
}

/* @summary Calculate the elapsed time between two marked points, in nanoseconds.
 * @param beg The timestamp representing the starting point of the time interval.
 * @param end The timestamp representing the ending point of the time interval.
 * @param freq The frequency, in ticks-per-second, of the system high-resolution clock.
 * @return The elapsed time between beg and end, in nanoseconds.
 */
static uint64_t
CORE__AsyncIoElapsedNanoseconds
(
    int64_t beg, 
    int64_t end, 
    int64_t freq
)
{   assert(beg <= end);
    return (1000000000ULL * (uint64_t)(end - beg)) / (uint64_t)freq;
}

/* @summary Retrieve the I/O request object corresponding to an OVERLAPPED instance.
 * @param overlapped The OVERLAPPED instance corresponding to a completed request.
 * @return The corresponding request object.
 */
static CORE_ASYNCIO_REQUEST*
CORE__AsyncIoRequestForOVERLAPPED
(
    OVERLAPPED *overlapped
)
{
    return ((CORE_ASYNCIO_REQUEST*)(((uint8_t*)overlapped) - offsetof(CORE_ASYNCIO_REQUEST, Overlapped)));
}

/* @summary Call the appropriate version of GetOverlappedResult for file or socket handles and retrieve the error code.
 * @param request The CORE_ASYNCIO_REQUEST that caused the worker thread to wake up.
 * @param overlapped The OVERLAPPED object that signaled the completion port.
 * @param transferred On return, this location is updated with the number of bytes transferred.
 * @param wait Specify TRUE to block the calling thread until the result is available.
 * @param flags For socket operations, this location is updated with flags that modified the socket operation.
 * @param error On return, the system error code is stored in this location.
 * @return Non-zero if the function succeeds, or zero if an error occurs.
 */
static BOOL
CORE__AsyncIoGetOverlappedResult
(
    CORE_ASYNCIO_REQUEST *request, 
    OVERLAPPED        *overlapped, 
    LPDWORD           transferred, 
    BOOL                     wait, 
    LPDWORD                 flags, 
    LPDWORD                 error
)
{
    if (request->RequestType == CORE_ASYNCIO_REQUEST_TYPE_READ_FILE  || 
        request->RequestType == CORE_ASYNCIO_REQUEST_TYPE_WRITE_FILE)
    {   /* we're working with a file handle - use GetOverlappedResult */
        BOOL r = GetOverlappedResult(request->Handle.File, overlapped, transferred, wait);
        *error = GetLastError();
        *flags = 0;
        return r;
    }
    assert(false && "CORE__AsyncIoGetOverlappedResult called for request with invalid type");
    SetLastError(ERROR_NOT_SUPPORTED);
    *error = ERROR_NOT_SUPPORTED;
    *flags = 0;
    return FALSE;
}

/* @summary Complete an I/O request and return it to the request pool.
 * @param req The I/O request that has completed.
 * @param pool The I/O thread pool that executed the request.
 * @param comp The I/O request completion information.
 * @param frequency The frequency of the high-resolution timer, in counts-per-second.
 * @return A pointer to the chained request to execute immediately, or NULL.
 */
static CORE_ASYNCIO_REQUEST*
CORE__AsyncIoCompleteRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE_ASYNCIO_THREAD_POOL *pool, 
    CORE__ASYNCIO_COMPLETION *comp, 
    LARGE_INTEGER        frequency
)
{
    CORE_ASYNCIO_REQUEST *chained = NULL;
    CORE_ASYNCIO_RESULT       res;
    LARGE_INTEGER          timest;

    /* retrieve the completion timestamp */
    QueryPerformanceCounter(&timest);
    /* transition the request to the completed state */
    req->RequestState    = CORE_ASYNCIO_REQUEST_STATE_COMPLETED;
    /* populate the result object and invoke the callback */
    res.QueueDelay       = CORE__AsyncIoElapsedNanoseconds(req->LaunchTime, req->SubmitTime, frequency.QuadPart);
    res.ExecutionTime    = CORE__AsyncIoElapsedNanoseconds(timest.QuadPart, req->LaunchTime, frequency.QuadPart);
    res.IoThreadPool     = pool;
    res.RequestPool      = req->RequestPool;
    res.RequestData      = req->RequestData;
    res.PoolContext      = pool->ContextData;
    res.UserContext      = req->UserContext;
    res.ResultCode       = comp->ResultCode;
    res.BytesTransferred = comp->BytesTransferred;
    if ((chained = req->CompleteCallback(req, &res, comp->Success)) != NULL)
    {    /* retrieve the submission timestamp for the chained request */
       QueryPerformanceCounter(&timest);
       chained->SubmitTime = timest.QuadPart;
       chained->LaunchTime = timest.QuadPart;
    }
    CORE_ReturnIoRequest(req);
    return chained;
}

/* @summary Execute a file open request. File open requests always complete synchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @param iocp The I/O completion port to associate with the new file handle. The completion port is managed by the asynchronous I/O system.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteOpenFileRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp, 
    HANDLE                    iocp
)
{
    CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA*) req->RequestData;
    HANDLE                                 fd = INVALID_HANDLE_VALUE;
    DWORD const           DEFAULT_SECTOR_SIZE = 4096;
    DWORD                              access = 0; /* dwDesiredAccess */
    DWORD                               share = 0; /* dwShareMode */
    DWORD                              create = 0; /* dwCreationDisposition */
    DWORD                               flags = 0; /* dwFlagsAndAttributes */
    uint32_t                            hints = data->HintFlags;

    /* build access, share, create and flags.
     * this code block is structured so that WRITE overrides/extends READ, etc. 
     * the order of the code here matters, so beware of that when changing it. */
    if (hints & CORE_ASYNCIO_HINT_FLAG_OVERWRITE)
    {   /* this implies write access */
        hints |= CORE_ASYNCIO_HINT_FLAG_WRITE;
    }
    if (hints & CORE_ASYNCIO_HINT_FLAG_READ)
    {
        access = GENERIC_READ;
        share  = FILE_SHARE_READ;
        create = OPEN_EXISTING;
        flags  = FILE_FLAG_OVERLAPPED;
    }
    if (hints & CORE_ASYNCIO_HINT_FLAG_WRITE)
    {
        access |= GENERIC_WRITE;
        flags   = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
        if (hints & CORE_ASYNCIO_HINT_FLAG_OVERWRITE)
        {   /* the file is always re-created */
            create = CREATE_ALWAYS;
        }
        else
        {   /* the file is created if it doesn't already exist */
            create = OPEN_ALWAYS;
        }
        if (hints & CORE_ASYNCIO_HINT_FLAG_TEMPORARY)
        {   /* temporary files are deleted on close; the cache manager tries to prevent writes to disk */
            flags |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
            share |= FILE_SHARE_DELETE;
        }
    }
    if (hints & CORE_ASYNCIO_HINT_FLAG_SEQUENTIAL)
    {   /* tell the cache manager to optimize for sequential access */
        flags |= FILE_FLAG_SEQUENTIAL_SCAN;
    }
    else
    {   /* assume that the file will be accessed randomly */
        flags |= FILE_FLAG_RANDOM_ACCESS;
    }
    if (hints & CORE_ASYNCIO_HINT_FLAG_UNCACHED)
    {   /* use unbuffered I/O - reads must be performed in sector-size multiples.
           user buffers must start on an address that is a sector-size multiple. */
        flags |= FILE_FLAG_NO_BUFFERING;
    }
    if (hints & CORE_ASYNCIO_HINT_FLAG_WRITE_THROUGH)
    {   /* writes are immediately flushed to disk */
        flags |= FILE_FLAG_WRITE_THROUGH;
    }

    if ((fd = CreateFile(data->FilePath, access, share, NULL, create, flags, NULL)) == INVALID_HANDLE_VALUE)
    {   /* the file could not be opened or created */
        req->Handle.File       = CORE_ASYNCIO_INVALID_FILE;
        comp->ResultCode       = GetLastError();
        comp->BytesTransferred = 0;
        comp->WSAFlags         = 0;
        comp->Success          = 0;
        return TRUE; /* completed synchronously */
    }

    if (CreateIoCompletionPort(fd, iocp, 0, 0) != iocp)
    {   /* the file handle could not be associated with the completion port */
        req->Handle.File       = CORE_ASYNCIO_INVALID_FILE;
        comp->ResultCode       = GetLastError();
        comp->BytesTransferred = 0;
        comp->WSAFlags         = 0;
        comp->Success          = 0;
        CloseHandle(fd);
        return TRUE; /* completed synchronously */
    }
    /* immediately complete requests that execute synchronously; don't notify the completion port */
    SetFileCompletionNotificationModes(fd, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    if (hints & CORE_ASYNCIO_HINT_FLAG_PREALLOCATE)
    {   /* preallocate storage for the data, which can significantly improve write performance
           BUT THE FILE MUST BE WRITTEN SEQUENTIALLY or it will be very slow for large files 
           because the unwritten space must be zero-filled for security purposes. 
           failures are non-fatal since this is an optimization. */
        LARGE_INTEGER   new_ptr; new_ptr.QuadPart = data->FileSize;
        if (SetFilePointerEx(fd, new_ptr, NULL, FILE_BEGIN))
        {   /* set the end-of-file marker to the new location */
            if (SetEndOfFile(fd))
            {   /* move the file pointer back to the start of the file */
                new_ptr.QuadPart = 0;
                SetFilePointerEx(fd, new_ptr, NULL, FILE_BEGIN);
            }
        }
    }
    {   /* retrieve the current size of the file, in bytes */
        FILE_STANDARD_INFO fsi;
        if (GetFileInformationByHandleEx(fd, FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO)))
        {   /* save the file size for the user */
            data->FileSize = fsi.EndOfFile.QuadPart;
        }
    }
    {   /* retrieve the required alignment for unbuffered reads and writes */
#if WINVER >= 0x0602 || _WIN32_WINNT >= 0x0602
        FILE_ALIGNMENT_INFO fai;
        if (GetFileInformationByHandleEx(fd, FileAlignmentInfo /* 0x11 */, &fai, sizeof(FILE_ALIGNMENT_INFO)))
        {   /* save the required alignment for the user */
            data->Alignment =(uint32_t)fai.AlignmentRequirement;
        }
        else
        {   /* retrieving the information failed - use the default large sector size */
            data->Alignment = DEFAULT_SECTOR_SIZE;
        }
#else   /* pre-Windows 8 - use the default large sector size */
        data->Alignment = DEFAULT_SECTOR_SIZE;
#endif
    }
    /* the request completes successfully */
    req->Handle.File       = fd;
    comp->ResultCode       = GetLastError();
    comp->BytesTransferred = 0;
    comp->WSAFlags         = 0;
    comp->Success          = 1;
    return TRUE; /* completed synchronously */
}

/* @summary Execute a file read request. File read requests may complete synchronously or asynchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteReadFileRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{
    CORE_ASYNCIO_FILE_READ_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_READ_REQUEST_DATA*) req->RequestData;
    int64_t                        abs_offset =  data->BaseOffset + data->FileOffset;
    DWORD                         transferred =  0;
    DWORD                              amount = (DWORD   ) data->DataAmount;
    uint8_t                             *dest = (uint8_t*) data->DataBuffer + data->BufferOffset;

    assert(data->DataAmount <= 0xFFFFFFFFUL);
    assert(req->Handle.File != CORE_ASYNCIO_INVALID_FILE);

    req->Overlapped.Internal     = 0;
    req->Overlapped.InternalHigh = 0;
    req->Overlapped.Offset       =(DWORD) (abs_offset        & 0xFFFFFFFFUL);
    req->Overlapped.OffsetHigh   =(DWORD)((abs_offset >> 32) & 0xFFFFFFFFUL);
    if (ReadFile(req->Handle.File, dest, amount, &transferred, &req->Overlapped))
    {   /* the read operation completed synchronously - likely the data was in-cache */
        comp->ResultCode         = GetLastError();
        comp->BytesTransferred   = transferred;
        comp->WSAFlags           = 0;
        comp->Success            = 1;
        return TRUE; /* completed synchronously */
    }
    /* the read could have failed, or it could be completing asynchronously */
    switch ((comp->ResultCode = GetLastError()))
    {
        case ERROR_IO_PENDING:
            { /* the request will complete asynchronously */
              comp->BytesTransferred = 0;
              comp->WSAFlags         = 0;
              comp->Success          = 1;
            } return FALSE; /* completed asynchronously */
        case ERROR_HANDLE_EOF:
            { /* reached end-of-file */
              comp->BytesTransferred = transferred;
              comp->WSAFlags         = 0;
              comp->Success          = 1;
            } return TRUE; /* completed synchronously */
        default:
            { /* an actual error occurred */
              comp->BytesTransferred = 0;
              comp->WSAFlags         = 0;
              comp->Success          = 0;
            } return TRUE; /* completed synchronously */
    }
}

/* @summary Execute a file write request. File write requests may complete synchronously or asynchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteWriteFileRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{
    CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA*) req->RequestData;
    int64_t                         abs_offset =  data->BaseOffset + data->FileOffset;
    DWORD                          transferred =  0;
    DWORD                               amount = (DWORD   ) data->DataAmount;
    uint8_t                            *source = (uint8_t*) data->DataBuffer + data->BufferOffset;

    assert(data->DataAmount <= 0xFFFFFFFFUL);
    assert(req->Handle.File != CORE_ASYNCIO_INVALID_FILE);

    req->Overlapped.Internal     = 0;
    req->Overlapped.InternalHigh = 0;
    req->Overlapped.Offset       =(DWORD) (abs_offset        & 0xFFFFFFFFUL);
    req->Overlapped.OffsetHigh   =(DWORD)((abs_offset >> 32) & 0xFFFFFFFFUL);
    if (WriteFile(req->Handle.File, source, amount, &transferred, &req->Overlapped))
    {   /* the write operation completed synchronously */
        comp->ResultCode         = GetLastError();
        comp->BytesTransferred   = transferred;
        comp->WSAFlags           = 0;
        comp->Success            = 1;
        return TRUE; /* completed synchronously */
    }
    /* the write could have failed, or it could be completing asynchronously */
    switch ((comp->ResultCode = GetLastError()))
    {
        case ERROR_IO_PENDING:
            { /* the request will complete asynchronously */
              comp->BytesTransferred = 0;
              comp->WSAFlags         = 0;
              comp->Success          = 1;
            } return FALSE; /* completed asynchronously */
        default:
            { /* an actual error occurred */
              comp->BytesTransferred = 0;
              comp->WSAFlags         = 0;
              comp->Success          = 0;
            } return TRUE; /* completed synchronously */
    }
}

/* @summary Execute a file flush request. File flush requests always complete synchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteFlushFileRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{   assert(req->Handle.File != CORE_ASYNCIO_INVALID_FILE);
    if (FlushFileBuffers(req->Handle.File))
    {   /* the flush operation was successful */
        comp->Success = 1;
    }
    else
    {   /* the flush operation failed */
        comp->Success = 0;
    }
    comp->ResultCode       = GetLastError();
    comp->BytesTransferred = 0;
    comp->WSAFlags         = 0;
    return TRUE; /* completed synchronously */
}

/* @summary Execute a file close request. File close requests always complete synchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteCloseFileRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{   assert(req->Handle.File != CORE_ASYNCIO_INVALID_FILE);
    if (CloseHandle(req->Handle.File))
    {   /* the close operation succeeded */
        comp->Success = 1;
    }
    else
    {   /* the close operation failed */
        comp->Success = 0;
    }
    comp->ResultCode       = GetLastError();
    comp->BytesTransferred = 0;
    comp->WSAFlags         = 0;
    return TRUE; /* completed synchronously */
}

/* @summary Execute an I/O request. The request may complete synchronously or asynchronously.
 * @param req The I/O request to execute.
 * @param comp On return, information about the completed I/O operation is returned here.
 * @param iocp The I/O completion port to associate with the new file handle. The completion port is managed by the asynchronous I/O system.
 * @return Non-zero if the request has executed synchronously and completed, or zero if the request is executing asynchronously.
 */
static BOOL
CORE__AsyncIoExecuteRequest
(
    CORE_ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_COMPLETION *comp,
    HANDLE                    iocp
)
{
    BOOL completed_synchronously = TRUE;
    switch (req->RequestType)
    {
        case CORE_ASYNCIO_REQUEST_TYPE_NOOP:
            { /* no-op just passes through data, can be used for timing, fence, etc. */
              comp->ResultCode        = ERROR_SUCCESS;
              comp->BytesTransferred  = 0;
              comp->WSAFlags          = 0;
              comp->Success           = 1;
              completed_synchronously = TRUE;
            } break;
        case CORE_ASYNCIO_REQUEST_TYPE_OPEN_FILE:
            { completed_synchronously = CORE__AsyncIoExecuteOpenFileRequest(req, comp, iocp);
            } break;
        case CORE_ASYNCIO_REQUEST_TYPE_READ_FILE:
            { completed_synchronously = CORE__AsyncIoExecuteReadFileRequest(req, comp);
            } break;
        case CORE_ASYNCIO_REQUEST_TYPE_WRITE_FILE:
            { completed_synchronously = CORE__AsyncIoExecuteWriteFileRequest(req, comp);
            } break;
        case CORE_ASYNCIO_REQUEST_TYPE_FLUSH_FILE:
            { completed_synchronously = CORE__AsyncIoExecuteFlushFileRequest(req, comp);
            } break;
        case CORE_ASYNCIO_REQUEST_TYPE_CLOSE_FILE:
            { completed_synchronously = CORE__AsyncIoExecuteCloseFileRequest(req, comp);
            } break;
        default:
            { /* unknown request type - treat as a no-op */
              comp->ResultCode        = ERROR_SUCCESS;
              comp->BytesTransferred  = 0;
              comp->WSAFlags          = 0;
              comp->Success           = 1;
              completed_synchronously = TRUE;
            } break;
    }
    return completed_synchronously;
}

/* @summary Implements the entry point and main body of a worker thread used for executing asynchronous I/O operations.
 * @param argp A pointer to a CORE__ASYNCIO_THREAD_INIT instance.
 * @return Zero if the thread terminates normally, or non-zero if the thread terminates with an error.
 */
static unsigned int __stdcall
CORE__AsyncIoThreadMain
(
    void *argp
)
{
    CORE__ASYNCIO_THREAD_INIT *init =(CORE__ASYNCIO_THREAD_INIT*) argp;
    CORE_ASYNCIO_THREAD_POOL  *pool = init->ThreadPool;
    CORE_ASYNCIO_REQUEST       *req = NULL;
    OVERLAPPED                  *ov = NULL;
    uintptr_t        thread_context = 0;
    uintptr_t          pool_context = init->PoolContext;
    uintptr_t                   key = 0;
    HANDLE                     term = init->TerminateSignal;
    HANDLE                     iocp = init->CompletionPort;
    HANDLE                    ready = init->ReadySignal;
    HANDLE                    error = init->ErrorSignal;
    unsigned int          exit_code = 0;
    DWORD                   term_rc = 0;
    DWORD                    nbytes = 0;
    LARGE_INTEGER         timestamp;
    LARGE_INTEGER         frequency;

    /* query the HPC frequency to convert ticks to seconds */
    QueryPerformanceFrequency(&frequency);

    /* allow the startup routine to run and initialize the thread context */
    if (init->ThreadInit(pool, pool_context, GetCurrentThreadId(), &thread_context) < 0)
    {   /* the initialization callback returned an error */
        SetEvent(error);
        return 1;
    }

    /* past this point, it is not valid to access any fields of init */
    SetEvent(ready); init = NULL;
    __try
    {   /* enter the main thread loop, waiting for events to become available on the completion port */
        for ( ; ; )
        {   /* check the termination signal prior to possibly entering a wait state */
            if ((term_rc = WaitForSingleObject(term, 0)) != WAIT_TIMEOUT)
            {   /* either thread termination was signaled, or an error occurred */
                break; /* break out of for ( ; ; ) */
            }
            /* wait for events on the I/O completion port */
            if (GetQueuedCompletionStatus(iocp, &nbytes, &key, &ov, INFINITE))
            {   /* check the termination signal prior to processing the request */
                if ((term_rc = WaitForSingleObject(term, 0)) != WAIT_TIMEOUT)
                {   /* either thread termination was signaled, or an error occurred */
                    break; /* break out of for ( ; ; ) */
                }
                if (key == CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN)
                {   /* thread termination was signaled */
                    break; /* break out of for ( ; ; ) */
                }
                if ((req = CORE__AsyncIoRequestForOVERLAPPED(ov)) != NULL)
                {
                    do
                    {   /* update the request object based on its current state */
                        switch (req->RequestState)
                        {
                            case CORE_ASYNCIO_REQUEST_STATE_CHAINED:
                            case CORE_ASYNCIO_REQUEST_STATE_SUBMITTED:
                                {
                                  CORE__ASYNCIO_COMPLETION comp;
                                  /* retrieve the request launch timestamp */
                                  QueryPerformanceCounter(&timestamp);
                                  /* transition the request to the launched state */
                                  req->RequestState = CORE_ASYNCIO_REQUEST_STATE_LAUNCHED;
                                  req->LaunchTime   = timestamp.QuadPart;
                                  if (CORE__AsyncIoExecuteRequest(req, &comp, iocp))
                                  {   /* the request completed synchronously, so finish and chain */
                                      req = CORE__AsyncIoCompleteRequest(req, pool, &comp, frequency);
                                  }
                                  else
                                  {   /* the request hasn't completed, so there is no chained request */
                                      req = NULL;
                                  }
                                } break;
                            case CORE_ASYNCIO_REQUEST_STATE_LAUNCHED:
                                { /* an asynchronous I/O read or write operation has completed */
                                  CORE__ASYNCIO_COMPLETION comp;
                                  DWORD        rc = ERROR_SUCCESS;
                                  DWORD     flags = 0;
                                  int     success = 1;
                                  if (!CORE__AsyncIoGetOverlappedResult(req, ov, &nbytes, FALSE, &flags, &rc))
                                  {   /* determine whether the operation actually failed */
                                      if (rc != ERROR_HANDLE_EOF)
                                          success = 0;
                                  }
                                  comp.ResultCode       = rc;
                                  comp.BytesTransferred = nbytes;
                                  comp.WSAFlags         = flags;
                                  comp.Success          = success;
                                  req = CORE__AsyncIoCompleteRequest(req, pool, &comp, frequency);
                                } break;
                            case CORE_ASYNCIO_REQUEST_STATE_COMPLETED:
                                { /* this should not happen - received an already-completed request */
                                  assert(req->RequestState != CORE_ASYNCIO_REQUEST_STATE_COMPLETED);
                                } break;
                            default:
                                { /* received a request object with unknown state - drop it */
                                  assert(false && "I/O request object has unknown state value");
                                } break;
                        }
                    } while (req != NULL);
                }
            }
        } /* end for ( ; ; ) */
    }
    __finally
    {   /* the worker thread is terminating - clean up any local resources */
        return exit_code;
    }
}

CORE_API(size_t)
CORE_QueryAsyncIoRequestPoolMemorySize
(
    size_t max_requests
)
{
    size_t required_size = 0;
    size_t     alignment = CORE_AlignOf(CORE_ASYNCIO_REQUEST);
    required_size += sizeof(CORE_ASYNCIO_REQUEST) + (alignment - 1); /* padding for alignment */
    required_size += sizeof(CORE_ASYNCIO_REQUEST) *  max_requests;   /* storage for requests  */
    return required_size;
}

CORE_API(int)
CORE_InitIoRequestPool
(
    CORE_ASYNCIO_REQUEST_POOL      *pool, 
    CORE_ASYNCIO_REQUEST_POOL_INIT *init
)
{
    size_t required_size = 0;
    uint32_t        i, n;

    if (init->PoolCapacity == 0)
    {   /* the caller must specify a non-zero capacity */
        assert(init->PoolCapacity > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->PoolMemory == NULL || init->PoolMemorySize == 0)
    {   /* the caller must supply the memory to use for the pool */
        assert(init->PoolMemory != NULL);
        assert(init->PoolMemorySize > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    required_size = CORE_QueryAsyncIoRequestPoolMemorySize(init->PoolCapacity);
    if (init->PoolMemorySize < required_size)
    {   /* the caller must supply sufficient memory for the pool */
        assert(init->PoolMemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* initialize the fields of the pool object */
    pool->FreeList       = NULL;
    pool->RequestPool    =(CORE_ASYNCIO_REQUEST*) CORE_AlignFor(init->PoolMemory, CORE_ASYNCIO_REQUEST);
    pool->PoolMemory     = init->PoolMemory;
    pool->PoolMemorySize = init->PoolMemorySize;
    pool->PoolCapacity   = init->PoolCapacity;
    pool->OwningThread   = init->OwningThread;
    InitializeCriticalSectionAndSpinCount(&pool->ListLock, 0x1000);
    /* initialize the fields of all of the request objects in the pool */
    ZeroMemory(init->PoolMemory, (size_t) init->PoolMemorySize);
    /* initialize the pool free list */
    for (i = 0, n = init->PoolCapacity; i < n; ++i)
    {
        CORE_ASYNCIO_REQUEST *req = &pool->RequestPool[i];
        req->NextRequest = pool->FreeList;
        req->RequestPool = pool;
        pool->FreeList   = req;
    }
    return 0;
}

CORE_API(CORE_ASYNCIO_REQUEST*)
CORE_AcquireIoRequest
(
    CORE_ASYNCIO_REQUEST_POOL *pool
)
{
    CORE_ASYNCIO_REQUEST *req = NULL;
    EnterCriticalSection(&pool->ListLock);
    {   /* return the node at the head of the free list */
        if ((req = pool->FreeList) != NULL)
        {   /* a node is available, pop from the free list */
            pool->FreeList   = req->NextRequest;
            req->NextRequest = NULL;
        }
    }
    LeaveCriticalSection(&pool->ListLock);
    return req;
}

CORE_API(void)
CORE_ReturnIoRequest
(
    CORE_ASYNCIO_REQUEST   *request
)
{   assert(request != NULL);
    assert(request->RequestPool != NULL);
    EnterCriticalSection(&request->RequestPool->ListLock);
    {   /* return the request to the front of the free list */
        request->NextRequest = request->RequestPool->FreeList;
        request->RequestPool->FreeList = request;
    }
    LeaveCriticalSection(&request->RequestPool->ListLock);
}

CORE_API(int)
CORE_AsyncIoWorkerThreadInitDefault
(
    CORE_ASYNCIO_THREAD_POOL *pool, 
    uintptr_t         pool_context, 
    uint32_t             thread_id, 
    uintptr_t      *thread_context
)
{
    UNREFERENCED_PARAMETER(pool);
    UNREFERENCED_PARAMETER(pool_context);
    UNREFERENCED_PARAMETER(thread_id);
    *thread_context = 0;
    return 0;
}

CORE_API(size_t)
CORE_QueryAsyncIoThreadPoolMemorySize
(
    size_t worker_count
)
{
    size_t required_size = 0;
    size_t     alignment = CORE_AlignOf(HANDLE);
    required_size += sizeof(HANDLE      ) + (alignment - 1); /* alignment padding */
    required_size += sizeof(unsigned int) *  worker_count;   /* OSThreadIds */
    required_size += sizeof(HANDLE      ) *  worker_count;   /* OSThreadHandle */
    required_size += sizeof(HANDLE      ) *  worker_count;   /* WorkerReady */
    required_size += sizeof(HANDLE      ) *  worker_count;   /* WorkerError */
    return required_size;
}

CORE_API(int)
CORE_LaunchIoThreadPool
(
    CORE_ASYNCIO_THREAD_POOL      *pool, 
    CORE_ASYNCIO_THREAD_POOL_INIT *init
)
{
    CORE__ASYNCIO_ARENA arena;
    size_t      required_size = 0;
    HANDLE               iocp = NULL;
    HANDLE               term = NULL;
    int              shutdown = 0;
    uint32_t             i, n;

    if (init->WorkerCount == 0)
    {   /* the thread pool must have at least one worker */
        assert(init->WorkerCount > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->PoolMemory == NULL || init->PoolMemorySize == 0)
    {   /* the caller must supply memory for the pool data */
        assert(init->PoolMemory != NULL);
        assert(init->PoolMemorySize > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    required_size = CORE_QueryAsyncIoThreadPoolMemorySize(init->WorkerCount);
    if (init->PoolMemorySize < required_size)
    {   /* the caller must supply enough memory for the pool */
        assert(init->PoolMemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->ThreadInitFunc == NULL)
    {   /* use the default no-op thread initialization function */
        init->ThreadInitFunc = CORE_AsyncIoWorkerThreadInitDefault;
    }
    if ((term = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {   /* failed to create the manual-reset event to terminate the worker threads */
        goto cleanup_and_fail;
    }
    if ((iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, init->WorkerCount+1)) == NULL)
    {   /* failed to create the I/O completion port for submitting requests and notifications */
        goto cleanup_and_fail;
    }

    /* zero everything out, assign memory */
    ZeroMemory(pool, sizeof(CORE_ASYNCIO_THREAD_POOL));
    ZeroMemory(init->PoolMemory, init->PoolMemorySize);
    CORE__AsyncIoInitMemoryArena(&arena, init->PoolMemory, init->PoolMemorySize);
    pool->OSThreadIds      = CORE__AsyncIoMemoryArenaAllocateArray(&arena, unsigned int, init->WorkerCount);
    pool->OSThreadHandle   = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    pool->WorkerReady      = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    pool->WorkerError      = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    if (pool->OSThreadIds == NULL || pool->OSThreadHandle == NULL || pool->WorkerReady == NULL || pool->WorkerError == NULL)
    {   /* insufficient memory - implies an error in CORE_QueryAsyncIoThreadPoolMemorySize */
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup_and_fail;
    }
    pool->CompletionPort   = iocp;
    pool->TerminateSignal  = term;
    pool->ContextData      = init->PoolContext;
    pool->ActiveThreads    = 0;
    pool->WorkerCount      = init->WorkerCount;
    pool->PoolMemory       = init->PoolMemory;
    pool->PoolMemorySize   = init->PoolMemorySize;

    /* launch all of the threads and have them initialize */
    for (i = 0, n = init->WorkerCount; i < n; ++i)
    {
        HANDLE                    whand = NULL;
        HANDLE                   wready = NULL;
        HANDLE                   werror = NULL;
        unsigned int          thread_id = 0;
        DWORD const        THREAD_READY = 0;
        DWORD const        THREAD_ERROR = 1;
        DWORD const          WAIT_COUNT = 2;
        DWORD                   wait_rc = 0;
        HANDLE                  wset[2];
        CORE__ASYNCIO_THREAD_INIT winit;

        if ((wready = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
        {
            goto cleanup_and_fail;
        }
        if ((werror = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
        {
            CloseHandle(wready);
            goto cleanup_and_fail;
        }

        winit.ThreadPool      = pool;
        winit.ThreadInit      = init->ThreadInitFunc;
        winit.ReadySignal     = wready;
        winit.ErrorSignal     = werror;
        winit.TerminateSignal = term;
        winit.CompletionPort  = iocp;
        winit.PoolContext     = init->PoolContext;
        if ((whand = (HANDLE)_beginthreadex(NULL, 64 * 1024, CORE__AsyncIoThreadMain, &winit, 0, &thread_id)) == NULL)
        {
            CloseHandle(werror);
            CloseHandle(wready);
            goto cleanup_and_fail;
        }
        pool->OSThreadIds   [pool->ActiveThreads] = thread_id;
        pool->OSThreadHandle[pool->ActiveThreads] = whand;
        pool->WorkerReady   [pool->ActiveThreads] = wready;
        pool->WorkerError   [pool->ActiveThreads] = werror;
        pool->ActiveThreads++;
        shutdown = 1;

        /* wait for the thread to become ready, or fail to initialize */
        wset[THREAD_READY] = wready;
        wset[THREAD_ERROR] = werror;
        if ((wait_rc = WaitForMultipleObjects(WAIT_COUNT, wset, FALSE, INFINITE)) != (WAIT_OBJECT_0+THREAD_READY))
        {   /* either thread initialization failed, or the wait failed */
            goto cleanup_and_fail;
        }
    }

    return 0;

cleanup_and_fail:
    if (shutdown > 0)
    {
        SetEvent(term);
        for (i = 0, n = pool->ActiveThreads; i < n; ++i)
        {
            PostQueuedCompletionStatus(iocp, 0, CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN, NULL);
        }
        WaitForMultipleObjects(pool->ActiveThreads, pool->OSThreadHandle, TRUE, INFINITE);
        for (i = 0, n = pool->ActiveThreads; i < n; ++i)
        {
            CloseHandle(pool->WorkerError[i]);
            CloseHandle(pool->WorkerReady[i]);
            CloseHandle(pool->OSThreadHandle[i]);
        }
    }
    if (iocp != NULL) CloseHandle(iocp);
    if (term != NULL) CloseHandle(term);
    return -1;
}

CORE_API(void)
CORE_TerminateIoThreadPool
(
    CORE_ASYNCIO_THREAD_POOL *pool
)
{
    uint32_t i, n;

    if (pool->ActiveThreads > 0)
    {   /* indicate that all threads should terminate */
        SetEvent(pool->TerminateSignal);
        /* wake each thread so that it notices the termination signal */
        for (i = 0, n = pool->ActiveThreads; i < n; ++i)
        {
            PostQueuedCompletionStatus(pool->CompletionPort, 0, CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN, NULL);
        }
        /* wait for all of the threads to terminate */
        WaitForMultipleObjects(pool->ActiveThreads, pool->OSThreadHandle, TRUE, INFINITE);
        /* close all per-thread handles */
        for (i = 0, n = pool->ActiveThreads; i < n; ++i)
        {
            CloseHandle(pool->WorkerError[i]);
            CloseHandle(pool->WorkerReady[i]);
            CloseHandle(pool->OSThreadHandle[i]);
        }
        pool->ActiveThreads = 0;
    }
    if (pool->CompletionPort != NULL)
    {
        CloseHandle(pool->CompletionPort);
        pool->CompletionPort = NULL;
    }
    if (pool->TerminateSignal != NULL)
    {
        CloseHandle(pool->TerminateSignal);
        pool->TerminateSignal = NULL;
    }
}

CORE_API(int)
CORE_SubmitIoRequest
(
    CORE_ASYNCIO_THREAD_POOL *pool, 
    CORE_ASYNCIO_REQUEST  *request
)
{
    LARGE_INTEGER timestamp;

    assert(request != NULL);
    assert(request->CompleteCallback != NULL);

    QueryPerformanceCounter(&timestamp);
    request->RequestState = CORE_ASYNCIO_REQUEST_STATE_SUBMITTED;
    request->SubmitTime   = timestamp.QuadPart;
    request->LaunchTime   = timestamp.QuadPart;
    if (!PostQueuedCompletionStatus(pool->CompletionPort, 0, 0, &request->Overlapped))
    {   /* submission of the request failed */
        return -1;
    }
    return 0;
}

#endif /* CORE_ASYNCIO_IMPLEMENTATION */

