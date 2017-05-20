/*
 * CORE_task.h: A single-file library for executing compute tasks on a thread pool.
 * This module is only available for x86-64 platforms.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_TASK_H__
#define __CORE_TASK_H__

#if !defined(_M_X64) && !defined(_M_AMD64)
    #error The CORE_task module supports 64-bit builds only due to requiring 64-bit atomic support.
#endif

/* @summary #define CORE_TASK_NO_PROFILER to disable built-in Concurrency Visualizer SDK support.
 */
#ifndef CORE_TASK_NO_PROFILER
#include "cvmarkers.h"
#else
struct _CV_SPAN;
struct _CV_PROVIDER;
struct _CV_MARKERSERIES;
#endif

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* @summary Define the size of a pointer, in bytes, on the target platform. Only 64-bit platforms are supported.
 */
#ifndef CORE_TASK_POINTER_SIZE
#define CORE_TASK_POINTER_SIZE            8
#endif

/* @summary Define the size of a single cacheline on the runtime target platform.
 */
#ifndef CORE_TASK_L1_CACHELINE_SIZE
#define CORE_TASK_L1_CACHELINE_SIZE       64
#endif

/* @summary Indicate that a type or field should be aligned to a cacheline boundary.
 */
#ifndef CORE_TASK_CACHELINE_ALIGN
#define CORE_TASK_CACHELINE_ALIGN         __declspec(align(CORE_TASK_L1_CACHELINE_SIZE))
#endif

/* @summary Define various constants used to configure the task scheduler.
 * CORE_TASK_ID_INVALID    : Returned from API functions when a task could not be created.
 * CORE_MIN_TASK_POOLS     : The minimum allowable number of task pools. Each thread that interacts with the task system has an associated task pool.
 * CORE_MAX_TASK_POOLS     : The maximum allowable number of task pools. Each thread that interacts with the task system has an associated task pool.
 * CORE_MIN_TASKS_PER_POOL : The minimum allowable number of simultaneously active tasks per-task pool.
 * CORE_MAX_TASKS_PER_POOL : The maximum allowable number of simultaneously active tasks per-task pool.
 * CORE_MAX_TASK_DATA_BYTES: The maximum amount of data that can be stored with a task.
 * CORE_MAX_TASK_PERMITS   : The maximum number of tasks a task can permit to run.
 */
#ifndef CORE_TASK_CONSTANTS
#define CORE_TASK_CONSTANTS
#define CORE_INVALID_TASK_ID              0x7FFFFFFFUL
#define CORE_MIN_TASK_POOLS               1
#define CORE_MAX_TASK_POOLS               4096
#define CORE_MIN_TASKS_PER_POOL           2
#define CORE_MAX_TASKS_PER_POOL           65536
#define CORE_MAX_TASK_DATA_BYTES          48
#define CORE_MAX_TASK_PERMITS             14
#define CORE_TASK_ID_MASK_INDEX           0x0000FFFFUL
#define CORE_TASK_ID_MASK_POOL            0x0FFF0000UL
#define CORE_TASK_ID_MASK_TYPE            0x10000000UL
#define CORE_TASK_ID_MASK_VALID           0x80000000UL
#define CORE_TASK_ID_SHIFT_INDEX          0
#define CORE_TASK_ID_SHIFT_POOL           16
#define CORE_TASK_ID_SHIFT_TYPE           28
#define CORE_TASK_ID_SHIFT_VALID          31
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
 t wi*/
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

/* @summary Emit a Concurrency Visualizer event to the trace session.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param fmt The printf-style format string.
 * @param ... Substitution arguments for the format string.
 */
#ifndef CORE_TaskProfilerEvent
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskProfilerEvent(env, fmt, ...)
#else
#define CORE_TaskProfilerEvent(env, fmt, ...)                                  \
    CvWriteAlertW((env)->Profiler->MarkerSeries, _T(fmt), __VA_ARGS__)
#endif
#endif

/* @summary Indicate the start of a labeled time period within the Concurrency Visualizer trace session. Note that spans cannot be nested within the same thread.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param span The CORE_TASK_PROFILER_SPAN representing the interval being labeled.
 * @param fmt The printf-style format string.
 * @param ... Substitution arguments for the format string.
 */
#ifndef CORE_TaskProfilerSpanEnter
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskProfilerSpanEnter(env, span, fmt, ...)
#else
#define CORE_TaskProfilerSpanEnter(env, span, fmt, ...)                        \
    CvEnterSpanW((env)->Profiler->MarkerSeries, &(span).CvSpan, _T(fmt), __VA_ARGS__)
#endif
#endif

/* @summary Indicate the end of a labeled time period within the Concurrency Visualizer trace session.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param span The CORE_TASK_PROFILER_SPAN representing the labeled interval.
 */
#ifndef CORE_TaskProfilerSpanLeave
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskProfilerSpanLeave(env, span)
#else
#define CORE_TaskProfilerSpanLeave(env, span)                                  \
    CvLeaveSpan((span).CvSpan)
#endif
#endif

/* @summary Construct a task identifier from its constituient parts.
 * Typically invoked as one of the following:
 * task_id = CORE_MakeTaskId(CORE_TASK_ID_INTERNAL, PoolIndex, SlotIndex, CORE_TASK_ID_VALID)
 * task_id = CORE_MakeTaskId(CORE_TASK_ID_INTERNAL, 0xFFF    , 0xFFFF   , CORE_TASK_ID_INVALID);
 * task_id = CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, PoolIndex, SlotIndex, CORE_TASK_ID_VALID);
 * task_id = CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0xFFF    , 0xFFFF   , CORE_TASK_ID_INVALID);
 * @param _type Specify 1 to identify an internally-completed task or 0 to identify an externally-completed task.
 * @param _pool The zero-based index of the task pool that defined the task.
 * @param _slot The zero-based index of the task data slot allocated to the task within the task pool.
 * @param _valid Specify 1 to create a valid task identifer or 0 to create an invalid task identifier.
 */
#ifndef CORE_MakeTaskId
#define CORE_MakeTaskId(_type, _pool, _slot, _valid)                           \
    ((((_valid) & 0x0001UL) << CORE_TASK_ID_SHIFT_VALID) | (((_type) & 0x0001UL) << CORE_TASK_ID_SHIFT_TYPE) | (((_pool) & 0x0FFFUL) << CORE_TASK_ID_SHIFT_POOL) | (((_slot) & 0xFFFFUL) << CORE_TASK_ID_SHIFT_INDEX))
#endif

/* @summary Check whether a task ID is valid.
 * @param _id The CORE_TASK_ID to check.
 */
#ifndef CORE_TaskIdValid
#define CORE_TaskIdValid(_id)                                                  \
    (((_id) & CORE_TASK_ID_MASK_VALUE) != 0)
#endif

/* @summary Check whether a task ID represents an externally-completed task.
 * @param _id The CORE_TASK_ID to check.
 */
#ifndef CORE_TaskIdExternal
#define CORE_TaskIdExternal(_id)                                               \
    (((_id) & CORE_TASK_ID_MASK_TYPE) == 0)
#endif

/* @summary Check whether a task ID represents an internally-completed task.
 * @param _id The CORE_TASK_ID to check.
 */
#ifndef CORE_TaskIdInternal
#define CORE_TaskIdInternal(_id)                                               \
    (((_id) & CORE_TASK_ID_MASK_TYPE) != 0)
#endif

/* @summary Extract the index of the task pool from a task identifier.
 * @param _id The CORE_TASK_ID to examine.
 * @return The zero-based index of the task pool that created the task.
 */
#ifndef CORE_TaskPoolIndex
#define CORE_TaskPoolIndex(_id)                                                \
    (((_id) & CORE_TASK_ID_MASK_POOL) >> CORE_TASK_ID_SHIFT_POOL)
#endif

/* @summary Extract the index of the task within the task pool that owns it.
 * @param _id The CORE_TASK_ID to examine.
 * @return The zero-based index of the task within its pool.
 */
#ifndef CORE_TaskIndexInPool
#define CORE_TaskIndexInPool(_id)                                              \
    (((_id) & CORE_TASK_ID_MASK_INDEX) >> CORE_TASK_ID_SHIFT_INDEX)
#endif

/* Forward-declare types exported by the library */
struct _CORE_TASK_CPU_INFO;
struct _CORE_TASK_PROFILER;
struct _CORE_TASK_PROFILER_SPAN;
struct _CORE_TASK_POOL;
struct _CORE_TASK_POOL_INIT;
struct _CORE_TASK_POOL_STORAGE;
struct _CORE_TASK_POOL_STORAGE_INIT;
struct _CORE_TASK_WORKER_POOL;
struct _CORE_TASK_WORKER_POOL_INIT;
struct _CORE_TASK_STEAL_QUEUE;
struct _CORE_TASK_FENCE;

/* @summary Task identifiers are opaque 32-bit integers.
 */
typedef uint32_t CORE_TASK_ID;

/* @summary Define the signature for the entry point of a task.
 * @param task_id The identifier of the task being executed.
 * @param task_args A pointer to the task-local data buffer used to store task parameters specified when the task was defined.
 * @param task_env The task execution environment, which can be used to define additional tasks or to allocate scratch memory.
 */
typedef void   (*CORE_TaskMain_Func)
(
    CORE_TASK_ID                      task_id, 
    void                           *task_args
);

/* @summary Define information describing the CPU layout of the host system.
 */
typedef struct _CORE_TASK_CPU_INFO {
    uint32_t                         NumaNodes;             /* The number of NUMA nodes in the host system. */
    uint32_t                         PhysicalCPUs;          /* The total number of physical CPUs in the host system. */
    uint32_t                         PhysicalCores;         /* The total number of physical cores across all CPUs. */
    uint32_t                         HardwareThreads;       /* The total number of hardware threads across all CPUs. */
    uint32_t                         ThreadsPerCore;        /* The number of hardware threads per physical core. */
    uint32_t                         CacheSizeL1;           /* The total size of the smallest L1 data cache, in bytes. */
    uint32_t                         CacheLineSizeL1;       /* The size of a single cache line in the L1 data cache, in bytes. */
    uint32_t                         CacheSizeL2;           /* The total size of the smallest L2 data cache, in bytes. */
    uint32_t                         CacheLineSizeL2;       /* The size of a single cache line in the L2 data cache, in bytes. */
    int32_t                          PreferAMD;             /* Non-zero if AMD implementations are preferred. */
    int32_t                          PreferIntel;           /* Non-zero if Intel implementations are preferred. */
    int32_t                          IsVirtualMachine;      /* Non-zero if the system appears to be a virtual machine. */
    char                             VendorName[16];        /* The nul-terminated CPUID vendor string. */
} CORE_TASK_CPU_INFO;

/* @summary Define the data associated with a task system profiler instance.
 */
typedef struct _CORE_TASK_PROFILER {
    struct _CV_PROVIDER             *Provider;              /* The provider instance. */
    struct _CV_MARKERSERIES         *MarkerSeries;          /* The marker series object. */
} CORE_TASK_PROFILER;

/* @summary Define the data representing a time span within the task system profiler.
 */
typedef struct _CORE_TASK_PROFILER_SPAN {
    struct _CV_SPAN                 *CvSpan;                /* The Concurrency Visualizer SDK object representing the time span. */
} CORE_TASK_PROFILER_SPAN;

/* @summary Define the data used to configure a task pool, which allows tasks to be defined by the owning thread.
 */
typedef struct _CORE_TASK_POOL_INIT {
    uint32_t                         PoolId;                /* One of _CORE_TASK_POOL_ID, or any application-defined value unique within the task scheduler identifying the type of task pool. */
    uint32_t                         PoolCount;             /* The number of task pools of this type that will be used by the application. */
    uint32_t                         StealThreshold;        /* The number of tasks that can be defined within the pool without executing a task before the pool begins to post steal notifications. */
    uint32_t                         MaxActiveTasks;        /* The maximum number of simultaneously active tasks that can be defined within each pool of this type. This value must be a power-of-two. */
} CORE_TASK_POOL_INIT;

/* @summary Define the data representing a fixed set of task data pools (used for defining tasks).
 */
typedef struct _CORE_TASK_POOL_STORAGE {
    struct _CORE_TASK_POOL         **TaskPoolList;          /* Pointers to each task pool object. */
    struct _CORE_TASK_STEAL_QUEUE   *StealQueue;            /* A blocking MPMC queue to which pools post notifications indicating that they have tasks available to steal. */
    uint32_t                         TaskPoolCount;         /* The total number of task pool objects. */
    uint32_t                         PoolTypeCount;         /* The number of task pool types defined within the storage array. */
    uint32_t                        *PoolTypeIds;           /* An array of PoolTypeCount values, where each value specifies the task pool ID. */
    struct _CORE_TASK_POOL         **PoolFreeList;          /* An array of PoolTypeCount pointers to the free list for each task pool ID. */
    CRITICAL_SECTION                *PoolFreeListLocks;     /* An array of PoolTypeCount critical sections protecting the free list for each task pool ID. */
    void                            *MemoryStart;           /* A pointer to the start of the memory block allocated for the storage array. */
    uint64_t                         MemorySize;            /* The total size of the memory block allocated for the storage array. */
} CORE_TASK_POOL_STORAGE;

/* @summary Define the attributes used to initialize a task pool storage container.
 */
typedef struct _CORE_TASK_POOL_STORAGE_INIT {
    void                            *MemoryStart;           /* A pointer to the start of the memory block allocated for the storage array. */
    uint64_t                         MemorySize;            /* The total size of the memory block allocated for the storage array. */
    struct _CORE_TASK_POOL_INIT     *TaskPoolTypes;         /* An array of structures defining the types of task pools used by the application. */
    uint32_t                         PoolTypeCount;         /* The number of _CORE_TASK_POOL_INIT structures in TaskPoolTypes. */
    uint32_t                         StealQueueCapacity;    /* The maximum number of notifications that can be posted to the steal pool. */
} CORE_TASK_POOL_STORAGE_INIT;

/* @summary Define some identifiers that may be passed to CORE_MakeTaskId for the _type argument to make the code more readable.
 */
typedef enum _CORE_TASK_ID_TYPE {
    CORE_TASK_ID_EXTERNAL                             =  0, /* The task is completed by an external event, such as an I/O operation. */
    CORE_TASK_ID_INTERNAL                             =  1, /* The task is completed internally by executing the task entry point. */
} CORE_TASK_ID_TYPE;

/* @summary Define some identifiers that may be passed to CORE_MakeTaskId for the _valid argument to make the code more readable.
 */
typedef enum _CORE_TASK_ID_VALIDITY {
    CORE_TASK_ID_INVALID                              =  0, /* The task identifier is not valid. */
    CORE_TASK_ID_VALID                                =  1, /* The task identifier is valid. */
} CORE_TASK_ID_VALIDITY;

/* @summary Define some well-known task pool identifiers.
 */
typedef enum _CORE_TASK_POOL_ID {
    CORE_TASK_POOL_ID_MAIN                            =  0, /* The identifier of the task pool associated with the main application thread. */
    CORE_TASK_POOL_ID_WORKER                          =  1, /* The identifier of the task pool associated with each worker thread. */
    CORE_TASK_POOL_ID_USER                            =  2, /* The identifier of the first custom application task pool. */
} CORE_TASK_POOL_ID;

/* @summary Define the possible validation codes that can be generated by CORE_ValidateTaskPoolConfiguration.
 */
typedef enum _CORE_TASK_POOL_VALIDATION_RESULT {
    CORE_TASK_POOL_VALIDATION_RESULT_SUCCESS          =  0, /* No issue was detected. */
    CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_POOLS   =  1, /* The task pool type specifies too many pools, or the sum of pool counts across all types exceeds the maximum number of pools. */
    CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_TASKS   =  2, /* The task pool type specifies too many concurrently active tasks in MaxActiveTasks. */
    CORE_TASK_POOL_VALIDATION_RESULT_TOO_FEW_TASKS    =  3, /* The task pool type specifies too few tasks for the MaxActiveTasks value. */
    CORE_TASK_POOL_VALIDATION_RESULT_NOT_POWER_OF_TWO =  4, /* The MaxActiveTasks value is not a power-of-two. */
    CORE_TASK_POOL_VALIDATION_RESULT_DUPLICATE_ID     =  5, /* The same PoolId is used for more than one CORE_TASK_POOL_INIT structure. */
    CORE_TASK_POOL_VALIDATION_RESULT_INVALID_USAGE    =  6, /* The PoolUsage field is invalid. */
    CORE_TASK_POOL_VALIDATION_RESULT_NO_WORKER_ID     =  7, /* None of the types supplied specify PoolId CORE_TASK_POOL_ID_WORKER. */
} CORE_TASK_POOL_VALIDATION_RESULT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Determine basic attributes of the host system CPU layout.
 * This function allocates a small amount of scratch memory and frees it before returning. Do not call this function in performance-critical code.
 * @param cpu_info On return, this location is updated with information about the host system CPU layout.
 * @return Zero if the function executes successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_QueryHostCpuInfo
(
    CORE_TASK_CPU_INFO *cpu_info
);

/* @summary Initialize a task scheduler profiler object. 
 * Profile data can be captured and displayed using the Visual Studio Concurrency Visualizer plugin.
 * The task profiler GUID is {349CE0E9-6DF5-4C25-AC5B-C84F529BC0CE}.
 * @param profiler The CORE_TASK_PROFILER object to initialize.
 * @param application_name A nul-terminated string specifying a string used to identify application markers.
 * @return Zero if the profiler is successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateTaskProfiler
(
    CORE_TASK_PROFILER  *profiler, 
    WCHAR const *application_name
);

/* @summary Free resources associated with a task scheduler profiler object.
 * @param profiler The CORE_TASK_PROFILER object to delete.
 */
CORE_API(void)
CORE_DeleteTaskProfiler
(
    CORE_TASK_PROFILER *profiler
);

/* @summary Inspect one or more task pool configurations and perform validation checks against them.
 * @param type_configs An array of type_count CORE_TASK_POOL_INIT structures defining the configuration for each type of task pool.
 * @param type_results An array of type_count integers where the validation results for each CORE_TASK_POOL_INIT will be written.
 * @param type_count The number of elements in the type_configs and type_results arrays.
 * @param global_result On return, any non type-specific validation error is written to this location.
 * @return Zero if the task pool configurations are all valid, or -1 if one or more problems were detected.
 */
CORE_API(int)
CORE_ValidateTaskPoolConfiguration
(
    CORE_TASK_POOL_INIT *type_configs,
    int32_t             *type_results, 
    uint32_t               type_count, 
    int32_t            *global_result
);

/* @summary Determine the amount of memory required to initialize a task pool storage object with the given configuration.
 * @param type_configs An array of type_count CORE_TASK_POOL_INIT structures defining the configuration for each task pool type.
 * @param type_count The number of elements in the type_configs array.
 * @return The minimum number of bytes required to successfully initialize a task pool storage object with the given configuration.
 */
CORE_API(size_t)
CORE_QueryTaskPoolStorageMemorySize
(
    CORE_TASK_POOL_INIT *type_configs, 
    uint32_t               type_count
);

/* @summary Initialize a task pool storage blob.
 * @param storage The CORE_TASK_POOL_STORAGE to initialize.
 * @param init Data used to configure the storage pool.
 * @return Zero if the storage object is successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateTaskPoolStorage
(
    CORE_TASK_POOL_STORAGE   *storage, 
    CORE_TASK_POOL_STORAGE_INIT *init
);

/* @summary Free all resources associated with a task pool storage object.
 * @param storage The CORE_TASK_POOL_STORAGE object to delete.
 */
CORE_API(void)
CORE_DeleteTaskPoolStorage
(
    CORE_TASK_POOL_STORAGE *storage
);

/* @summary Acquire a task pool and bind it to the calling thread.
 * This function is safe to call from multiple threads simultaneously.
 * This function should not be called from performance-critical code, as it may block.
 * @param storage The CORE_TASK_POOL_STORAGE object from which the pool should be acquired.
 * @param pool_type_id One of _CORE_TASK_POOL_ID, or an application-defined value specifying the pool type to acquire.
 * @return A pointer to the task pool object, or NULL if no pool of the specified type could be acquired.
 */
CORE_API(struct _CORE_TASK_POOL*)
CORE_AcquireTaskPool
(
    CORE_TASK_POOL_STORAGE *storage, 
    uint32_t           pool_type_id
);

/* @summary Release a task pool back to the storage object it was allocated from.
 * @param pool The task pool object to release.
 */
CORE_API(void)
CORE_ReleaseTaskPool
(
    struct _CORE_TASK_POOL *pool
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_TASK_H__ */

#ifdef CORE_TASK_IMPLEMENTATION

/* @summary Define the amount of padding, in bytes, used for a CORE__TASK_SEMAPHORE object.
 */
#ifndef CORE__TASK_SEMAPHORE_PADDING_SIZE
#define CORE__TASK_SEMAPHORE_PADDING_SIZE            (CORE_TASK_L1_CACHELINE_SIZE-sizeof(int32_t)-sizeof(HANDLE))
#endif

/* @summary Define the amount of padding, in bytes, separating the enqueue or dequeue index from adjacent data in an MPMC bounded queue.
 */
#ifndef CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
#define CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX     (CORE_TASK_L1_CACHELINE_SIZE-sizeof(uint32_t))
#endif

/* @summary Define the amount of padding, in bytes, separating the shared data from adjacent data in an MPMC bounded queue.
 */
#ifndef CORE__TASK_FREE_QUEUE_PADDING_SIZE_SHARED
#define CORE__TASK_FREE_QUEUE_PADDING_SIZE_SHARED    (CORE_TASK_L1_CACHELINE_SIZE-sizeof(void*)-sizeof(uint32_t)-sizeof(uint32_t)-sizeof(void*)-sizeof(size_t))
#endif

/* @summary Define the amount of padding, in bytes, separating the enqueue or dequeue index from adjacent data in an bounded ready-to-run task queue.
 */
#ifndef CORE__TASK_WORK_QUEUE_PADDING_SIZE_INDEX
#define CORE__TASK_WORK_QUEUE_PADDING_SIZE_INDEX     (CORE_TASK_L1_CACHELINE_SIZE-sizeof(int64_t))
#endif

/* @summary Define the amount of padding, in bytes, separating the shared data from adjacent data in an bounded ready-to-run task queue.
 */
#ifndef CORE__TASK_WORK_QUEUE_PADDING_SIZE_SHARED
#define CORE__TASK_WORK_QUEUE_PADDING_SIZE_SHARED    (CORE_TASK_L1_CACHELINE_SIZE-sizeof(void*)-sizeof(uint32_t)-sizeof(uint32_t)-sizeof(void*)-sizeof(size_t))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an instance of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 */
#ifndef CORE__TaskAllocationSizeType
#define CORE__TaskAllocationSizeType(_type)                                    \
    ((sizeof(_type)) + (__alignof(_type)-1))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an array of instances of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 * @param _count The number of elements in the array.
 */
#ifndef CORE__TaskAllocationSizeArray
#define CORE__TaskAllocationSizeArray(_type, _count)                           \
    ((sizeof(_type) * (_count)) + (__alignof(_type)-1))
#endif

/* @summary Allocate host memory with the correct size and alignment for an instance of a given type from a memory arena.
 * @param _arena The CORE__TASK_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__TaskMemoryArenaAllocateType
#define CORE__TaskMemoryArenaAllocateType(_arena, _type)                       \
    ((_type*) CORE__TaskMemoryArenaAllocateHost((_arena), sizeof(_type), __alignof(_type)))
#endif

/* @summary Allocate memory with the correct size and alignment for an array of instance of a given type from a memory arena.
 * @param _arena The CORE__TASK_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the array.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__TaskMemoryArenaAllocateArray
#define CORE__TaskMemoryArenaAllocateArray(_arena, _type, _count)              \
    ((_type*) CORE__TaskMemoryArenaAllocateHost((_arena), sizeof(_type) * (_count), __alignof(_type)))
#endif

/* @summary Define the data associated with an internal memory arena allocator.
 */
typedef struct _CORE__TASK_ARENA {
    uint8_t                              *BaseAddress;            /* The base address of the memory range. */
    size_t                                MemorySize;             /* The size of the memory block, in bytes. */
    size_t                                NextOffset;             /* The offset of the next available address. */
} CORE__TASK_ARENA;

/* @summary Define the type used to mark a location within a memory arena.
 */
typedef size_t CORE__TASK_ARENA_MARKER;

/* @summary Define the data associated with a single item in an MPMC concurrent queue.
 * These items store the zero-based index of a free task slot within the task pool, or the zero-based index of a task pool that has tasks available to steal.
 */
typedef struct _CORE__TASK_MPMC_CELL {
    uint32_t                              Sequence;               /* The sequence number assigned to the cell. */
    uint32_t                              Index;                  /* The value stored in the cell. */
} CORE__TASK_MPMC_CELL;

/* @summary Define the data associated with a semaphore guaranteed to stay in userspace unless a thread needs to be downed or woken.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_SEMAPHORE {
    #define PAD_SIZE                      CORE__TASK_SEMAPHORE_PADDING_SIZE
    HANDLE                                Semaphore;              /* The operating system semaphore object. */
    int32_t                               Count;                  /* The current count. */
    uint8_t                               Pad[PAD_SIZE];          /* Padding out to a cacheline boundary. */
    #undef  PAD_SIZE
} CORE__TASK_SEMAPHORE;

/* @summary Define the data associated with a fixed-size, MPMC concurrent queue. The queue capacity must be a power-of-two.
 * The MPMC queue is used in an MPSC fashion to return completed task indices to the owning task pool.
 * Threads that complete tasks are the producers, and the thread that owns the task pool is the consumer.
 * The queue capacity matches the task pool capacity.
 * The queue implementation (C++) is originally by Dmitry Vyukov and is available here:
 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_FREE_QUEUE {
    #define PAD_SHARED                    CORE__TASK_FREE_QUEUE_PADDING_SIZE_SHARED
    #define PAD_ENQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    #define PAD_DEQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    CORE__TASK_MPMC_CELL                 *Storage;                /* Storage for queue items. Fixed-size, power-of-two capacity. */
    uint32_t                              StorageMask;            /* The mask used to map EnqueuePos and DequeuePos into the storage array. */
    uint32_t                              Capacity;               /* The maximum number of items that can be stored in the queue. */
    void                                 *MemoryStart;            /* The pointer to the start of the allocated memory block. */
    size_t                                MemorySize;             /* The size of the allocated memory block, in bytes. */
    uint8_t                               Pad0[PAD_SHARED];       /* Padding separating shared data from producer-only data. */
    uint32_t                              EnqueuePos;             /* A monotonically-increasing integer representing the position of the next enqueue operation. */
    uint8_t                               Pad1[PAD_ENQUEUE];      /* Padding separating the producer-only data from the consumer-only data. */
    uint32_t                              DequeuePos;             /* A monotonically-increasing integer representing the position of the next dequeue operation. */
    uint8_t                               Pad2[PAD_DEQUEUE];      /* Padding separating the consumer-only data from adjacent data. */
    #undef  PAD_SHARED
    #undef  PAD_ENQUEUE
    #undef  PAD_DEQUEUE
} CORE__TASK_FREE_QUEUE;

/* @summary Define the data associated with a fixed-size double-ended concurrent queue. The deque capacity must be a power-of-two.
 * The deque stores ready-to-run task identifiers. It supports operations Push, Take and Steal. Push and Take operate in a LIFO manner, while Steal operates in a FIFO manner.
 * The thread that owns the deque (and task pool) can perform Push and Take operations. Other threads may perform Steal operations.
 * The PublicPos and PrivatePos are 64-bit values to avoid overflow over any reasonable time period (1000+ years assuming 1M tasks/tick at 240Hz.) Otherwise, they must be periodically reset to zero. 
 * The deque capacity matches the task pool capacity.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_WORK_QUEUE {
    #define PAD_SHARED                    CORE__TASK_WORK_QUEUE_PADDING_SIZE_SHARED
    #define PAD_PUBLIC                    CORE__TASK_WORK_QUEUE_PADDING_SIZE_INDEX
    #define PAD_PRIVATE                   CORE__TASK_WORK_QUEUE_PADDING_SIZE_INDEX
    CORE_TASK_ID                         *Storage;                /* Storage for queue items (ready-to-run task identifiers.) Fixed-size, power-of-two capacity. */
    uint32_t                              StorageMask;            /* The mask used to map PublicPos and PrivatePos into the storage array. */
    uint32_t                              Capacity;               /* The maximum number of items that can be stored in the queue. */
    void                                 *MemoryStart;            /* The pointer to the start of the allocated memory block. */
    size_t                                MemorySize;             /* The size of the allocated memory block, in bytes. */
    uint8_t                               Pad0[PAD_SHARED];       /* Padding separating shared data from the public-only end of the queue. */
    int64_t                               PublicPos;              /* A monotonically-increasing integer representing the public end of the dequeue, updated by Steal operations. */
    uint8_t                               Pad1[PAD_PUBLIC];       /* Padding separating the public-only data from the private-only data. */
    int64_t                               PrivatePos;             /* A monotonically-increasing integer representing the private end of the dequeue, updated by Push and Take operations. */
    uint8_t                               Pad2[PAD_PRIVATE];      /* Padding separating the private-only data from adjacent data. */
    #undef  PAD_SHARED
    #undef  PAD_PUBLIC
    #undef  PAD_PRIVATE
} CORE__TASK_WORK_QUEUE;

/* @summary Define the data associated with a fixed-size, MPMC concurrent queue. The queue capacity must be a power-of-two.
 * The MPMC queue is used in an MPMC fashion to store notifications that a given task pool has tasks available to steal.
 * The queue has an associated semaphore, so that threads with nothing to do can wait nicely until a task is potentially available.
 * Aside from the blocking and semaphore behavior, the implementation is identical to the CORE__TASK_FREE_QUEUE.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_STEAL_QUEUE {
    #define PAD_SHARED                    CORE__TASK_FREE_QUEUE_PADDING_SIZE_SHARED
    #define PAD_ENQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    #define PAD_DEQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    CORE__TASK_SEMAPHORE                  Semaphore;              /* The semaphore used to put threads to sleep or wake them up. */
    CORE__TASK_MPMC_CELL                 *Storage;                /* Storage for queue items. Fixed-size, power-of-two capacity. */
    uint32_t                              StorageMask;            /* The mask used to map EnqueuePos and DequeuePos into the storage array. */
    uint32_t                              Capacity;               /* The maximum number of items that can be stored in the queue. */
    void                                 *MemoryStart;            /* The pointer to the start of the allocated memory block. */
    size_t                                MemorySize;             /* The size of the allocated memory block, in bytes. */
    uint8_t                               Pad0[PAD_SHARED];       /* Padding separating shared data from producer-only data. */
    uint32_t                              EnqueuePos;             /* A monotonically-increasing integer representing the position of the next enqueue operation. */
    uint8_t                               Pad1[PAD_ENQUEUE];      /* Padding separating the producer-only data from the consumer-only data. */
    uint32_t                              DequeuePos;             /* A monotonically-increasing integer representing the position of the next dequeue operation. */
    uint8_t                               Pad2[PAD_DEQUEUE];      /* Padding separating the consumer-only data from adjacent data. */
    #undef  PAD_SHARED
    #undef  PAD_ENQUEUE
    #undef  PAD_DEQUEUE
} CORE__TASK_STEAL_QUEUE;

/* @summary Define the data tracked internally for each task. Aligned to and limited to one cacheline.
 * Multiple threads may try to simultaneously read and write WaitCount, WorkCount, PermitCount and PermitIds.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_DATA {
    #define NUM_DATA                      CORE_MAX_TASK_DATA_BYTES
    #define NUM_PERMITS                   CORE_MAX_TASK_PERMITS
    int32_t                               WaitCount;              /* The number of tasks that must complete before this task can run. */
    CORE_TASK_ID                          ParentId;               /* The identifier of the parent task, or CORE_INVALID_TASK_ID. */
    CORE_TaskMain_Func                    TaskMain;               /* The function to call to execute the task workload. */
    uint8_t                               TaskData[NUM_DATA];     /* Argument data to pass to the task entrypoint. */
    int32_t                               WorkCount;              /* The number of outstanding work items (one for the task, plus one for each child task.) */
    int32_t                               PermitCount;            /* The number of tasks that this task permits to run (the number of valid permits.) */
    CORE_TASK_ID                          PermitIds[NUM_PERMITS]; /* The task ID of each task permitted to run when this task completes. */
    #undef  NUM_PERMITS
    #undef  NUM_DATA
} CORE__TASK_DATA;

/* @summary Define the data associated with a fixed-size, preallocated pool of tasks.
 * The task pool is owned by a single thread. This thread can define tasks within the pool.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_POOL {
    CORE__TASK_FREE_QUEUE                 FreeTasks;              /* The MPSC queue of available task slots. The thread that owns the pool is the consumer; threads that execute tasks are the producers. */
    CORE__TASK_WORK_QUEUE                 ReadyTasks;             /* The deque of ready-to-run task IDs. The thread that owns the pool may Push and Take; other threads may only Steal. */
    CORE__TASK_SEMAPHORE                  Semaphore;              /* A semaphore used to sleep threads when there are no task data slots available. */
    struct _CORE_TASK_POOL_STORAGE       *Storage;                /* The CORE_TASK_POOL_STORAGE that owns this pool. */
    struct _CORE_TASK_POOL               *NextPool;               /* Pointer to the next pool in the free list. */
    struct _CORE__TASK_DATA              *TaskData;               /* Storage for the task data items allocated to the pool. */
    HANDLE                               *WorkerSet;              /* An array of WorkerCount I/O completion port handles used to wake worker threads when tasks are available to steal. */
    uint32_t                              WorkerCount;            /* The number of worker threads that can receive notifications when tasks are available to steal from this pool. */
    uint32_t                              NextWorker;             /* The zero-based index of the next worker thread to wake when tasks are available to steal from this pool. */
    uint32_t                              Capacity;               /* The capacity of the pool, in tasks. This value is always a power of two. */
    uint32_t                              ThreadId;               /* The operating system identifier of the thread that owns this task pool. */
    uint32_t                              PoolIndex;              /* The zero-based index of the pool within the CORE_TASK_POOL_STORAGE. */
    uint32_t                              PoolId;                 /* One of _CORE_TASK_POOL_ID acting as an identifier for the pool type. */
    uint32_t                              PoolUsage;              /* One or more bitwise OR'd _CORE_TASK_POOL_USAGE flags. */
    int32_t                               LastError;              /* One of _CORE_TASK_DEFINITION_ERROR */
} CORE__TASK_POOL;

/* @summary Define the supported memory ordering constraints for atomic operations.
 */
typedef enum _CORE__TASK_ATOMIC_ORDERING {
    CORE__TASK_ATOMIC_ORDERING_RELAXED   = 0,                /* No inter-thread ordering constraints. */
    CORE__TASK_ATOMIC_ORDERING_ACQUIRE   = 1,                /* Imposes a happens-before constraint from a store-release. Subsequent loads are not hoisted. */
    CORE__TASK_ATOMIC_ORDERING_RELEASE   = 2,                /* Imposes a happens-before constraint to a load-acquire. Preceeding stores are not sunk. */
    CORE__TASK_ATOMIC_ORDERING_ACQ_REL   = 3,                /* Combine ACQUIRE and RELEASE semantics. Subsequent loads are not hoisted, preceeding stores are not sunk. */
    CORE__TASK_ATOMIC_ORDERING_SEQ_CST   = 4,                /* Enforce total ordering (sequential consistency) with all other SEQ_CST operations. */
} CORE__TASK_ATOMIC_ORDERING;

/* @summary Initialize a memory arena allocator around an externally-managed memory block.
 * @param arena The memory arena allocator to initialize.
 * @param memory A pointer to the start of the memory block to sub-allocate from.
 * @param memory_size The size of the memory block, in bytes.
 */
static void
CORE__TaskInitMemoryArena
(
    CORE__TASK_ARENA *arena, 
    void            *memory,
    size_t      memory_size
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
CORE__TaskMemoryArenaAllocateHost
(
    CORE__TASK_ARENA *arena, 
    size_t             size, 
    size_t        alignment
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
static CORE__TASK_ARENA_MARKER
CORE__TaskMemoryArenaMark
(
    CORE__TASK_ARENA *arena
)
{
    return arena->NextOffset;
}

/* @summary Roll back a memory arena allocator to a given point in time.
 * @param arena The memory arena allocator to roll back.
 * @param marker A marker obtained by a prior call to the ArenaMark function, or 0 to invalidate all existing allocations made from the arena.
 */
static void
CORE__TaskResetMemoryArenaToMarker
(
    CORE__TASK_ARENA        *arena,
    CORE__TASK_ARENA_MARKER marker
)
{   assert(marker <= arena->NextOffset);
    arena->NextOffset = marker;
}

/* @summary Invalidate all allocations made from a memory arena.
 * @param arena The memory arena allocator to roll back.
 */
static void
CORE__TaskResetMemoryArena
(
    CORE__TASK_ARENA *arena
)
{
    arena->NextOffset = 0;
}

/* @summary Atomically load a 32-bit value from a memory location.
 * @param address The memory location to read. This address must be 32-bit aligned.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 * @return The 32-bit value stored at the specified memory location.
 */
static int32_t
CORE__TaskAtomicLoad_s32
(
    int32_t *address, 
    int        order
)
{   /* on x86 and x86-64 loads of aligned 32-bit values are atomic */
    int32_t v;
    assert((((uintptr_t) address) & 3) == 0 && "address must have 32-bit alignment");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_ACQUIRE:
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { v = *address;
              _ReadWriteBarrier();
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on load");
              v = *address;
              _ReadWriteBarrier();
            } break;
    }
    return v;
}

/* @summary Atomically load a 32-bit value from a memory location.
 * @param address The memory location to read. This address must be 32-bit aligned.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 * @return The 32-bit value stored at the specified memory location.
 */
static uint32_t
CORE__TaskAtomicLoad_u32
(
    uint32_t *address, 
    int         order
)
{   /* on x86 and x86-64 loads of aligned 32-bit values are atomic */
    uint32_t v;
    assert((((uintptr_t) address) & 3) == 0 && "address must have 32-bit alignment");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_ACQUIRE:
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { v = *address;
              _ReadWriteBarrier();
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on load");
              v = *address;
              _ReadWriteBarrier();
            } break;
    }
    return v;
}

/* @summary Atomically load a 64-bit value from a memory location.
 * @param address The memory location to read. This address must be 64-bit aligned.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 * @return The 64-bit value stored at the specified memory location.
 */
static int64_t
CORE__TaskAtomicLoad_s64
(
    int64_t *address, 
    int        order
)
{   /* on x86-64 loads of aligned 64-bit values are atomic */
    int64_t v;
    assert((((uintptr_t) address) & 7) == 0 && "address must have 64-bit alignment");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_ACQUIRE:
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { v = *address;
              _ReadWriteBarrier();
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on load");
              v = *address;
              _ReadWriteBarrier();
            } break;
    }
    return v;
}

/* @summary Atomically store a 32-bit value to a memory location.
 * @param address The memory location to write. This address must be 32-bit aligned.
 * @param value The value to write to the specified memory location.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 */
static void
CORE__TaskAtomicStore_s32
(
    int32_t *address, 
    int32_t    value, 
    int        order
)
{   /* on x86 and x86-64 stores of aligned 32-bit values are atomic */
    assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_RELEASE:
            { _ReadWriteBarrier();
              *address = value;
            } break;
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { _InterlockedExchange((volatile LONG*) address, value);
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on store");
              _ReadWriteBarrier();
              *address = value;
            } break;
    }
}

/* @summary Atomically store a 32-bit value to a memory location.
 * @param address The memory location to write. This address must be 32-bit aligned.
 * @param value The value to write to the specified memory location.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 */
static void
CORE__TaskAtomicStore_u32
(
    uint32_t *address, 
    uint32_t    value, 
    int         order
)
{   /* on x86 and x86-64 stores of aligned 32-bit values are atomic */
    assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_RELEASE:
            { _ReadWriteBarrier();
              *address = value;
            } break;
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { _InterlockedExchange((volatile LONG*) address, (LONG) value);
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on store");
              _ReadWriteBarrier();
              *address = value;
            } break;
    }
}

/* @summary Atomically store a 64-bit value to a memory location.
 * @param address The memory location to write. This address must be 64-bit aligned.
 * @param value The value to write to the specified memory location.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 */
static void
CORE__TaskAtomicStore_s64
(
    int64_t *address, 
    int64_t    value, 
    int        order
)
{   /* on x86-64 stores of aligned 64-bit values are atomic */
    assert((((uintptr_t) address) & 7) == 0 && "address must be 64-bit aligned");
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_RELEASE:
            { _ReadWriteBarrier();
              *address = value;
            } break;
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { _InterlockedExchange64((volatile LONGLONG*) address, value);
            } break;
        default:
            { assert(false && "unsupported memory ordering constraint on store");
              _ReadWriteBarrier();
              *address = value;
            } break;
    }
}

/* @summary Load a 32-bit value from a memory location, add a value to it, and store the updated value back to the memory location as an atomic operation.
 * @param address The memory location containing the current value. This must be a 32-bit aligned address.
 * @param value The value to add (if positive) or subtract (if negative) from the value stored at address.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint. CORE__TASK_ATOMIC_ORDERING_RELEASE not supported.
 * @return The value initially stored at the memory location.
 */
static int32_t
CORE__TaskAtomicFetchAdd_s32
(
    int32_t *address, 
    int32_t    value, 
    int        order
)
{   assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    UNREFERENCED_PARAMETER(order);
    return _InterlockedExchangeAdd((volatile LONG*) address, value);
}

/* @summary Load a 32-bit value from a memory location, add a value to it, and store the updated value back to the memory location as an atomic operation.
 * @param address The memory location containing the current value. This must be a 32-bit aligned address.
 * @param value The value to add (if positive) or subtract (if negative) from the value stored at address.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint. CORE__TASK_ATOMIC_ORDERING_RELEASE not supported.
 * @return The value initially stored at the memory location.
 */
static uint32_t
CORE__TaskAtomicFetchAdd_u32
(
    uint32_t *address, 
    uint32_t    value, 
    int         order
)
{   assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    UNREFERENCED_PARAMETER(order);
    return (uint32_t) _InterlockedExchangeAdd((volatile LONG*) address, (LONG) value);
}

/* @summary Load a 64-bit value from a memory location, add a value to it, and store the updated value back to the memory location as an atomic operation.
 * @param address The memory location containing the current value. This must be a 64-bit aligned address.
 * @param value The value to add (if positive) or subtract (if negative) from the value stored at address.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint. CORE__TASK_ATOMIC_ORDERING_RELEASE not supported.
 * @return The value initially stored at the memory location.
 */
static int64_t
CORE__TaskAtomicFetchAdd_s64
(
    int64_t *address, 
    int64_t    value, 
    int        order
)
{   assert((((uintptr_t) address) & 7) == 0 && "address must be 64-bit aligned");
    UNREFERENCED_PARAMETER(order);
    return _InterlockedExchangeAdd64((volatile LONGLONG*) address, value);
}

/* @summary Load a 32-bit value from a memory location, compare it with an expected value, and if they match, store a new value in the memory location as a single atomic operation.
 * @param address The memory location to load and potentially update. The address must be 32-bit aligned.
 * @param expected A memory location containing the expected value. On return, this memory location contains the original value stored at address.
 * @param desired The value that will be stored to the memory location if the current value matches the expected value.
 * @param weak Specify non-zero to perform a weak compare-exchange, which may sometimes fail even if *address == *expected, but may perform better.
 * @param order_success One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation is successful.
 * @param order_failure One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation fails.
 * @return Non-zero if the value stored at address matched the expected value and was updated to desired, or zero if the value stored at address did not match the expected value.
 */
static int
CORE__TaskAtomicCompareAndSwap_s32
(
    int32_t  *address, 
    int32_t *expected, 
    int32_t   desired, 
    int          weak, 
    int order_success, 
    int order_failure
)
{
    int      success;
    int32_t original;
    int32_t      ex = *expected;
    UNREFERENCED_PARAMETER(weak);
    UNREFERENCED_PARAMETER(order_failure);
    UNREFERENCED_PARAMETER(order_success);
    assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    original = _InterlockedCompareExchange((volatile LONG*) address, desired, ex);
    success  = (original == ex) ? 1 : 0;
   *expected =  original;
    return success;
}

/* @summary Load a 32-bit value from a memory location, compare it with an expected value, and if they match, store a new value in the memory location as a single atomic operation.
 * @param address The memory location to load and potentially update. The address must be 32-bit aligned.
 * @param expected A memory location containing the expected value. On return, this memory location contains the original value stored at address.
 * @param desired The value that will be stored to the memory location if the current value matches the expected value.
 * @param weak Specify non-zero to perform a weak compare-exchange, which may sometimes fail even if *address == *expected, but may perform better.
 * @param order_success One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation is successful.
 * @param order_failure One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation fails.
 * @return Non-zero if the value stored at address matched the expected value and was updated to desired, or zero if the value stored at address did not match the expected value.
 */
static int
CORE__TaskAtomicCompareAndSwap_u32
(
    uint32_t  *address, 
    uint32_t *expected, 
    uint32_t   desired, 
    int           weak, 
    int  order_success, 
    int  order_failure
)
{
    int       success;
    uint32_t original;
    uint32_t      ex = *expected;
    UNREFERENCED_PARAMETER(weak);
    UNREFERENCED_PARAMETER(order_failure);
    UNREFERENCED_PARAMETER(order_success);
    assert((((uintptr_t) address) & 3) == 0 && "address must be 32-bit aligned");
    original = (uint32_t) _InterlockedCompareExchange((volatile LONG*) address, (LONG) desired, (LONG) ex);
    success  = (original == ex) ? 1 : 0;
   *expected =  original;
    return success;
}

/* @summary Load a 64-bit value from a memory location, compare it with an expected value, and if they match, store a new value in the memory location as a single atomic operation.
 * @param address The memory location to load and potentially update. The address must be 64-bit aligned.
 * @param expected A memory location containing the expected value. On return, this memory location contains the original value stored at address.
 * @param desired The value that will be stored to the memory location if the current value matches the expected value.
 * @param weak Specify non-zero to perform a weak compare-exchange, which may sometimes fail even if *address == *expected, but may perform better.
 * @param order_success One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation is successful.
 * @param order_failure One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint to apply if the CAS operation fails.
 * @return Non-zero if the value stored at address matched the expected value and was updated to desired, or zero if the value stored at address did not match the expected value.
 */
static int
CORE__TaskAtomicCompareAndSwap_s64
(
    int64_t  *address, 
    int64_t *expected, 
    int64_t   desired, 
    int          weak, 
    int order_success, 
    int order_failure
)
{
    int      success;
    int64_t original;
    int64_t      ex = *expected;
    UNREFERENCED_PARAMETER(weak);
    UNREFERENCED_PARAMETER(order_failure);
    UNREFERENCED_PARAMETER(order_success);
    assert((((uintptr_t) address) & 7) == 0 && "address must be 64-bit aligned");
    original = _InterlockedCompareExchange64((volatile LONGLONG*) address, desired, ex);
    success  = (original == ex) ? 1 : 0;
   *expected =  original;
    return success;
}

/* @summary Create a semaphore and initialize it with the specified number of available resources.
 * @param sem The semaphore object to initialize.
 * @param count The number of available resources.
 * @param max_value The maximum number of available resources, or zero to specify the maximum possible value.
 * @return Zero if the semaphore is successfully initialized, or -1 if an error occurred.
 */
static int
CORE__TaskCreateSemaphore
(
    CORE__TASK_SEMAPHORE *sem, 
    int32_t             count, 
    int32_t         max_value
)
{
    LONG maxv = max_value;
    if (maxv == 0)
    {   /* use the maximum possible value */
        maxv  = LONG_MAX;
    }
    if ((sem->Semaphore = CreateSemaphore(NULL, 0, maxv, NULL)) != NULL)
    {   /* the semaphore is successfully initialized */
        sem->Count = count;
        return  0;
    }
    else
    {   /* failed to create the operating system semaphore object */
        return -1;
    }
}

/* @summary Free all resources associated with a semaphore.
 * @param sem The semaphore object to destroy.
 */
static void
CORE__TaskDeleteSemaphore
(
    CORE__TASK_SEMAPHORE *sem
)
{
    if (sem->Semaphore != NULL)
    {
        CloseHandle(sem->Semaphore);
        sem->Semaphore = NULL;
    }
}

/* @summary Decrement the semaphore's counter (claiming an available resource.)
 * @param sem The semaphore from which the resource will be acquired.
 * @return Non-zero if a resource was acquired, or zero if no resources are available.
 */
static int
CORE__TaskSemaphoreTryWait
(
    CORE__TASK_SEMAPHORE *sem
)
{
    int32_t count = CORE__TaskAtomicLoad_s32(&sem->Count, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
    while  (count > 0)
    {
        if (CORE__TaskAtomicCompareAndSwap_s32(&sem->Count, &count, count-1, 1, CORE__TASK_ATOMIC_ORDERING_ACQ_REL, CORE__TASK_ATOMIC_ORDERING_RELAXED))
        {   /* the count was decremented, a resource was claimed */
            return 1;
        }
        /* backoff ? */
    }
    return 0;
}

/* @summary Decrement the semaphore's counter (claiming an available resource.)
 * If the counter value is less than zero, the calling thread is immediately put to sleep until the counter becomes positive.
 * @param sem The semaphore from which the resource will be acquired.
 */
static void
CORE__TaskSemaphoreWaitNoSpin
(
    CORE__TASK_SEMAPHORE *sem
)
{
    if (CORE__TaskAtomicFetchAdd_s32(&sem->Count, -1, CORE__TASK_ATOMIC_ORDERING_ACQ_REL) < 1)
    {
        WaitForSingleObject(sem->Semaphore, INFINITE);
    }
}

/* @summary Decrement the semaphore's counter (claiming an available resource.) 
 * If the counter value is less than or equal to zero, the calling thread is blocked until the counter becomes positive.
 * @param sem The semaphore from which the resource will be acquired.
 */
static void
CORE__TaskSemaphoreWait
(
    CORE__TASK_SEMAPHORE *sem, 
    int32_t        spin_count
)
{   assert(spin_count >= 0);
    while (spin_count--)
    {
        if (CORE__TaskSemaphoreTryWait(sem))
        {   /* successfully claimed a resource */
            return;
        }
    }
    CORE__TaskSemaphoreWaitNoSpin(sem);
}

/* @summary Increment the semaphore's counter (making a resource available.)
 * If the counter value is greater than or equal to zero, a waiting thread is woken.
 * @param sem The semaphore to which the resource is being returned.
 */
static void
CORE__TaskSemaphorePost
(
    CORE__TASK_SEMAPHORE *sem
)
{
    if (CORE__TaskAtomicFetchAdd_s32(&sem->Count, +1, CORE__TASK_ATOMIC_ORDERING_ACQ_REL) < 0)
    {
        ReleaseSemaphore(sem->Semaphore, 1, 0);
    }
}

/* @summary Increment the semaphore's counter by a specified amount, possibly waking one or more threads.
 * @param sem The semaphore to which the resource(s) are being returned.
 * @param count The number of resources being returned. This must be a positive integer.
 */
static void
CORE__TaskSemaphorePostCount
(
    CORE__TASK_SEMAPHORE *sem, 
    int32_t             count
)
{
    int32_t old_count = CORE__TaskAtomicFetchAdd_s32(&sem->Count, count, CORE__TASK_ATOMIC_ORDERING_ACQ_REL);
    if (old_count < 0)
    {
        int32_t num_waiters = -old_count;
        int32_t num_to_wake = num_waiters < count ? num_waiters : count;
        ReleaseSemaphore(sem->Semaphore, num_to_wake, 0);
    }
}

/* @summary Calculate the amount of memory required for an MPMC concurrent queue of available task slot indices of a given capacity.
 * @param capacity The capacity of the queue. This value must be a power-of-two greater than 2.
 * @return The minimum number of bytes required to successfully initialize a queue with the specified capacity.
 */
static size_t
CORE__QueryTaskFreeQueueMemorySize
(
    uint32_t capacity
)
{   assert(capacity >= 2); /* minimum capacity is 2 */
    assert((capacity & (capacity-1)) == 0); /* must be a power-of-two */
    return (capacity * sizeof(CORE__TASK_MPMC_CELL));
}

/* @summary Initialize a multi-producer, multi-consumer concurrent queue of available task slot indices.
 * @param fifo The MPMC queue object to initialize.
 * @param capacity The capacity of the queue. This value must be a power-of-two greater than 2.
 * @param memory The memory block to use for the queue storage.
 * @param memory_size The size of the memory block to use for the queue storage. This value must be at least the size returned by CORE__QueryTaskFreeQueueMemorySize(capacity).
 * @return Zero if the queue is successfully initialized, or non-zero if an error occurred.
 */
static int
CORE__InitTaskFreeQueue
(
    CORE__TASK_FREE_QUEUE *fifo,
    uint32_t           capacity, 
    void                *memory, 
    size_t          memory_size
)
{
    uint32_t i;

    if (capacity < 2)
    {   /* the minimum supported capacity is two elements */
        assert(capacity >= 2);
        ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
        return -1;
    }
    if ((capacity & (capacity-1)) != 0)
    {   /* the capacity must be a power-of-two */
        assert((capacity & (capacity-1)) == 0);
        ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
        return -1;
    }
    if (memory_size < (capacity * sizeof(CORE__TASK_MPMC_CELL)))
    {   /* the caller did not supply enough memory */
        assert(memory_size >= (capacity * sizeof(CORE__TASK_MPMC_CELL)));
        ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
        return -1;
    }

    ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
    fifo->Storage     =(CORE__TASK_MPMC_CELL*) memory;
    fifo->StorageMask =(uint32_t) (capacity-1);
    fifo->Capacity    = capacity;
    fifo->MemoryStart = memory;
    fifo->MemorySize  = memory_size;
    for (i = 0; i < capacity; ++i)
    {   /* set the sequence number for each cell */
        CORE__TaskAtomicStore_u32(&fifo->Storage[i].Sequence, i, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        fifo->Storage[i].Index = i;
    }
    CORE__TaskAtomicStore_u32(&fifo->EnqueuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    CORE__TaskAtomicStore_u32(&fifo->DequeuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    return 0;
}

/* @summary Push a task slot index onto the back of an available task queue. This function should be called by the thread that completed the task.
 * @param fifo The MPMC concurrent queue to receive the value.
 * @param task_index The task slot index being returned to the pool.
 * @return Non-zero if the value was enqueued, or zero if the queue is currently full.
 */
static int
CORE__TaskFreeQueuePush
(
    CORE__TASK_FREE_QUEUE *fifo, 
    uint32_t         task_index
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos = CORE__TaskAtomicLoad_u32(&fifo->EnqueuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    uint32_t               seq;
    int64_t               diff;
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int64_t) seq - (int64_t) pos;
        if (diff == 0)
        {   /* the queue is not full, attempt to claim this slot */
            if (CORE__TaskAtomicCompareAndSwap_u32(&fifo->EnqueuePos, &pos, pos + 1, 1, CORE__TASK_ATOMIC_ORDERING_RELAXED, CORE__TASK_ATOMIC_ORDERING_RELAXED))
            {   /* the slot was successfully claimed */
                break;
            }
        }
        else if (diff < 0)
        {   /* the queue is full */
            return 0;
        }
        else
        {   /* another producer claimed this slot, try again */
            pos = CORE__TaskAtomicLoad_u32(&fifo->EnqueuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        }
    }
    cell->Index = task_index;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    return 1;
}

/* @summary Push a task slot index onto the back of an available task queue. This function should be called by the thread that completed the task.
 * @param fifo The MPMC concurrent queue to receive the value.
 * @param task_id The task identifier returned to the pool. The task slot index is extracted from this value.
 * @return Non-zero if the value was enqueued, or zero if the queue is currently full.
 */
static int
CORE__TaskFreeQueuePushId
(
    CORE__TASK_FREE_QUEUE *fifo, 
    CORE_TASK_ID        task_id
)
{   assert(CORE_TaskIdValid(task_id));
    return CORE__TaskFreeQueuePush(fifo, CORE_TaskIndexInPool(task_id));
}

/* @summary Attempt to take a value from the front of an available task queue. This function should be called by the thread that owns the task pool and is defining the task.
 * @param fifo The MPMC concurrent queue from which the oldest value should be retrieved.
 * @param task_index If the function returns non-zero, on return, this location stores the dequeued task slot index value.
 * @return Non-zero if a value was dequeued, or zero if the queue is currently empty.
 */
static int
CORE__TaskFreeQueueTake
(
    CORE__TASK_FREE_QUEUE *fifo, 
    uint32_t        *task_index
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos = CORE__TaskAtomicLoad_u32(&fifo->DequeuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    uint32_t               seq;
    int64_t               diff;
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int64_t) seq - (int64_t)(pos + 1);
        if (diff == 0)
        {   /* the queue is not empty, attempt to claim this slot */
            if (CORE__TaskAtomicCompareAndSwap_u32(&fifo->DequeuePos, &pos, pos + 1, 1, CORE__TASK_ATOMIC_ORDERING_RELAXED, CORE__TASK_ATOMIC_ORDERING_RELAXED))
            {   /* the slot was successfully claimed */
                break;
            }
        }
        else if (diff < 0)
        {   /* the queue is empty */
            return 0;
        }
        else
        {   /* another consumer claimed this slot, try again */
            pos = CORE__TaskAtomicLoad_u32(&fifo->DequeuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        }
    }
   *task_index = cell->Index;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + mask + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    return 1;
}

/* @summary Calculate the amount of memory required for an concurrent deque of ready-to-run task IDs of a given capacity.
 * @param capacity The capacity of the deque. This value must be a power-of-two greater than 2.
 * @return The minimum number of bytes required to successfully initialize a deque with the specified capacity.
 */
static size_t
CORE__QueryTaskWorkQueueMemorySize
(
    uint32_t capacity
)
{   assert(capacity >= 2); /* minimum capacity is 2 */
    assert((capacity & (capacity-1)) == 0); /* must be a power-of-two */
    return (capacity * sizeof(CORE_TASK_ID));
}

/* @summary Initialize a concurrent deque of ready-to-run task IDs.
 * @param fifo The MPMC queue object to initialize.
 * @param capacity The capacity of the deque. This value must be a power-of-two greater than 2.
 * @param memory The memory block to use for the deque storage.
 * @param memory_size The size of the memory block to use for the deque storage. This value must be at least the size returned by CORE__QueryTaskWorkQueueMemorySize(capacity).
 * @return Zero if the deque is successfully initialized, or non-zero if an error occurred.
 */
static int
CORE__InitTaskWorkQueue
(
    CORE__TASK_WORK_QUEUE *fifo,
    uint32_t           capacity, 
    void                *memory, 
    size_t          memory_size
)
{
    uint32_t i;

    if (capacity < 2)
    {   /* the minimum supported capacity is two elements */
        assert(capacity >= 2);
        ZeroMemory(fifo, sizeof(CORE__TASK_WORK_QUEUE));
        return -1;
    }
    if ((capacity & (capacity-1)) != 0)
    {   /* the capacity must be a power-of-two */
        assert((capacity & (capacity-1)) == 0);
        ZeroMemory(fifo, sizeof(CORE__TASK_WORK_QUEUE));
        return -1;
    }
    if (memory_size < (capacity * sizeof(CORE_TASK_ID)))
    {   /* the caller did not supply enough memory */
        assert(memory_size >= (capacity * sizeof(CORE_TASK_ID)));
        ZeroMemory(fifo, sizeof(CORE__TASK_WORK_QUEUE));
        return -1;
    }

    ZeroMemory(fifo, sizeof(CORE__TASK_WORK_QUEUE));
    fifo->Storage     =(CORE_TASK_ID*) memory;
    fifo->StorageMask =(uint32_t) (capacity-1);
    fifo->Capacity    = capacity;
    fifo->MemoryStart = memory;
    fifo->MemorySize  = memory_size;
    CORE__TaskAtomicStore_s64(&fifo->PublicPos , 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    CORE__TaskAtomicStore_s64(&fifo->PrivatePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    return 0;
}

/* @summary Push a task ID onto the back of a ready-to-run task queue. This function should be called by the thread that owns the task pool only.
 * @param fifo The concurrent deque to receive the work item.
 * @param task_id The ready-to-run task identifier.
 * @return Non-zero if the value was enqueued, or zero if the deque is currently full.
 */
static void
CORE__TaskWorkQueuePush
(
    CORE__TASK_WORK_QUEUE *fifo, 
    CORE_TASK_ID        task_id
)
{
    CORE_TASK_ID *stor = fifo->Storage;
    int64_t       mask = fifo->StorageMask;
    int64_t        pos = CORE__TaskAtomicLoad_s64(&fifo->PrivatePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    stor[pos & mask]   = task_id;
    CORE__TaskAtomicStore_s64(&fifo->PrivatePos, pos + 1, CORE__TASK_ATOMIC_ORDERING_RELAXED);
}

/* @summary Attempt to take an item from the back of a ready-to-run task queue. This function should be called by the thread that owns the task pool only.
 * @param fifo The concurrent deque from which to take the work item.
 * @param task_id If the function returns non-zero, on return this location is updated with the task identifier of a ready-to-run task.
 * @param more_items If the function returns non-zero, on return this location is set to non-zero if there is at least one additional item in the deque.
 * @return Non-zero if a value was dequeued, or zero if the deque is currently empty.
 */
static int
CORE__TaskWorkQueueTake
(
    CORE__TASK_WORK_QUEUE *fifo, 
    CORE_TASK_ID       *task_id, 
    int             *more_items
)
{
    CORE_TASK_ID *stor = fifo->Storage;
    int64_t       mask = fifo->StorageMask;
    int64_t        pos = CORE__TaskAtomicLoad_s64(&fifo->PrivatePos, CORE__TASK_ATOMIC_ORDERING_RELAXED) - 1; /* no concurrent push operation can happen */
    int64_t        top = 0;
    int            res = 1;
    CORE__TaskAtomicStore_s64(&fifo->PrivatePos, pos, CORE__TASK_ATOMIC_ORDERING_SEQ_CST);                    /* make the pop visible to a concurrent steal */
    top = CORE__TaskAtomicLoad_s64(&fifo->PublicPos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    if (top <= pos)
    {   /* the deque is currently non-empty */
       *task_id = stor[pos & mask];
        if (top != pos)
        {   /* there's at least one more item in the deque - no need to race */
           *more_items = 1;
            return 1;
        }
        /* this was the final item in the deque - race a concurrent steal to claim it */
        if (!CORE__TaskAtomicCompareAndSwap_s64(&fifo->PublicPos, &top, top + 1, 0, CORE__TASK_ATOMIC_ORDERING_SEQ_CST, CORE__TASK_ATOMIC_ORDERING_RELAXED))
        {   /* this thread lost the race to a concurrent steal */
           *more_items = 0;
           *task_id = CORE_INVALID_TASK_ID;
            res = 0;
        }
        CORE__TaskAtomicStore_s64(&fifo->PrivatePos, top + 1, CORE__TASK_ATOMIC_ORDERING_RELAXED);
       *more_items = 0;
        return res;
    }
    else
    {   /* the deque is currently empty */
       *more_items = 0;
       *task_id = CORE_INVALID_TASK_ID;
        CORE__TaskAtomicStore_s64(&fifo->PrivatePos, top, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        return 0;
    }
}

/* @summary Attempt to take an item from the front of a ready-to-run task queue. This function can be called by any thread that executes work items.
 * @param fifo The concurrent deque from which to take the work item.
 * @param task_id If the function returns non-zero, on return this location is updated with the task identifier of a ready-to-run task.
 * @param more_items If the function returns non-zero, on return this location is set to non-zero if there is at least one additional item in the deque.
 * @return Non-zero if a value was dequeued, or zero if the deque is currently empty.
 */
static int
CORE__TaskWorkQueueSteal
(
    CORE__TASK_WORK_QUEUE *fifo, 
    CORE_TASK_ID       *task_id, 
    int             *more_items
)
{
    CORE_TASK_ID *stor = fifo->Storage;
    int64_t       mask = fifo->StorageMask;
    int64_t        top = CORE__TaskAtomicLoad_s64(&fifo->PublicPos , CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
    int64_t        pos = CORE__TaskAtomicLoad_s64(&fifo->PrivatePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    if (top < pos)
    {   /* the deque is currently non-empty */
       *task_id = stor[top & mask];
        /* race with other threads to claim the item */
        if (CORE__TaskAtomicCompareAndSwap_s64(&fifo->PublicPos, &top, top + 1, 0, CORE__TASK_ATOMIC_ORDERING_RELEASE, CORE__TASK_ATOMIC_ORDERING_RELAXED))
        {   /* this thread won the race and claimed the item */
           *more_items = ((top+1) < pos) ? 1 : 0;
            return 1;
        }
        else
        {   /* this thread lost the race - it should try again */
           *more_items = 1;
           *task_id = CORE_INVALID_TASK_ID;
            return 0;
        }
    }
    else
    {   /* the deque is currently empty */
       *more_items = 0;
       *task_id = CORE_INVALID_TASK_ID;
        return 0;
    }
}

/* @summary Calculate the amount of memory required for an MPMC concurrent queue of task pool indices of a given capacity.
 * @param capacity The capacity of the queue. This value must be a power-of-two greater than 2.
 * @return The minimum number of bytes required to successfully initialize a queue with the specified capacity.
 */
static size_t
CORE__QueryTaskStealQueueMemorySize
(
    uint32_t capacity
)
{   assert(capacity >= 2); /* minimum capacity is 2 */
    assert((capacity & (capacity-1)) == 0); /* must be a power-of-two */
    return (capacity * sizeof(CORE__TASK_MPMC_CELL));
}

/* @summary Initialize a multi-producer, multi-consumer concurrent queue of task pool indices.
 * @param fifo The MPMC queue object to initialize.
 * @param capacity The capacity of the queue. This value must be a power-of-two greater than 2.
 * @param memory The memory block to use for the queue storage.
 * @param memory_size The size of the memory block to use for the queue storage. This value must be at least the size returned by CORE__QueryTaskFreeQueueMemorySize(capacity).
 * @return Zero if the queue is successfully initialized, or non-zero if an error occurred.
 */
static int
CORE__InitTaskStealQueue
(
    CORE__TASK_STEAL_QUEUE *fifo,
    uint32_t            capacity, 
    void                 *memory, 
    size_t           memory_size
)
{
    uint32_t i;

    if (capacity < 2)
    {   /* the minimum supported capacity is two elements */
        assert(capacity >= 2);
        ZeroMemory(fifo, sizeof(CORE__TASK_STEAL_QUEUE));
        return -1;
    }
    if ((capacity & (capacity-1)) != 0)
    {   /* the capacity must be a power-of-two */
        assert((capacity & (capacity-1)) == 0);
        ZeroMemory(fifo, sizeof(CORE__TASK_STEAL_QUEUE));
        return -1;
    }
    if (memory_size < (capacity * sizeof(CORE__TASK_MPMC_CELL)))
    {   /* the caller did not supply enough memory */
        assert(memory_size >= (capacity * sizeof(CORE__TASK_MPMC_CELL)));
        ZeroMemory(fifo, sizeof(CORE__TASK_STEAL_QUEUE));
        return -1;
    }

    /* initialize the semaphore used to sleep threads when the queue is empty, 
     * and wake sleeping threads when the queue becomes non-empty. the semaphore
     * is initially empty.
     */
    if (CORE__TaskCreateSemaphore(&fifo->Semaphore, 0, capacity) < 0)
    {   /* the semaphore could not be initialized */
        ZeroMemory(fifo, sizeof(CORE__TASK_STEAL_QUEUE));
        return -1;
    }

    ZeroMemory(fifo, sizeof(CORE__TASK_STEAL_QUEUE));
    fifo->Storage     =(CORE__TASK_MPMC_CELL*) memory;
    fifo->StorageMask =(uint32_t) (capacity-1);
    fifo->Capacity    = capacity;
    fifo->MemoryStart = memory;
    fifo->MemorySize  = memory_size;
    for (i = 0; i < capacity; ++i)
    {   /* set the sequence number for each cell */
        CORE__TaskAtomicStore_u32(&fifo->Storage[i].Sequence, i, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        fifo->Storage[i].Index = i;
    }
    CORE__TaskAtomicStore_u32(&fifo->EnqueuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    CORE__TaskAtomicStore_u32(&fifo->DequeuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    return 0;
}

/* @summary Push a task pool index onto the back of a steal queue, and signal the semaphore to indicate that a resource is available. This function should be called by the thread that defines the task.
 * @param fifo The MPMC concurrent queue to receive the value.
 * @param pool_index The task pool index that has a task available to steal.
 * @return Non-zero if the value was enqueued, or zero if the queue is currently full.
 */
static int
CORE__TaskStealQueuePush
(
    CORE__TASK_STEAL_QUEUE *fifo, 
    uint32_t          pool_index
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos = CORE__TaskAtomicLoad_u32(&fifo->EnqueuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    uint32_t               seq;
    int64_t               diff;
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int64_t) seq - (int64_t) pos;
        if (diff == 0)
        {   /* the queue is not full, attempt to claim this slot */
            if (CORE__TaskAtomicCompareAndSwap_u32(&fifo->EnqueuePos, &pos, pos + 1, 1, CORE__TASK_ATOMIC_ORDERING_RELAXED, CORE__TASK_ATOMIC_ORDERING_RELAXED))
            {   /* the slot was successfully claimed */
                break;
            }
        }
        else if (diff < 0)
        {   /* the queue is full */
            return 0;
        }
        else
        {   /* another producer claimed this slot, try again */
            pos = CORE__TaskAtomicLoad_u32(&fifo->EnqueuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        }
    }
    cell->Index = pool_index;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    CORE__TaskSemaphorePost(&fifo->Semaphore);
    return 1;
}

/* @summary Attempt to take a value from the front of a steal queue. This function should be called by any thread that has run out of tasks in its local ready-to-run queue.
 * If the steal queue is empty, the calling thread blocks until items are available to steal from another thread.
 * @param fifo The MPMC concurrent queue from which the oldest value should be retrieved.
 * @param pool_index If the function returns non-zero, on return, this location stores the dequeued task pool index value.
 * @return Non-zero if a value was dequeued, or zero if the caller should try again.
 */
static int
CORE__TaskStealQueueTake
(
    CORE__TASK_STEAL_QUEUE *fifo, 
    uint32_t         *pool_index
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos;
    uint32_t               seq;
    int64_t               diff;

    /* attempt to claim a resource from the semaphore. 
     * if the thread does not block, the queue is non-empty.
     */
    CORE__TaskSemaphoreWait(&fifo->Semaphore, 4096);

    /* dequeue an item from the front of the queue */
    pos = CORE__TaskAtomicLoad_u32(&fifo->DequeuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int64_t) seq - (int64_t)(pos + 1);
        if (diff == 0)
        {   /* the queue is not empty, attempt to claim this slot */
            if (CORE__TaskAtomicCompareAndSwap_u32(&fifo->DequeuePos, &pos, pos + 1, 1, CORE__TASK_ATOMIC_ORDERING_RELAXED, CORE__TASK_ATOMIC_ORDERING_RELAXED))
            {   /* the slot was successfully claimed */
                break;
            }
        }
        else if (diff < 0)
        {   /* the queue is empty */
            return 0;
        }
        else
        {   /* another consumer claimed this slot, try again */
            pos = CORE__TaskAtomicLoad_u32(&fifo->DequeuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        }
    }
   *pool_index = cell->Index;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + mask + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    return 1;
}

/* @summary Implement a replacement for the strcmp function.
 * This function is not optimized and should not be used in performance-critical code.
 * @param str_a Pointer to a nul-terminated string. This value cannot be NULL.
 * @param str_b Pointer to a nul-terminated string. This value cannot be NULL.
 * @return Zero if the two strings match. A positive value if the first non-matching character in str_a > the corresponding character in str_b. A negative value if the first non-matching character str_a < the corresponding character in str_b.
 */
static int
CORE__TaskStringCompare
(
    char const *str_a, 
    char const *str_b
)
{
    unsigned char const *a = (unsigned char const *) str_a;
    unsigned char const *b = (unsigned char const *) str_b;
    while (*a && (*a == *b))
    {
        ++a;
        ++b;
    }
    return (*a - *b);
}

/* @summary Calculate the number of bits set in a processor affinity mask.
 * @param processor_mask The processor affinity mask to check.
 * @return The number of bits set in the mask.
 */
static uint32_t
CORE__TaskCountSetBitsInProcessorMask
(
    ULONG_PTR processor_mask
)
{
    uint32_t         i;
    uint32_t set_count = 0;
    uint32_t max_shift = sizeof(ULONG_PTR) * 8 - 1;
    ULONG_PTR test_bit =((ULONG_PTR)1) << max_shift;
    for (i = 0; i <= max_shift; ++i)
    {
        set_count +=(processor_mask & test_bit) ? 1 : 0;
        test_bit >>= 1;
    }
    return set_count;
}

CORE_API(int)
CORE_QueryHostCpuInfo
(
    CORE_TASK_CPU_INFO *cpu_info
)
{   /* this function supports systems with up to 64 cores  */
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *lpibuf = NULL;
    size_t                                  i, n;
    uint32_t                         num_threads = 0;
    DWORD                            buffer_size = 0;
    int                                  regs[4] ={0, 0, 0, 0};

    ZeroMemory(cpu_info, sizeof(CORE_TASK_CPU_INFO));

    /* retrieve the CPU vendor string using the __cpuid intrinsic */
    __cpuid(regs, 0);
    *((int*) &cpu_info->VendorName[0]) = regs[1];
    *((int*) &cpu_info->VendorName[4]) = regs[3];
    *((int*) &cpu_info->VendorName[8]) = regs[2];
         if (!CORE__TaskStringCompare(cpu_info->VendorName, "AuthenticAMD")) cpu_info->PreferAMD        = 1;
    else if (!CORE__TaskStringCompare(cpu_info->VendorName, "GenuineIntel")) cpu_info->PreferIntel      = 1;
    else if (!CORE__TaskStringCompare(cpu_info->VendorName, "KVMKVMKVMKVM")) cpu_info->IsVirtualMachine = 1;
    else if (!CORE__TaskStringCompare(cpu_info->VendorName, "Microsoft Hv")) cpu_info->IsVirtualMachine = 1;
    else if (!CORE__TaskStringCompare(cpu_info->VendorName, "VMwareVMware")) cpu_info->IsVirtualMachine = 1;
    else if (!CORE__TaskStringCompare(cpu_info->VendorName, "XenVMMXenVMM")) cpu_info->IsVirtualMachine = 1;

    /* inspect the CPU topology. this requires scratch memory. */
    GetLogicalProcessorInformation(NULL, &buffer_size);
    if ((lpibuf = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_size)) == NULL)
    {   /* failed to allocate the required amount of memory */
        ZeroMemory(cpu_info, sizeof(CORE_TASK_CPU_INFO));
        return -1;
    }
    if (GetLogicalProcessorInformation(lpibuf, &buffer_size))
    {   /* at least one SYSTEM_LOGICAL_PROCESSOR_INFORMATION was returned */
        for (i = 0, n = buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i < n; ++i)
        {
            switch (lpibuf[i].Relationship)
            {
                case RelationProcessorCore:
                    { num_threads = CORE__TaskCountSetBitsInProcessorMask(lpibuf[i].ProcessorMask);
                      cpu_info->HardwareThreads += num_threads;
                      cpu_info->ThreadsPerCore = num_threads;
                      cpu_info->PhysicalCores++;
                    } break;
                case RelationNumaNode:
                    { cpu_info->NumaNodes++;
                    } break;
                case RelationCache:
                    { if (lpibuf[i].Cache.Level == 1 && lpibuf[i].Cache.Type == CacheData)
                      { /* L1 cache information */
                          cpu_info->CacheSizeL1     = lpibuf[i].Cache.Size;
                          cpu_info->CacheLineSizeL1 = lpibuf[i].Cache.LineSize;
                      }
                      if (lpibuf[i].Cache.Level == 2 && lpibuf[i].Cache.Type == CacheUnified)
                      { /* L2 cache information */
                          cpu_info->CacheSizeL2     = lpibuf[i].Cache.Size;
                          cpu_info->CacheLineSizeL2 = lpibuf[i].Cache.LineSize;
                      }
                    } break;
                case RelationProcessorPackage:
                    { cpu_info->PhysicalCPUs++;
                    } break;
                default:
                    { /* unknown relationship type - skip */
                    } break;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpibuf);
        return 0;
    }
    else
    {   /* the call to GetLogicalProcessorInformation failed */
        ZeroMemory(cpu_info, sizeof(CORE_TASK_CPU_INFO));
        HeapFree(GetProcessHeap(), 0, lpibuf);
        return -1;
    }
}

CORE_API(int)
CORE_CreateTaskProfiler
(
    CORE_TASK_PROFILER  *profiler, 
    WCHAR const *application_name
)
{
#ifdef CORE_TASK_NO_PROFILER
    ZeroMemory(profiler, sizeof(CORE_TASK_PROFILER));
    return 0;
#else
    CV_MARKERSERIES  *series = NULL;
    CV_PROVIDER    *provider = NULL;
    HRESULT              res = S_OK;
    GUID const PROFILER_GUID = { 
        0x349ce0e9, 0x6df5, 0x4c25, 
      { 0xac, 0x5b, 0xc8, 0x4f, 0x52, 0x9b, 0xc0, 0xce } 
    };
    if (application_name == NULL)
    {   /* a non-NULL name is required, but an empty string is okay */
        application_name = L"CORE_TASK_PROFILER";
    }
    if (!SUCCEEDED((res = CvInitProvider(&PROFILER_GUID, &provider))))
    {
        ZeroMemory(profiler, sizeof(CORE_TASK_PROFILER));
        return -1;
    }
    if (!SUCCEEDED((res = CvCreateMarkerSeriesW(provider, application_name, &series))))
    {
        ZeroMemory(profiler, sizeof(CORE_TASK_PROFILER));
        CvReleaseProvider(provider);
        return -1;
    }
    profiler->Provider     = provider;
    profiler->MarkerSeries = series;
    return 0;
#endif
}

CORE_API(void)
CORE_DeleteTaskProfiler
(
    CORE_TASK_PROFILER *profiler
)
{
#ifdef CORE_TASK_NO_PROFILER
    UNREFERENCED_PARAMETER(profiler);
#else
    if (profiler->MarkerSeries != NULL)
    {
        CvReleaseMarkerSeries(profiler->MarkerSeries);
        profiler->MarkerSeries = NULL;
    }
    if (profiler->Provider != NULL)
    {
        CvReleaseProvider(profiler->Provider);
        profiler->Provider = NULL;
    }
#endif
}

CORE_API(int)
CORE_ValidateTaskPoolConfiguration
(
    CORE_TASK_POOL_INIT *type_configs,
    int32_t             *type_results, 
    uint32_t               type_count, 
    int32_t            *global_result
)
{
    uint32_t      i, j;
    uint64_t num_pools = 0;
    int   found_worker = 0;
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
       *global_result = CORE_TASK_POOL_VALIDATION_RESULT_NO_WORKER_ID;
        return -1;
    }
    if (type_count > CORE_MAX_TASK_POOLS)
    {   assert(type_count <= CORE_MAX_TASK_POOLS);
       *global_result = CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_POOLS;
        return -1;
    }
    /* start out assuming no problems */
   *global_result = CORE_TASK_POOL_VALIDATION_RESULT_SUCCESS;
    for (i = 0; i < type_count; ++i)
    {   /* start out assuming that the pool configuration is valid */
        num_pools      += type_configs[i].PoolCount;
        type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_SUCCESS;

        if (type_configs[i].PoolId == CORE_TASK_POOL_ID_WORKER)
        {   /* the one required pool ID is specified */
            found_worker = 1;
        }
        if (type_configs[i].PoolCount > CORE_MAX_TASK_POOLS)
        {   /* the PoolCount exceeds what can be addressed */
            assert(type_configs[i].PoolCount <= CORE_MAX_TASK_POOLS);
            type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_POOLS;
            result = -1;
        }
        if (type_configs[i].MaxActiveTasks < CORE_MIN_TASKS_PER_POOL)
        {   /* MaxActiveTasks specifies too small a value */
            assert(type_configs[i].MaxActiveTasks >= CORE_MIN_TASKS_PER_POOL);
            type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_TOO_FEW_TASKS;
            result = -1;
        }
        if (type_configs[i].MaxActiveTasks > CORE_MAX_TASKS_PER_POOL)
        {   /* MaxActiveTasks specifies too large a value */
            assert(type_configs[i].MaxActiveTasks <= CORE_MAX_TASKS_PER_POOL);
            type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_TASKS;
            result = -1;
        }
        if ((type_configs[i].MaxActiveTasks & (type_configs[i].MaxActiveTasks-1)) != 0)
        {   /* MaxActiveTasks must be a power-of-two */
            assert((type_configs[i].MaxActiveTasks & (type_configs[i].MaxActiveTasks-1)) == 0);
            type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_NOT_POWER_OF_TWO;
            result = -1;
        }
        for (j = 0; j < type_count; ++j)
        {
            if (i != j && type_configs[i].PoolId == type_configs[j].PoolId)
            {   /* the same PoolId is used for more than one pool configuration */
                assert(type_configs[i].PoolId != type_configs[j].PoolId);
                type_results[i] = CORE_TASK_POOL_VALIDATION_RESULT_DUPLICATE_ID;
                result = -1;
                break;
            }
        }
    }
    if (found_worker == 0)
    {
       *global_result = CORE_TASK_POOL_VALIDATION_RESULT_NO_WORKER_ID;
        return -1;
    }
    if (num_pools > CORE_MAX_TASK_POOLS)
    {
       *global_result = CORE_TASK_POOL_VALIDATION_RESULT_TOO_MANY_POOLS;
        return -1;
    }
    return result;
}

CORE_API(size_t)
CORE_QueryTaskPoolStorageMemorySize
(
    CORE_TASK_POOL_INIT *type_configs, 
    uint32_t               type_count
)
{
    size_t  required_size = 0;
    uint32_t   pool_count = 0;
    uint32_t      i, j, n;
    /* calculate the total number of task pools that will be allocated */
    for (i = 0; i < type_count; ++i)
    {
        pool_count += type_configs[i].PoolCount;
    }
    /* calculate the amount of memory required for the public data in _CORE_TASK_POOL_STORAGE */
    required_size += CORE__TaskAllocationSizeType (CORE__TASK_STEAL_QUEUE);        /* TaskStealQueue    */
    required_size += CORE__TaskAllocationSizeArray(CORE__TASK_POOL* , pool_count); /* TaskPoolList      */
    required_size += CORE__TaskAllocationSizeArray(uint32_t         , type_count); /* PoolTypeIds       */
    required_size += CORE__TaskAllocationSizeArray(CORE__TASK_POOL* , type_count); /* PoolFreeList      */
    required_size += CORE__TaskAllocationSizeArray(CRITICAL_SECTION , type_count); /* PoolFreeListLocks */
    required_size += CORE__QueryTaskStealQueueMemorySize(65536);                   /* TaskStealQueue    */
    /* calculate the amount of memory required for the private data in _CORE_TASK_POOL_STORAGE */
    for (i = 0; i < type_count; ++i)
    {
        size_t pool_size = 0;
        pool_size       += CORE__TaskAllocationSizeType(CORE__TASK_POOL);
        pool_size       += CORE__QueryTaskFreeQueueMemorySize(type_configs[i].MaxActiveTasks);
        pool_size       += CORE__QueryTaskWorkQueueMemorySize(type_configs[i].MaxActiveTasks);
        pool_size       += CORE__TaskAllocationSizeArray(CORE__TASK_DATA, type_configs[i].MaxActiveTasks);
        required_size   +=(pool_size * type_configs[i].PoolCount);
    }
    return required_size;
}

CORE_API(int)
CORE_CreateTaskPoolStorage
(
    CORE_TASK_POOL_STORAGE   *storage, 
    CORE_TASK_POOL_STORAGE_INIT *init
)
{
    CORE__TASK_ARENA arena;
    void        *steal_mem = NULL;
    size_t      steal_size = 0;
    size_t   required_size = 0;
    uint32_t    pool_index = 0;
    uint32_t    pool_count = 0;
    uint32_t    slot_count = 0;
    uint32_t    i, j, n, m;

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

    required_size = CORE_QueryTaskPoolStorageMemorySize(init->TaskPoolTypes, init->PoolTypeCount);
    if (init->MemorySize < required_size)
    {   /* the caller must supply sufficient memory for the storage object */
        assert(init->MemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* calculate the total number of task pools that will be allocated */
    for (i = 0, n = init->PoolTypeCount; i < n; ++i)
    {
        pool_count += init->TaskPoolTypes[i].PoolCount;
        slot_count +=(init->TaskPoolTypes[i].PoolCount * init->TaskPoolTypes[i].MaxActiveTasks);
    }

    /* zero everything out and assign memory */
    ZeroMemory(init->MemoryStart, init->MemorySize);
    ZeroMemory(storage, sizeof(CORE_TASK_POOL_STORAGE));
    CORE__TaskInitMemoryArena(&arena, init->MemoryStart, init->MemorySize);
    storage->MemoryStart       = init->MemoryStart;
    storage->MemorySize        = init->MemorySize;
    storage->PoolTypeCount     = init->PoolTypeCount;
    storage->TaskPoolCount     = pool_count;
    steal_size                 = CORE__QueryTaskStealQueueMemorySize(init->StealQueueCapacity);
    storage->StealQueue        = CORE__TaskMemoryArenaAllocateType (&arena, CORE__TASK_STEAL_QUEUE);
    steal_mem                  = CORE__TaskMemoryArenaAllocateHost (&arena, steal_size, CORE_AlignOf(uint32_t));
    storage->TaskPoolList      = CORE__TaskMemoryArenaAllocateArray(&arena, struct _CORE_TASK_POOL*, pool_count);
    storage->PoolTypeIds       = CORE__TaskMemoryArenaAllocateArray(&arena, uint32_t               , init->PoolTypeCount);
    storage->PoolFreeList      = CORE__TaskMemoryArenaAllocateArray(&arena, struct _CORE_TASK_POOL*, init->PoolTypeCount);
    storage->PoolFreeListLocks = CORE__TaskMemoryArenaAllocateArray(&arena, CRITICAL_SECTION       , init->PoolTypeCount);
    if (storage->PoolTypeIds == NULL || storage->PoolFreeList == NULL || storage->PoolFreeListLocks == NULL || storage->TaskPoolList == NULL || storage->StealQueue == NULL || steal_mem == NULL)
    {   /* insufficient memory - this implies an error in CORE_QueryTaskPoolStorageMemorySize */
        ZeroMemory(storage, sizeof(CORE_TASK_POOL_STORAGE));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    /* initialize the steal queue for the storage pool */
    if (CORE__InitTaskStealQueue(storage->StealQueue, init->StealQueueCapacity, steal_mem, steal_size) < 0)
    {   /* failed to initialize the steal queue */
        ZeroMemory(storage, sizeof(CORE_TASK_POOL_STORAGE));
        return -1;
    }
    /* allocate and initialize the task pool objects themselves */
    for (i = 0, n = init->PoolTypeCount; i < n; ++i)
    {
        size_t   free_size = CORE__QueryTaskFreeQueueMemorySize(init->TaskPoolTypes[i].MaxActiveTasks);
        size_t   work_size = CORE__QueryTaskWorkQueueMemorySize(init->TaskPoolTypes[i].MaxActiveTasks);
        uint32_t max_tasks = init->TaskPoolTypes[i].MaxActiveTasks;
        /* initialize the type ID and critical section protecting the free list */
        storage->PoolTypeIds[i]= init->TaskPoolTypes[i].PoolId;
        InitializeCriticalSectionAndSpinCount(&storage->PoolFreeListLocks[i], 4096);
        for (j = 0, m = init->TaskPoolTypes[i].PoolCount; j < m; ++j)
        {   /* initialize the individual CORE__TASK_POOL object */
            CORE__TASK_POOL *pool = CORE__TaskMemoryArenaAllocateType (&arena, CORE__TASK_POOL);
            void        *task_mem = CORE__TaskMemoryArenaAllocateArray(&arena, CORE__TASK_DATA, max_tasks);
            void        *free_mem = CORE__TaskMemoryArenaAllocateHost (&arena, free_size, CORE_AlignOf(uint32_t));
            void        *work_mem = CORE__TaskMemoryArenaAllocateHost (&arena, work_size, CORE_AlignOf(CORE_TASK_ID));
            if (pool == NULL || task_mem == NULL || free_mem == NULL || work_mem == NULL)
            {   /* insufficient memory - this implies an error in CORE_QueryTaskPoolStorageMemorySize */
                ZeroMemory(storage, sizeof(CORE_TASK_POOL_STORAGE));
                SetLastError(ERROR_INVALID_PARAMETER);
                return -1;
            }
            CORE__InitTaskFreeQueue(&pool->FreeTasks  , max_tasks, free_mem, free_size);
            CORE__InitTaskWorkQueue(&pool->ReadyTasks , max_tasks, work_mem, work_size);
            pool->Storage      = storage;
            pool->NextPool     = storage->PoolFreeList[i];
            pool->TaskData     =(CORE__TASK_DATA*)task_mem;
            pool->Capacity     = max_tasks;
            pool->ThreadId     = 0;
            pool->PoolIndex    = pool_index;
            pool->PoolId       = init->TaskPoolTypes[i].PoolId;
            pool->LastError    = 0;

            /* push the pool onto the front of the free list */
            storage->PoolFreeList[i] = pool;
            /* push the pool into the back of the pool list */
            storage->TaskPoolList[pool_index++] = pool;
        }
    }
    return 0;
}

CORE_API(void)
CORE_DeleteTaskPoolStorage
(
    CORE_TASK_POOL_STORAGE *storage
)
{
    uint32_t i, n;

    if (storage->StealQueue != NULL)
    {   /* TODO: delete the semaphore */
    }
    if (storage->PoolTypeCount > 0)
    {
        for (i = 0, n = storage->PoolTypeCount; i < n; ++i)
        {
            DeleteCriticalSection(&storage->PoolFreeListLocks[i]);
        }
    }
    if (storage->TaskPoolCount > 0)
    {
        for (i = 0, n = storage->TaskPoolCount; i < n; ++i)
        {
            CORE__TaskDeleteSemaphore(&storage->TaskPoolList[i]->Semaphore);
        }
    }
}

CORE_API(struct _CORE_TASK_POOL*)
CORE_AcquireTaskPool
(
    CORE_TASK_POOL_STORAGE *storage, 
    uint32_t           pool_type_id
)
{
    CORE__TASK_POOL     *pool = NULL;
    uint32_t    *pool_id_list = storage->PoolTypeIds;
    uint32_t  pool_type_count = storage->PoolTypeCount;
    uint32_t  pool_type_index = 0;
    int       pool_type_found = 0;
    uint32_t                i;

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
    {   /* a pool was successfully acquired - bind it to the calling thread */
        CORE__InitTaskFreeQueue(&pool->FreeTasks  , pool->Capacity, pool->FreeTasks.MemoryStart , pool->FreeTasks.MemorySize);
        CORE__InitTaskWorkQueue(&pool->ReadyTasks , pool->Capacity, pool->ReadyTasks.MemoryStart, pool->ReadyTasks.MemorySize);
        CORE__TaskCreateSemaphore(&pool->Semaphore, pool->Capacity, 0);
        pool->NextPool  = NULL;
        pool->ThreadId  = GetCurrentThreadId();
        pool->LastError = 0;
    }
    return pool;
}

CORE_API(void)
CORE_ReleaseTaskPool
(
    struct _CORE_TASK_POOL *pool
)
{
    CORE_TASK_POOL_STORAGE *storage = NULL;
    uint32_t          *pool_id_list = NULL;
    uint32_t           pool_type_id = 0;
    uint32_t        pool_type_count = 0;
    uint32_t        pool_type_index = 0;
    int             pool_type_found = 0;
    uint32_t                      i;

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
    {   /* free resources allocated to the pool on acquisition */
        CORE__TaskDeleteSemaphore(&pool->Semaphore);
        /* push the pool onto the front of the free list */
        EnterCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
        {
            pool->NextPool = storage->PoolFreeList[pool_type_index];
            storage->PoolFreeList[pool_type_index] = pool;
        }
        LeaveCriticalSection(&storage->PoolFreeListLocks[pool_type_index]);
    }
}

#endif /* CORE_TASK_IMPLEMENTATION */

