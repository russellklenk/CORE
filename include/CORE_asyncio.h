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
struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE;
struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE_INIT;
struct _CORE_ASYNCIO_WORKER_POOL;
struct _CORE_ASYNCIO_WORKER_POOL_INIT;

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
    struct _CORE_ASYNCIO_WORKER_POOL *pool, 
    uintptr_t                 pool_context,
    uint32_t                     thread_id,
    uintptr_t              *thread_context
);

/* @summary Define a union for storing the different types of handles the async I/O system can work with.
 */
typedef union _CORE_ASYNCIO_HANDLE {
    HANDLE                                     File;               /* The Win32 file handle. */
} CORE_ASYNCIO_HANDLE;

/* @summary Define the data that must be specified to open a file.
 */
typedef struct _CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA {
    WCHAR                                     *FilePath;           /* Pointer to a caller-managed buffer specifying the path of the file to open. */
    int64_t                                    FileSize;           /* On input, if the file should be preallocated, this value specifies the file size in bytes. On output, this value specifies the file size in bytes. */
    uint32_t                                   HintFlags;          /* One or more _CORE_ASYNCIO_HINT_FLAGS specifying how the file will be accessed. */
    uint32_t                                   Alignment;          /* On input, set to zero. On output, this value is set to the required alignment for unbuffered I/O. */
} CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA;

/* @summary Define the data that must be specified to read from a file.
 */
typedef struct _CORE_ASYNCIO_FILE_READ_REQUEST_DATA {
    void                                      *DataBuffer;         /* The caller managed data buffer to which data will be written. */
    size_t                                     BufferOffset;       /* The byte offset at which to begin writing data to the buffer. */
    int64_t                                    DataAmount;         /* The number of bytes to read from the file. */
    int64_t                                    BaseOffset;         /* The byte offset of the start of the operation from the start of the physical file. */
    int64_t                                    FileOffset;         /* The byte offset of the start of the operation from the start of the logical file. */
} CORE_ASYNCIO_FILE_READ_REQUEST_DATA;

/* @summary Define the data that must be specified to write data to a file.
 */
typedef struct _CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA {
    void                                      *DataBuffer;         /* The caller managed data buffer from which data will be read. */
    size_t                                     BufferOffset;       /* The byte offset at which to begin reading data from the buffer. */
    size_t                                     DataAmount;         /* The number of bytes to write to the file. */
    int64_t                                    BaseOffset;         /* The byte offset of the start of the operation from the start of the physical file. */
    int64_t                                    FileOffset;         /* The byte offset of the start of the operation from the start of the logical file. */
} CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA;

/* @summary Define a union representing all of the possible types of asynchronous I/O request data.
 */
typedef union _CORE_ASYNCIO_REQUEST_DATA {
    #define DATA_SIZE CORE_ASYNCIO_REQUEST_MAX_DATA
    CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA        FileOpen;           /* Data associated with a file open request. */
    CORE_ASYNCIO_FILE_READ_REQUEST_DATA        FileRead;           /* Data associated with a file read request. */
    CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA       FileWrite;          /* Data associated with a file write request. */
    uint8_t                                    Data[DATA_SIZE];    /* Data associated with the request. */
    #undef  DATA_SIZE
} CORE_ASYNCIO_REQUEST_DATA;

/* @summary Define the data used to create an asynchronous I/O request.
 */
typedef struct _CORE_ASYNCIO_REQUEST_INIT {
    CORE_AsyncIoCompletion_Func                RequestComplete;    /* The callback function to invoke on the worker thread when the I/O operation has completed. */
    CORE_ASYNCIO_HANDLE                        RequestHandle;      /* The handle associated with the request. Not all request types have an associated handle. */
    uintptr_t                                  RequestContext;     /* Opaque data to be passed through to the completion callback when the I/O operation has completed. */
    uint32_t                                   RequestType;        /* One of _CORE_ASYNCIO_REQUEST_TYPE specifying the type of request. */
    uint32_t                                   RequestDataSize;    /* The size of the data associated with the request. */
    CORE_ASYNCIO_REQUEST_DATA                  RequestData;        /* Additional type-specific data associated with the request. */
} CORE_ASYNCIO_REQUEST_INIT;

/* @summary Define the data returned from a background I/O request through the completion callback.
 * Enough data is returned that it is possible to return a chained I/O request to be executed immediately.
 */
typedef struct _CORE_ASYNCIO_RESULT {
    struct _CORE_ASYNCIO_REQUEST_POOL         *RequestPool;        /* The I/O request pool from which the request was allocated. */
    struct _CORE_ASYNCIO_WORKER_POOL          *WorkerPool;         /* The I/O thread pool on which the request was executed. */
    CORE_ASYNCIO_HANDLE                        RequestHandle;      /* The handle associated with the request. */
    void                                      *RequestData;        /* A pointer to the data associated with the operation. The type of data depends on the type of request. */
    uintptr_t                                  RequestContext;     /* Opaque data associated with the request and supplied by the requestor on submission. */
    uint32_t                                   ResultCode;         /* ERROR_SUCCESS or another operating system result code indicating the result of the operation. */
    uint32_t                                   BytesTransferred;   /* The number of bytes transferred during the I/O operation. */
    uint64_t                                   ExecutionTime;      /* The amount of time, in nanoseconds, that it took to actually execute the I/O operation. */
    uint64_t                                   QueueDelay;         /* The time, in nanoseconds, between when the I/O request was submitted and when it started executing. */
} CORE_ASYNCIO_RESULT;

/* @summary Define the data used to configure an I/O request pool, which allows asynchronous I/O operations to be submitted by the owning thread.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL_INIT {
    uint32_t                                   PoolId;             /* One of _CORE_ASYNCIO_REQUEST_POOL_ID, or any application-defined value unique within the I/O system identifying the type of request pool. */
    uint32_t                                   PoolCount;          /* The number of I/O request pools of this type that will be allocated. */
    uint32_t                                   PoolCapacity;       /* The maximum number of requests that can be allocated from the pool. */
    uint32_t                                   Reserved;           /* Reserved for future use. Set to zero. */
} CORE_ASYNCIO_REQUEST_POOL_INIT;

/* @summary Define the attributes used to initialize an I/O request pool storage object.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE_INIT {
    CORE_ASYNCIO_REQUEST_POOL_INIT            *RequestPoolTypes;   /* An array of structures defining the types of I/O request pools used by the application. */
    uint32_t                                   PoolTypeCount;      /* The number of _CORE_ASYNCIO_REQUEST_POOL_INIT structures in the RequestPoolTypes array. */
    void                                      *MemoryStart;        /* A pointer to the start of the memory block allocated for the storage object. */
    uint64_t                                   MemorySize;         /* The total size of the memory block allocated for the storage array. */
} CORE_ASYNCIO_REQUEST_POOL_STORAGE_INIT;

/* @summary Define the configuration data used to create an I/O worker thread pool.
 */
typedef struct _CORE_ASYNCIO_WORKER_POOL_INIT {
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *RequestPoolStorage; /* The _CORE_ASYNCIO_REQUEST_POOL_STORAGE object. */
    CORE_AsyncIoWorkerThreadInit_Func          ThreadInitFunc;     /* The callback function to invoke to create any thread-local data. */
    uintptr_t                                  PoolContext;        /* Opaque data associated with the thread pool and available to all worker threads. */
    void                                      *PoolMemory;         /* The owner-managed memory block used for all storage within the pool. */
    uint64_t                                   PoolMemorySize;     /* The size of the pool memory block, in bytes. This value can be determined for a given pool capacity using CORE_QueryAsyncIoThreadPoolMemorySize. */
    uint32_t                                   WorkerCount;        /* The number of worker threads in the pool. */
} CORE_ASYNCIO_WORKER_POOL_INIT;

/* @summary Define the supported types of asynchronous I/O requests.
 */
typedef enum _CORE_ASYNCIO_REQUEST_TYPE {
    CORE_ASYNCIO_REQUEST_TYPE_NOOP                         =  0,  /* Ignore the operation and pass through the data unchanged. */
    CORE_ASYNCIO_REQUEST_TYPE_OPEN_FILE                    =  1,  /* Asynchronously open a file. */
    CORE_ASYNCIO_REQUEST_TYPE_READ_FILE                    =  2,  /* Issue an explicit asynchronous file read. */
    CORE_ASYNCIO_REQUEST_TYPE_WRITE_FILE                   =  3,  /* Issue an explicit asynchronous file write. */
    CORE_ASYNCIO_REQUEST_TYPE_FLUSH_FILE                   =  4,  /* Issue an explicit asynchronous file flush. */
    CORE_ASYNCIO_REQUEST_TYPE_CLOSE_FILE                   =  5,  /* Asynchronously close a file. */
} CORE_ASYNCIO_REQUEST_TYPE;

/* @summary Define the states of an asynchronous I/O request.
 */
typedef enum _CORE_ASYNCIO_REQUEST_STATE {
    CORE_ASYNCIO_REQUEST_STATE_CHAINED                     =  0,  /* The I/O request has been submitted as a chained request, which executes immediately and is not queued. */
    CORE_ASYNCIO_REQUEST_STATE_SUBMITTED                   =  1,  /* The I/O request has been submitted, but not yet launched. */
    CORE_ASYNCIO_REQUEST_STATE_LAUNCHED                    =  2,  /* The I/O request has been picked up by a worker thread and is executing. */
    CORE_ASYNCIO_REQUEST_STATE_COMPLETED                   =  3,  /* The I/O request has been completed. */
} CORE_ASYNCIO_REQUEST_STATE;

/* @summary Define some well-known I/O request pool identifiers.
 */
typedef enum _CORE_ASYNCIO_REQUEST_POOL_ID {
    CORE_ASYNCIO_REQUEST_POOL_ID_MAIN                      =  0,  /* The identifier for the I/O request pool associated with the main thread. */
    CORE_ASYNCIO_REQUEST_POOL_ID_WORKER                    =  1,  /* The identifier for the I/O request pool associated with an I/O or worker thread. */
    CORE_ASYNCIO_REQUEST_POOL_ID_USER                      =  2,  /* The identifier of the first custom application pool type. */
} CORE_ASYNCIO_REQUEST_POOL_ID;

/* @summary Define the possible validation codes that can be generated by CORE_ValidateIoRequestPoolConfiguration.
 */
typedef enum _CORE_ASYNCIO_POOL_VALIDATION_RESULT {
    CORE_ASYNCIO_POOL_VALIDATION_RESULT_SUCCESS            =  0,  /* No issue was detected. */
    CORE_ASYNCIO_POOL_VALIDATION_RESULT_NO_POOL_TYPES      =  1,  /* No pool types were supplied. */
    CORE_ASYNCIO_POOL_VALIDATION_RESULT_DUPLICATE_ID       =  2,  /* The same PoolId is used for more than one CORE_ASYNCIO_REQUEST_POOL_INIT structure. */
} CORE_ASYNCIO_POOL_VALIDATION_RESULT;

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

#if 0
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
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Inspect one or more I/O request pool configurations and perform validation checks against them.
 * @param type_configs An array of type_count CORE_ASYNCIO_REQUEST_POOL_INIT structures defining the configuration for each type of request pool.
 * @param type_results An array of type_count integers where the validation results for each CORE_ASYNCIO_REQUEST_POOL_INIT will be written.
 * @param type_count The number of elements in the type_configs and type_results arrays.
 * @param global_result On return, any non type-specific validation error is written to this location.
 * @return Zero if the task pool configurations are all valid, or -1 if one or more problems were detected.
 */
CORE_API(int)
CORE_ValidateIoRequestPoolConfiguration
(
    CORE_ASYNCIO_REQUEST_POOL_INIT *type_configs, 
    int32_t                        *type_results, 
    uint32_t                          type_count, 
    int32_t                       *global_result
);

/* @summary Determine the amount of memory required to initialize an I/O request pool storage object with the given configuration.
 * @param type_configs An array of type_count CORE_ASYNCIO_REQUEST_POOL_INIT structures defining the configuration for each I/O request pool type.
 * @param type_count The number of elements in the type_configs array.
 * @return The minimum number of bytes required to successfully initialize an I/O request pool storage object with the given configuration.
 */
CORE_API(size_t)
CORE_QueryIoRequestPoolStorageMemorySize
(
    CORE_ASYNCIO_REQUEST_POOL_INIT *type_configs, 
    uint32_t                          type_count
);

/* @summary Initialize an I/O request pool storage blob.
 * @param storage The CORE_ASYNCIO_REQUEST_POOL_STORAGE to initialize.
 * @param init Data used to configure the storage pool.
 * @return Zero if the storage object is successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateIoRequestPoolStorage
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE **storage, 
    CORE_ASYNCIO_REQUEST_POOL_STORAGE_INIT        *init
);

/* @summary Free all resources associated with an I/O request pool storage object.
 * @param storage The CORE_ASYNCIO_REQUEST_POOL_STORAGE to delete.
 */
CORE_API(void)
CORE_DeleteIoRequestPoolStorage
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage
);

/* @summary Query an I/O request pool storage object for the total number of I/O request pools it manages.
 * @param storage The CORE_ASYNCIO_REQUEST_POOL_STORAGE object to query.
 * @return The total number of I/O request pools managed by the storage object.
 */
CORE_API(uint32_t)
CORE_QueryIoRequestPoolTotalCount
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage
);

/* @summary Acquire an I/O request pool and bind it to the calling thread.
 * This function is safe to call from multiple threads simultaneously.
 * This function should not be called from performance-critical code, as it may block.
 * @param storage The CORE_ASYNCIO_REQUEST_POOL_STORAGE object from which the pool should be acquired.
 * @param pool_type_id One of CORE_ASYNCIO_REQUEST_POOL_ID, or an application-defined value specifying the pool type to acquire.
 * @return A pointer to the I/O request pool object, or NULL if no pool of the specified type could be acquired.
 */
CORE_API(struct _CORE_ASYNCIO_REQUEST_POOL*)
CORE_AcquireIoRequestPool
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage,
    uint32_t                              pool_type_id
);

/* @summary Release an I/O request pool back to the storage object it was allocated from.
 * @param pool The I/O request pool object to release.
 */
CORE_API(void)
CORE_ReleaseIoRequestPool
(
    struct _CORE_ASYNCIO_REQUEST_POOL *pool
);

/* @summary Query an I/O request pool for the maximum number of simultaneously-active I/O requests.
 * @param pool The I/O request pool object to query.
 * @return The maximum number of uncompleted requests that can be defined against the pool.
 */
CORE_API(uint32_t)
CORE_QueryIoRequestPoolCapacity
(
    struct _CORE_ASYNCIO_REQUEST_POOL *pool
);

/* @summary Provide a default worker thread initialization function. On return, the thread_context argument is set to zero.
 * @param pool The I/O thread pool that owns the worker thread.
 * @param pool_context Opaque data supplied when the I/O pool was created.
 * @param thread_id The operating system identifier of the worker thread.
 * @param thread_context On return, the function should update this value to point to any data to be associated with the thread.
 * @return Zero if initialization completes successfully, or -1 if initialization failed.
 */
CORE_API(int)
CORE_AsyncIoWorkerThreadInitDefault
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool, 
    uintptr_t                 pool_context, 
    uint32_t                     thread_id, 
    uintptr_t              *thread_context
);

/* @summary Calculate the amount of memory required to launch an I/O worker pool with the specified number of worker threads.
 * @param worker_count The number of worker threads in the pool.
 * @return The number of bytes of memory required to launch an I/O worker pool with the specified number of workers.
 */
CORE_API(size_t)
CORE_QueryAsyncIoWorkerPoolMemorySize
(
    uint32_t worker_count
);

/* @summary Retrieve the application-defined data associated with an I/O worker pool.
 * @param pool The I/O worker pool to query.
 * @return The application-defined data associated with the I/O worker pool.
 */
CORE_API(uintptr_t)
CORE_QueryAsyncIoWorkerPoolContext
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool
);

/* @summary Initialize and launch a pool of I/O worker threads.
 * @param pool On return, this location is updated with a pointer to the worker pool object.
 * @param init Data used to configure the pool of worker threads.
 * @return Zero if the pool is initialized and launched successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_LaunchIoWorkerPool
(
    struct _CORE_ASYNCIO_WORKER_POOL **pool, 
    CORE_ASYNCIO_WORKER_POOL_INIT     *init
);

/* @summary Stop all threads and free all resources associated with a pool of I/O worker threads.
 * @param pool The _CORE_ASYNCIO_WORKER_POOL to terminate and delete.
 */
CORE_API(void)
CORE_TerminateIoWorkerPool
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool
);

/* @summary Initialize, but do not submit, an I/O request to be executed asynchronously.
 * This function is suitable for use in initializing a chained I/O request to be returned from a completion callback.
 * @param request_pool The _CORE_ASYNCIO_REQUEST_POOL owned by the calling thread.
 * @param init Data representing the I/O request to execute.
 * @return A pointer to the I/O request object, or NULL if the request data is invalid or no requests are available in the pool.
 */
CORE_API(struct _CORE_ASYNCIO_REQUEST*)
CORE_InitAsyncIoRequest
(
    struct _CORE_ASYNCIO_REQUEST_POOL *request_pool, 
    CORE_ASYNCIO_REQUEST_INIT                 *init
);

/* @summary Initialize and submit an I/O request to be executed asynchronously.
 * This function should not be called from an I/O request completion callback.
 * @param request_pool The _CORE_ASYNCIO_REQUEST_POOL owned by the calling thread.
 * @param worker_pool The thread pool used to execute the request.
 * @param init Data representing the request to execute.
 * @return Zero if the request is successfully submitted, or -1 if an error occurred.
 */
CORE_API(int)
CORE_SubmitAsyncIoRequest
(
    struct _CORE_ASYNCIO_REQUEST_POOL *request_pool,
    struct _CORE_ASYNCIO_WORKER_POOL   *worker_pool, 
    CORE_ASYNCIO_REQUEST_INIT                 *init
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_ASYNCIO_H__ */

#ifdef CORE_ASYNCIO_IMPLEMENTATION

/* @summary Define a special value posted to a thread when it should shutdown.
 */
#ifndef CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN
#define CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN    ~((ULONG_PTR)0)
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an instance of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 */
#ifndef CORE__AsyncIoAllocationSizeType
#define CORE__AsyncIoAllocationSizeType(_type)                                 \
    ((sizeof(_type)) + (__alignof(_type)-1))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an array of instances of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 * @param _count The number of elements in the array.
 */
#ifndef CORE__AsyncIoAllocationSizeArray
#define CORE__AsyncIoAllocationSizeArray(_type, _count)                        \
    ((sizeof(_type) * (_count)) + (__alignof(_type)-1))
#endif

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

/* @summary Define the data associated with an internal memory arena allocator.
 */
typedef struct _CORE__ASYNCIO_ARENA {
    uint8_t                                   *BaseAddress;        /* The base address of the memory range. */
    size_t                                     MemorySize;         /* The size of the memory block, in bytes. */
    size_t                                     NextOffset;         /* The offset of the next available address. */
} CORE__ASYNCIO_ARENA;

/* @summary Define the type used to mark a location within a memory arena.
 */
typedef size_t CORE__ASYNCIO_ARENA_MARKER;

/* @summary Define the data associated with an asynchronous I/O request.
 * Request objects are allocated from a pool that is typically thread-local.
 */
typedef struct _CORE_ASYNCIO_REQUEST {
    #define DATA_SIZE                          CORE_ASYNCIO_REQUEST_MAX_DATA
    struct _CORE_ASYNCIO_REQUEST              *NextRequest;        /* The next node in the request list, or NULL if this is the tail node. */
    struct _CORE_ASYNCIO_REQUEST_POOL         *RequestPool;        /* The _CORE_ASYNCIO_REQUEST_POOL from which the request was allocated. */
    CORE_AsyncIoCompletion_Func                CompleteCallback;   /* The callback to invoke when the I/O operation completes. */
    CORE_ASYNCIO_HANDLE                        Handle;             /* The handle associated with the operation. */
    uintptr_t                                  UserContext;        /* Opaque data to be passed through to the completion callback. */
    int32_t                                    RequestType;        /* One of _CORE_ASYNCIO_REQUEST_TYPE specifying the type of the request. */
    int32_t                                    RequestState;       /* One of _CORE_ASYNCIO_REQUEST_STATE specifying the state of the request. */
    int64_t                                    SubmitTime;         /* The timestamp, in ticks, at which the I/O request was submitted. */
    int64_t                                    LaunchTime;         /* The timestamp, in ticks, at which the I/O request was dequeued. */
    uint8_t                                    Data[DATA_SIZE];    /* Storage for request type-specific data associated with the request. */
    OVERLAPPED                                 Overlapped;         /* The OVERLAPPED instance associated used by the request. */
    #undef  DATA_SIZE
} CORE__ASYNCIO_REQUEST;

/* @summary Define the data associated with a pool of background I/O requests. 
 * Each thread that can submit I/O requests typically maintains its own request pool.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL {
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *Storage;            /* The _CORE_ASYNCIO_REQUEST_POOL_STORAGE that owns this I/O request pool. */
    struct _CORE_ASYNCIO_REQUEST_POOL         *NextPool;           /* Pointer to the next pool in the free list. */
    struct _CORE_ASYNCIO_REQUEST              *FreeRequestList;    /* Pointer to the first free request, or NULL if no requests are available. */
    struct _CORE_ASYNCIO_REQUEST              *RequestData;        /* An array of Capacity I/O request objects. This is the raw storage underlying the live and free lists. */
    uint32_t                                   Capacity;           /* The maximum number of requests that can be allocated from the pool. */
    uint32_t                                   OwningThread;       /* The operating system identifier of the thread that owns the pool. */
    uint32_t                                   PoolIndex;          /* The zero-based index of the pool within the storage object. */
    uint32_t                                   PoolId;             /* One of _CORE_ASYNCIO_REQUEST_POOL_ID acting as an identifier for the request pool type. */
    CRITICAL_SECTION                           RequestListLock;    /* Lock protecting the request free list, which may be accessed concurrently by a submitting thread and one or more I/O worker threads. */
} CORE__ASYNCIO_REQUEST_POOL;

/* @summary Define the data representing a fixed set of I/O request data pools.
 */
typedef struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE {
    CORE__ASYNCIO_REQUEST_POOL               **RequestPoolList;    /* Pointers to each I/O request pool object. */
    uint32_t                                   RequestPoolCount;   /* The total number of I/O request pool objects. */
    uint32_t                                   PoolTypeCount;      /* The number of request pool types defined within the storage object. */
    uint32_t                                  *PoolTypeIds;        /* An array of PoolTypeCount values, where each value specifies the I/O request pool ID. */
    CORE__ASYNCIO_REQUEST_POOL               **PoolFreeList;       /* An array of PoolTypeCount pointers to the free list for each I/O request pool ID. */
    CRITICAL_SECTION                          *PoolFreeListLocks;  /* An array of PoolTypeCount critical sections protecting the free list for each I/O request pool ID. */
    void                                      *MemoryStart;        /* A pointer to the start of the memory block allocated for the storage object. */
    uint64_t                                   MemorySize;         /* The total size of the memory block allocated for the storage object. */
} CORE__ASYNCIO_REQUEST_POOL_STORAGE;

/* @summary Define the data associated with a pool of background I/O worker threads.
 */
typedef struct _CORE_ASYNCIO_WORKER_POOL {
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *RequestPoolStorage; /* The I/O request pool storage object that maintains the I/O completion port to monitor. */
    uint32_t                                   ActiveThreads;      /* The number of currently active threads in the pool. */
    uint32_t                                   WorkerCount;        /* The maximum number of active worker threads in the pool. */
    unsigned int                              *OSThreadIds;        /* An array of WorkerCount operating system thread identifiers, of which ActiveThreads entries are valid. */
    HANDLE                                    *OSThreadHandle;     /* An array of WorkerCount operating system thread handles, of which ActiveThreads entries are valid.*/
    HANDLE                                    *WorkerReady;        /* An array of WorkerCount manual-reset event handles, of which ActiveThreads entries are valid. */
    HANDLE                                    *WorkerError;        /* An array of WorkerCount manual-reset event handles, of which ActiveThreads entries are valid. */
    HANDLE                                     CompletionPort;     /* The I/O completion port used to receive asynchronous I/O completion notifications. */
    HANDLE                                     TerminateSignal;    /* A manual-reset event to be signaled by the application when the pool should terminate. */
    uintptr_t                                  ContextData;        /* Opaque data associated with the thread pool and available to all worker threads. */
    void                                      *PoolMemory;         /* The owner-managed memory block used for all storage within the pool. */
    uint64_t                                   PoolMemorySize;     /* The size of the pool memory block, in bytes. This value can be determined for a given pool capacity using CORE_QueryAsyncIoThreadPoolMemorySize. */
} CORE__ASYNCIO_WORKER_POOL;

/* @summary Define the data passed to an I/O worker thread on startup.
 */
typedef struct _CORE__ASYNCIO_THREAD_INIT {
    CORE_AsyncIoWorkerThreadInit_Func          ThreadInit;         /* The callback function to invoke to create any thread-local data. */
    CORE__ASYNCIO_WORKER_POOL                 *ThreadPool;         /* The I/O thread pool that owns the worker thread. */
    CORE__ASYNCIO_REQUEST_POOL_STORAGE        *RequestPoolStorage; /* The I/O request pool storage object that manages the I/O completion port to monitor. */
    HANDLE                                     ReadySignal;        /* A manual-reset event to be signaled by the worker when it has successfully completed initialization and is ready to run. */
    HANDLE                                     ErrorSignal;        /* A manual-reset event to be signaled by the worker before it terminates when it encounters a fatal error. */
    HANDLE                                     TerminateSignal;    /* A manual-reset event to be signaled by the application when the worker should terminate. */
    HANDLE                                     CompletionPort;     /* The I/O completion port used to receive asynchronous I/O completion notifications. */
    uintptr_t                                  PoolContext;        /* Opaque data associated with the I/O thread pool. */
} CORE__ASYNCIO_THREAD_INIT;

/* @summary Define the data returned from the completion of an I/O request.
 */
typedef struct _CORE__ASYNCIO_COMPLETION {
    DWORD                                      ResultCode;         /* The operating system result code returned by the operation. */
    DWORD                                      BytesTransferred;   /* The total number of bytes transferred during the operation. */
    DWORD                                      WSAFlags;           /* The output flags value if the operation was a socket or datagram read, otherwise zero.*/
    int                                        Success;            /* The overall success status of the operation, where non-zero indicates success and zero indicates failure. */
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
static CORE__ASYNCIO_REQUEST*
CORE__AsyncIoRequestForOVERLAPPED
(
    OVERLAPPED *overlapped
)
{
    return ((CORE__ASYNCIO_REQUEST*)(((uint8_t*)overlapped) - offsetof(CORE__ASYNCIO_REQUEST, Overlapped)));
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
    CORE__ASYNCIO_REQUEST *request, 
    OVERLAPPED         *overlapped, 
    LPDWORD            transferred, 
    BOOL                      wait, 
    LPDWORD                  flags, 
    LPDWORD                  error
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
    assert(0 && "CORE__AsyncIoGetOverlappedResult called for request with invalid type");
    SetLastError(ERROR_NOT_SUPPORTED);
   *error = ERROR_NOT_SUPPORTED;
   *flags = 0;
    return FALSE;
}

/* @summary Acquire an I/O request object from a pool.
 * @param pool The I/O request pool from which the request object should be acquired.
 * @return The I/O request object, or NULL if the pool has no available requests.
 */
static struct _CORE_ASYNCIO_REQUEST*
CORE__AsyncIoAcquireRequest
(
    struct _CORE_ASYNCIO_REQUEST_POOL *pool
)
{
    CORE__ASYNCIO_REQUEST *req = NULL;
    EnterCriticalSection(&pool->RequestListLock);
    {   /* return the node at the head of the free list */
        if ((req = pool->FreeRequestList) != NULL)
        {   /* a node is available, pop from the free list */
            pool->FreeRequestList = req->NextRequest;
            req->NextRequest = NULL;
        }
    }
    LeaveCriticalSection(&pool->RequestListLock);
    return req;
}

/* @summary Return an I/O request object to the pool it was acquired from.
 * @param request The I/O request object to return.
 */
static void
CORE__AsyncIoReturnRequest
(
    struct _CORE_ASYNCIO_REQUEST *request
)
{   assert(request != NULL);
    assert(request->RequestPool != NULL);
    EnterCriticalSection(&request->RequestPool->RequestListLock);
    {   /* return the request to the front of the free list */
        request->NextRequest = request->RequestPool->FreeRequestList;
        request->RequestPool->FreeRequestList = request;
    }
    LeaveCriticalSection(&request->RequestPool->RequestListLock);
}

/* @summary Submit an asynchronous I/O request.
 * @param pool The _CORE_ASYNCIO_WORKER_POOL that will execute the request.
 * @param request The I/O request to submit. Any data associated with the request must remain valid until the request completes.
 * @return Zero if the request is submitted successfully, or -1 if an error occurred.
 */
static int
CORE__AsyncIoSubmitRequest
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool, 
    struct _CORE_ASYNCIO_REQUEST  *request
)
{
    LARGE_INTEGER timestamp;
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

/* @summary Complete an I/O request and return it to the request pool.
 * @param req The I/O request that has completed.
 * @param pool The I/O thread pool that executed the request.
 * @param comp The I/O request completion information.
 * @param frequency The frequency of the high-resolution timer, in counts-per-second.
 * @return A pointer to the chained request to execute immediately, or NULL.
 */
static CORE__ASYNCIO_REQUEST*
CORE__AsyncIoCompleteRequest
(
    CORE__ASYNCIO_REQUEST      *req, 
    CORE__ASYNCIO_WORKER_POOL *pool, 
    CORE__ASYNCIO_COMPLETION  *comp, 
    LARGE_INTEGER         frequency
)
{
    CORE__ASYNCIO_REQUEST *chained = NULL;
    CORE_ASYNCIO_RESULT        res;
    LARGE_INTEGER           timest;

    /* retrieve the completion timestamp */
    QueryPerformanceCounter(&timest);
    /* transition the request to the completed state */
    req->RequestState    = CORE_ASYNCIO_REQUEST_STATE_COMPLETED;
    /* populate the result object and invoke the callback */
    res.RequestPool      = req->RequestPool;
    res.WorkerPool       = pool;
    res.RequestData      = req->Data;
    res.RequestHandle    = req->Handle;
    res.RequestContext   = req->UserContext;
    res.ResultCode       = comp->ResultCode;
    res.BytesTransferred = comp->BytesTransferred;
    res.ExecutionTime    = CORE__AsyncIoElapsedNanoseconds(timest.QuadPart, req->LaunchTime, frequency.QuadPart);
    res.QueueDelay       = CORE__AsyncIoElapsedNanoseconds(req->LaunchTime, req->SubmitTime, frequency.QuadPart);
    if ((chained = req->CompleteCallback(req, &res, comp->Success)) != NULL)
    {    /* retrieve the submission timestamp for the chained request */
       QueryPerformanceCounter(&timest);
       chained->SubmitTime = timest.QuadPart;
       chained->LaunchTime = timest.QuadPart;
    }
    CORE__AsyncIoReturnRequest(req);
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
    CORE__ASYNCIO_REQUEST     *req, 
    CORE__ASYNCIO_COMPLETION *comp, 
    HANDLE                    iocp
)
{
    CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_OPEN_REQUEST_DATA*) req->Data;
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
    CORE__ASYNCIO_REQUEST     *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{
    CORE_ASYNCIO_FILE_READ_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_READ_REQUEST_DATA*) req->Data;
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
    CORE__ASYNCIO_REQUEST     *req, 
    CORE__ASYNCIO_COMPLETION *comp
)
{
    CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA *data = (CORE_ASYNCIO_FILE_WRITE_REQUEST_DATA*) req->Data;
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
    CORE__ASYNCIO_REQUEST     *req, 
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
    CORE__ASYNCIO_REQUEST     *req, 
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
    CORE__ASYNCIO_REQUEST     *req, 
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
    CORE__ASYNCIO_WORKER_POOL *pool = init->ThreadPool;
    CORE__ASYNCIO_REQUEST      *req = NULL;
    OVERLAPPED                  *ov = NULL;
    uintptr_t        thread_context = 0;
    uintptr_t          pool_context = init->PoolContext;
    ULONG_PTR                   key = 0;
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
                                  assert(0 && "I/O request object has unknown state value");
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

CORE_API(int)
CORE_ValidateIoRequestPoolConfiguration
(
    CORE_ASYNCIO_REQUEST_POOL_INIT *type_configs, 
    int32_t                        *type_results, 
    uint32_t                          type_count, 
    int32_t                       *global_result
)
{
    uint32_t      i, j;
    uint64_t num_pools = 0;
    int         result = 0; /* assume success */

    if (type_configs == NULL)
    {   assert(type_configs != NULL);
        return -1;
    }
    if (type_results == NULL)
    {   assert(type_results != NULL);
        return -1;
    }
    if (global_result == NULL)
    {   assert(global_result != NULL);
        return -1;
    }
    if (type_count == 0)
    {   assert(type_count > 0);
       *global_result = CORE_ASYNCIO_POOL_VALIDATION_RESULT_NO_POOL_TYPES;
        return -1;
    }
    /* start out assuming no problems */
   *global_result = CORE_ASYNCIO_POOL_VALIDATION_RESULT_SUCCESS;
    for (i = 0; i < type_count; ++i)
    {   /* start out assuming that the pool configuration is valid */
        num_pools      += type_configs[i].PoolCount;
        type_results[i] = CORE_ASYNCIO_POOL_VALIDATION_RESULT_SUCCESS;

        for (j = 0; j < type_count; ++j)
        {
            if (i != j && type_configs[i].PoolId == type_configs[j].PoolId)
            {   /* the same PoolId is used for more than one pool configuration */
                assert(type_configs[i].PoolId != type_configs[j].PoolId);
                type_results[i] = CORE_ASYNCIO_POOL_VALIDATION_RESULT_DUPLICATE_ID;
                result = -1;
                break;
            }
        }
    }
    return result;
}

CORE_API(size_t)
CORE_QueryIoRequestPoolStorageMemorySize
(
    CORE_ASYNCIO_REQUEST_POOL_INIT *type_configs, 
    uint32_t                          type_count
)
{
    size_t required_size = 0;
    uint32_t  pool_count = 0;
    uint32_t     i, j, n;
    /* calculate the total number of request pools that will be allocated */
    for (i = 0; i < type_count; ++i)
    {
        pool_count += type_configs[i].PoolCount;
    }
    /* calculate the amount of memory required for data stored directly in _CORE_ASYNCIO_REQUEST_POOL_STORAGE.
     * this includes the size of the structure itself, since all data is private.
     */
    required_size += CORE__AsyncIoAllocationSizeType (CORE__ASYNCIO_REQUEST_POOL_STORAGE);
    required_size += CORE__AsyncIoAllocationSizeArray(CORE__ASYNCIO_REQUEST_POOL*, pool_count); /* RequestPoolList   */
    required_size += CORE__AsyncIoAllocationSizeArray(uint32_t                   , type_count); /* PoolTypeIds       */
    required_size += CORE__AsyncIoAllocationSizeArray(CORE__ASYNCIO_REQUEST_POOL*, type_count); /* PoolFreeList      */
    required_size += CORE__AsyncIoAllocationSizeArray(CRITICAL_SECTION           , type_count); /* PoolFreeListLocks */
    /* calculate the amount of memory required for each _CORE_ASYNCIO_REQUEST_POOL in the storage blob */
    for (i = 0; i < type_count; ++i)
    {
        size_t pool_size = 0;
        pool_size       += CORE__AsyncIoAllocationSizeType (CORE__ASYNCIO_REQUEST_POOL);
        pool_size       += CORE__AsyncIoAllocationSizeArray(CORE__ASYNCIO_REQUEST, type_configs[i].PoolCapacity);
        required_size   +=(pool_size * type_configs[i].PoolCount);
    }
    return required_size;
}

CORE_API(int)
CORE_CreateIoRequestPoolStorage
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE **storage, 
    CORE_ASYNCIO_REQUEST_POOL_STORAGE_INIT        *init
)
{
    CORE__ASYNCIO_ARENA arena;
    CORE__ASYNCIO_REQUEST_POOL_STORAGE *stor = NULL;
    CORE__ASYNCIO_REQUEST_POOL         *pool = NULL;
    size_t                     required_size = 0;
    uint32_t                      pool_index = 0;
    uint32_t                      pool_count = 0;
    uint32_t                      i, j, n, m;

    if (init->PoolTypeCount == 0)
    {   /* the storage object must have at least one pool type */
        assert(init->PoolTypeCount > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MemoryStart == NULL || init->MemorySize == 0)
    {   /* the caller must supply memory for the storage object data */
        assert(init->MemoryStart != NULL);
        assert(init->MemorySize > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    required_size = CORE_QueryIoRequestPoolStorageMemorySize(init->RequestPoolTypes, init->PoolTypeCount);
    if (init->MemorySize < required_size)
    {   /* the caller must supply sufficient memory for the storage object */
        assert(init->MemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* calculate the total number of request pools that will be allocated */
    for (i = 0, n = init->PoolTypeCount; i < n; ++i)
    {
        pool_count += init->RequestPoolTypes[i].PoolCount;
    }

    /* zero-initialize the entire memory block and allocate the base structure */
    ZeroMemory(init->MemoryStart,(size_t)init->MemorySize);
    CORE__AsyncIoInitMemoryArena(&arena, init->MemoryStart, (size_t) init->MemorySize);
    stor                    = CORE__AsyncIoMemoryArenaAllocateType (&arena, CORE__ASYNCIO_REQUEST_POOL_STORAGE);
    stor->RequestPoolList   = CORE__AsyncIoMemoryArenaAllocateArray(&arena, CORE__ASYNCIO_REQUEST_POOL*, pool_count);
    stor->PoolFreeList      = CORE__AsyncIoMemoryArenaAllocateArray(&arena, CORE__ASYNCIO_REQUEST_POOL*, init->PoolTypeCount);
    stor->PoolFreeListLocks = CORE__AsyncIoMemoryArenaAllocateArray(&arena, CRITICAL_SECTION           , init->PoolTypeCount);
    stor->PoolTypeIds       = CORE__AsyncIoMemoryArenaAllocateArray(&arena, uint32_t                   , init->PoolTypeCount);
    stor->RequestPoolCount  = pool_count;
    stor->PoolTypeCount     = init->PoolTypeCount;
    stor->MemoryStart       = init->MemoryStart;
    stor->MemorySize        = init->MemorySize;
    /* clear out the loop variables used during failure cleanup */
    i = j = n = m = 0;
    /* allocate and initialize the request pool objects themselves */
    for (i = 0, n = init->PoolTypeCount; i < n; ++i)
    {   /* copy the type identifier into the storage blob */
        stor->PoolTypeIds[i] = init->RequestPoolTypes[i].PoolId;
        /* initialize the critical section protecting the per-type free list */
        if (!InitializeCriticalSectionAndSpinCount(&stor->PoolFreeListLocks[i], 0x1000))
        {   /* this shouldn't happen on XP and above */
            goto cleanup_and_fail;
        }
        /* create the specified number of request pools of the current type */
        for (j = 0, m = init->RequestPoolTypes[i].PoolCount; j < m; ++j)
        {   /* initialize the individual CORE__ASYNCIO_REQUEST_POOL object */
            pool                  = CORE__AsyncIoMemoryArenaAllocateType (&arena, CORE__ASYNCIO_REQUEST_POOL);
            pool->Storage         = stor;
            pool->NextPool        = stor->PoolFreeList[i];
            pool->FreeRequestList = NULL;
            pool->RequestData     = CORE__AsyncIoMemoryArenaAllocateArray(&arena, CORE__ASYNCIO_REQUEST, init->RequestPoolTypes[i].PoolCapacity);
            pool->Capacity        = init->RequestPoolTypes[i].PoolCapacity;
            pool->OwningThread    = 0;
            pool->PoolIndex       = pool_index;
            pool->PoolId          = init->RequestPoolTypes[i].PoolId;
            if (!InitializeCriticalSectionAndSpinCount(&pool->RequestListLock, 0x1000))
            {   /* this shouldn't happen on XP and above */
                goto cleanup_and_fail;
            }
            stor->PoolFreeList[i] = pool;
            stor->RequestPoolList[pool_index++] = pool;
        }
    }
   *storage = stor;
    return 0;

cleanup_and_fail:
    if (stor != NULL)
    {   /* destroy the per-pool critical sections */
        for (j = 0; j < pool_index; ++j)
        {
            DeleteCriticalSection(&stor->RequestPoolList[j]->RequestListLock);
        }
        /* destroy the per-type free list critical sections */
        for (j = 0; j < i; ++j)
        {
            DeleteCriticalSection(&stor->PoolFreeListLocks[j]);
        }
    }
   *storage = NULL;
    return -1;
}

CORE_API(void)
CORE_DeleteIoRequestPoolStorage
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage
)
{
    if (storage != NULL)
    {
        uint32_t i, n;
        for (i = 0, n = storage->RequestPoolCount; i < n; ++i)
        {
            DeleteCriticalSection(&storage->RequestPoolList[i]->RequestListLock);
        }
        for (i = 0, n = storage->PoolTypeCount; i < n; ++i)
        {
            DeleteCriticalSection(&storage->PoolFreeListLocks[i]);
        }
    }
}

CORE_API(uint32_t)
CORE_QueryIoRequestPoolTotalCount
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage
)
{
    return storage->RequestPoolCount;
}

CORE_API(struct _CORE_ASYNCIO_REQUEST_POOL*)
CORE_AcquireIoRequestPool
(
    struct _CORE_ASYNCIO_REQUEST_POOL_STORAGE *storage,
    uint32_t                              pool_type_id
)
{
    CORE__ASYNCIO_REQUEST_POOL *pool = NULL;
    uint32_t           *pool_id_list = storage->PoolTypeIds;
    uint32_t         pool_type_count = storage->PoolTypeCount;
    uint32_t         pool_type_index = 0;
    int              pool_type_found = 0;
    uint32_t                    i, n;

    for (i = 0; i < pool_type_count; ++i)
    {
        if (pool_id_list[i] == pool_type_id)
        {
            pool_type_found  = 1;
            pool_type_index  = i;
            break;
        }
    }
    if (pool_type_found)
    {   /* attempt to pop a pool of the specified type from the free list */
        EnterCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
        {
            if (storage->PoolFreeList[pool_type_index] != NULL)
            {   /* the free list is non-empty - pop a node */
                pool = storage->PoolFreeList[pool_type_index];
                storage->PoolFreeList[pool_type_index] = pool->NextPool;
            }
        }
        LeaveCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
    }
    if (pool != NULL)
    {   /* a pool was successfully acquired - bind it to the calling thread.
         * this is expensive, since we have to ensure that the pool is properly initialized.
         * this entails re-creating and re-initializing the free list.
         */
        ZeroMemory(pool->RequestData, pool->Capacity * sizeof(CORE__ASYNCIO_REQUEST));
        pool->NextPool        = NULL;
        pool->FreeRequestList = NULL;
        pool->OwningThread    = GetCurrentThreadId();
        for (i = 0, n = pool->Capacity; i < n; ++i)
        {
            pool->RequestData[i].NextRequest = pool->FreeRequestList;
            pool->RequestData[i].RequestPool = pool;
            pool->FreeRequestList = &pool->RequestData[i];
        }
    }
    return pool;
}

CORE_API(void)
CORE_ReleaseIoRequestPool
(
    struct _CORE_ASYNCIO_REQUEST_POOL *pool
)
{
    CORE__ASYNCIO_REQUEST_POOL_STORAGE *storage = NULL;
    uint32_t                      *pool_id_list = NULL;
    uint32_t                       pool_type_id = 0;
    uint32_t                    pool_type_count = 0;
    uint32_t                    pool_type_index = 0;
    int                         pool_type_found = 0;
    uint32_t                                  i;

    if (pool == NULL)
    {   assert(pool != NULL);
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }
    if (pool->Storage == NULL)
    {   assert(pool->Storage != NULL);
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    /* locate the pool type in the storage object */
    storage         = pool->Storage;
    pool_type_id    = pool->PoolId;
    pool_id_list    = storage->PoolTypeIds;
    pool_type_count = storage->PoolTypeCount;
    for (i = 0; i < pool_type_count; ++i)
    {
        if (pool_id_list[i] == pool_type_id)
        {
            pool_type_found  = 1;
            pool_type_index  = i;
            break;
        }
    }
    if (pool_type_found)
    {   /* push the pool onto the front of the free list */
        EnterCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
        {
            pool->NextPool = storage->PoolFreeList[pool_type_index];
            storage->PoolFreeList[pool_type_index] = pool;
        }
        LeaveCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
    }
}

CORE_API(uint32_t)
CORE_QueryIoRequestPoolCapacity
(
    struct _CORE_ASYNCIO_REQUEST_POOL *pool
)
{
    return pool->Capacity;
}

CORE_API(int)
CORE_AsyncIoWorkerThreadInitDefault
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool, 
    uintptr_t                 pool_context, 
    uint32_t                     thread_id, 
    uintptr_t              *thread_context
)
{
    UNREFERENCED_PARAMETER(pool);
    UNREFERENCED_PARAMETER(pool_context);
    UNREFERENCED_PARAMETER(thread_id);
    *thread_context = 0;
    return 0;
}

CORE_API(size_t)
CORE_QueryAsyncIoWorkerPoolMemorySize
(
    uint32_t worker_count
)
{
    size_t required_size = 0;
    /* calculate the amount of memory required for the data stored directly in _CORE_ASYNCIO_WORKER_POOL.
     * this includes the size of the structure itself, since all data is private.
     */
    required_size += CORE__AsyncIoAllocationSizeType (CORE__ASYNCIO_WORKER_POOL);
    required_size += CORE__AsyncIoAllocationSizeArray(unsigned int, worker_count); /* OSThreadIds    */
    required_size += CORE__AsyncIoAllocationSizeArray(HANDLE      , worker_count); /* OSThreadHandle */
    required_size += CORE__AsyncIoAllocationSizeArray(HANDLE      , worker_count); /* WorkerReady    */
    required_size += CORE__AsyncIoAllocationSizeArray(HANDLE      , worker_count); /* WorkerError    */
    return required_size;
}

CORE_API(uintptr_t)
CORE_QueryAsyncIoWorkerPoolContext
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool
)
{
    return pool->ContextData;
}

CORE_API(int)
CORE_LaunchIoWorkerPool
(
    struct _CORE_ASYNCIO_WORKER_POOL **pool, 
    CORE_ASYNCIO_WORKER_POOL_INIT     *init
)
{
    CORE__ASYNCIO_ARENA arena;
    CORE__ASYNCIO_WORKER_POOL *worker_pool = NULL;
    HANDLE                            iocp = NULL;
    HANDLE                            term = NULL;
    size_t                   required_size = 0;
    int                           shutdown = 0;
    uint32_t                          i, n;

    if (init->WorkerCount == 0)
    {   /* the thread pool must have at least one worker */
        assert(init->WorkerCount > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->RequestPoolStorage == NULL)
    {   /* the caller must supply a _CORE_ASYNCIO_REQUEST_POOL_STORAGE object */
        assert(init->RequestPoolStorage != NULL);
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

    required_size = CORE_QueryAsyncIoWorkerPoolMemorySize(init->WorkerCount);
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
    ZeroMemory(init->PoolMemory,(size_t) init->PoolMemorySize);
    CORE__AsyncIoInitMemoryArena(&arena, init->PoolMemory, (size_t) init->PoolMemorySize);
    worker_pool                     = CORE__AsyncIoMemoryArenaAllocateType (&arena, CORE__ASYNCIO_WORKER_POOL);
    worker_pool->OSThreadIds        = CORE__AsyncIoMemoryArenaAllocateArray(&arena, unsigned int, init->WorkerCount);
    worker_pool->OSThreadHandle     = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    worker_pool->WorkerReady        = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    worker_pool->WorkerError        = CORE__AsyncIoMemoryArenaAllocateArray(&arena, HANDLE      , init->WorkerCount);
    worker_pool->RequestPoolStorage = init->RequestPoolStorage;
    worker_pool->ActiveThreads      = 0;
    worker_pool->WorkerCount        = init->WorkerCount;
    worker_pool->CompletionPort     = iocp;
    worker_pool->TerminateSignal    = term;
    worker_pool->ContextData        = init->PoolContext;
    worker_pool->PoolMemory         = init->PoolMemory;
    worker_pool->PoolMemorySize     = init->PoolMemorySize;

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

        winit.ThreadInit         = init->ThreadInitFunc;
        winit.ThreadPool         = worker_pool;
        winit.RequestPoolStorage = init->RequestPoolStorage;
        winit.ReadySignal        = wready;
        winit.ErrorSignal        = werror;
        winit.TerminateSignal    = term;
        winit.CompletionPort     = iocp;
        winit.PoolContext        = init->PoolContext;
        if ((whand = (HANDLE)_beginthreadex(NULL, 64 * 1024, CORE__AsyncIoThreadMain, &winit, 0, &thread_id)) == NULL)
        {
            CloseHandle(werror);
            CloseHandle(wready);
            goto cleanup_and_fail;
        }
        worker_pool->OSThreadIds   [worker_pool->ActiveThreads] = thread_id;
        worker_pool->OSThreadHandle[worker_pool->ActiveThreads] = whand;
        worker_pool->WorkerReady   [worker_pool->ActiveThreads] = wready;
        worker_pool->WorkerError   [worker_pool->ActiveThreads] = werror;
        worker_pool->ActiveThreads++;
        shutdown = 1;

        /* wait for the thread to become ready, or fail to initialize */
        wset[THREAD_READY] = wready;
        wset[THREAD_ERROR] = werror;
        if ((wait_rc = WaitForMultipleObjects(WAIT_COUNT, wset, FALSE, INFINITE)) != (WAIT_OBJECT_0+THREAD_READY))
        {   /* either thread initialization failed, or the wait failed */
            goto cleanup_and_fail;
        }
    }
   *pool = worker_pool;
    return 0;

cleanup_and_fail:
    if (shutdown > 0)
    {
        SetEvent(term);
        for (i = 0, n = worker_pool->ActiveThreads; i < n; ++i)
        {
            PostQueuedCompletionStatus(iocp, 0, CORE__ASYNCIO_COMPLETION_KEY_SHUTDOWN, NULL);
        }
        WaitForMultipleObjects(worker_pool->ActiveThreads, worker_pool->OSThreadHandle, TRUE, INFINITE);
        for (i = 0, n = worker_pool->ActiveThreads; i < n; ++i)
        {
            CloseHandle(worker_pool->WorkerError[i]);
            CloseHandle(worker_pool->WorkerReady[i]);
            CloseHandle(worker_pool->OSThreadHandle[i]);
        }
    }
    if (iocp != NULL) CloseHandle(iocp);
    if (term != NULL) CloseHandle(term);
   *pool = NULL; 
    return -1;
}

CORE_API(void)
CORE_TerminateIoWorkerPool
(
    struct _CORE_ASYNCIO_WORKER_POOL *pool
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

CORE_API(struct _CORE_ASYNCIO_REQUEST*)
CORE_InitAsyncIoRequest
(
    struct _CORE_ASYNCIO_REQUEST_POOL *request_pool, 
    CORE_ASYNCIO_REQUEST_INIT                 *init
)
{
    struct _CORE_ASYNCIO_REQUEST *req = NULL;

    if (init->RequestComplete == NULL)
    {   assert(init->RequestComplete != NULL);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (init->RequestDataSize > CORE_ASYNCIO_REQUEST_MAX_DATA)
    {   assert(init->RequestDataSize <= CORE_ASYNCIO_REQUEST_MAX_DATA);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if ((req = CORE__AsyncIoAcquireRequest(request_pool)) != NULL)
    {   /* initialize the fields of the request object */
        req->CompleteCallback = init->RequestComplete;
        req->Handle           = init->RequestHandle;
        req->UserContext      = init->RequestContext;
        CopyMemory(req->Data, init->RequestData.Data, init->RequestDataSize);
        return req;
    }
    else
    {   /* no I/O request objects are available at the current time */
        SetLastError(ERROR_OUT_OF_STRUCTURES);
        return NULL;
    }
}

CORE_API(int)
CORE_SubmitAsyncIoRequest
(
    struct _CORE_ASYNCIO_REQUEST_POOL *request_pool,
    struct _CORE_ASYNCIO_WORKER_POOL   *worker_pool, 
    CORE_ASYNCIO_REQUEST_INIT                 *init
)
{
    struct _CORE_ASYNCIO_REQUEST *req = NULL;
    if ((req = CORE_InitAsyncIoRequest(request_pool, init)) != NULL)
    {   /* the request was successfully acquired and initialized - submit it */
        return CORE__AsyncIoSubmitRequest(worker_pool, req);
    }
    else
    {   /* either the pool has no available requests, or the request data is invalid */
        return -1;
    }
}

#endif /* CORE_ASYNCIO_IMPLEMENTATION */

