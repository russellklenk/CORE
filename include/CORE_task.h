/*
 * CORE_task.h: A single-file library for executing compute tasks on a thread pool.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_TASK_H__
#define __CORE_TASK_H__

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

/* @summary Define the size of a pointer, in bytes, on the target platform.
 */
#ifndef CORE_TASK_POINTER_SIZE
#ifdef _M_X64
#define CORE_TASK_POINTER_SIZE            8
#endif
#ifdef _M_X86
#define CORE_TASK_POINTER_SIZE            4
#endif
#endif

/* @summary Define the size of a single cacheline on the runtime target platform.
 */
#ifndef CORE_TASK_CACHELINE_SIZE
#define CORE_TASK_CACHELINE_SIZE          64
#define CORE_TASK_QUEUE_PAD_01           (CORE_TASK_CACHELINE_SIZE-8)
#define CORE_TASK_QUEUE_PAD_2            (CORE_TASK_CACHELINE_SIZE-8-CORE_TASK_POINTER_SIZE)
#endif

/* @summary Indicate that a type or field should be aligned to a cacheline boundary.
 */
#ifndef CORE_TASK_CACHELINE_ALIGN
#define CORE_TASK_CACHELINE_ALIGN         __declspec(align(CORE_TASK_CACHELINE_SIZE))
#endif

/* @summary Define various constants used to configure the task scheduler.
 * CORE_INVALID_TASK_ID    : Returned from API functions when a task could not be created.
 * CORE_MIN_TASK_POOLS     : The minimum allowable number of task pools. Each thread that interacts with the task system has an associated task pool.
 * CORE_MAX_TASK_POOLS     : The maximum allowable number of task pools. Each thread that interacts with the task system has an associated task pool.
 * CORE_MIN_TASKS_PER_POOL : The minimum allowable number of simultaneously active tasks per-task pool.
 * CORE_MAX_TASKS_PER_POOL : The maximum allowable number of simultaneously active tasks per-task pool.
 * CORE_MAX_TASK_DATA_BYTES: The maximum amount of data that can be stored with a task.
 * CORE_MAX_TASK_PERMITS   : The maximum number of tasks a task can permit to run.
 */
#ifndef CORE_TASK_CONSTANTS
#define CORE_TASK_CONSTANTS
#define CORE_INVALID_TASK_ID              0x7FFFFFFFL
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

/* @summary Emit a Concurrency Visualizer event to the trace session.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param fmt The printf-style format string.
 * @param ... Substitution arguments for the format string.
 */
#ifndef CORE_TaskEvent
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskEvent(env, fmt, ...)
#else
#define CORE_TaskEvent(env, fmt, ...)                                          \
    CvWriteAlertW((env)->Profiler->MarkerSeries, _T(fmt), __VA_ARGS__)
#endif
#endif

/* @summary Indicate the start of a labeled time period within the Concurrency Visualizer trace session. Note that spans cannot be nested within the same thread.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param span The CORE_TASK_PROFILER_SPAN representing the interval being labeled.
 * @param fmt The printf-style format string.
 * @param ... Substitution arguments for the format string.
 */
#ifndef CORE_TaskSpanEnter
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskSpanEnter(env, span, fmt, ...)
#else
#define CORE_TaskSpanEnter(env, span, fmt, ...)                                \
    CvEnterSpanW((env)->Profiler->MarkerSeries, &(span).CvSpan, _T(fmt), __VA_ARGS__)
#endif
#endif

/* @summary Indicate the end of a labeled time period within the Concurrency Visualizer trace session.
 * @param env The CORE_TASK_ENVIRONMENT object owned by the calling thread.
 * @param span The CORE_TASK_PROFILER_SPAN representing the labeled interval.
 */
#ifndef CORE_TaskSpanLeave
#ifdef  CORE_TASK_NO_PROFILER
#define CORE_TaskSpanLeave(env, span)
#else
#define CORE_TaskSpanLeave(env, span)                                          \
    CvLeaveSpan((span).CvSpan)
#endif
#endif

/* Forward-declare types exported by the library */
struct _CORE_TASK_CPU_INFO;
struct _CORE_TASK_PROFILER;
struct _CORE_TASK_PROFILER_SPAN;
struct _CORE_TASK_QUEUE;
struct _CORE_TASK_DATA;
struct _CORE_TASK_POOL;
struct _CORE_TASK_POOL_INIT;
struct _CORE_TASK_ENVIRONMENT;
struct _CORE_TASK_SCHEDULER;
struct _CORE_TASK_SCHEDULER_INIT;
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
    void                           *task_args, 
    struct _CORE_TASK_ENVIRONMENT   *task_env
);

/* @summary Define information describing the CPU layout of the host system.
 */
typedef struct _CORE_TASK_CPU_INFO {
    uint32_t                         NumaNodes;             /* The number of NUMA nodes in the host system. */
    uint32_t                         PhysicalCPUs;          /* The total number of physical CPUs in the host system. */
    uint32_t                         PhysicalCores;         /* The total number of physical cores across all CPUs. */
    uint32_t                         HardwareThreads;       /* The total number of hardware threads across all CPUs. */
    uint32_t                         ThreadsPerCore;        /* The number of hardware threads per physical core. */
    int32_t                          PreferAMD;             /* Non-zero if AMD implementations are preferred. */
    int32_t                          PreferIntel;           /* Non-zero if Intel implementations are preferred. */
    int32_t                          IsVirtualMachine;      /* Non-zero if the system appears to be a virtual machine. */
    char                             VendorName[13];        /* The nul-terminated CPUID vendor string. */
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

/* @summary Define the data associated with a double-ended queue of ready-to-run task identifiers.
 * The thread that owns the queue can perform push and take operations; other threads can only perform steal operations.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_QUEUE {
    #define P0                       CORE_TASK_QUEUE_PAD_01
    #define P1                       CORE_TASK_QUEUE_PAD_01
    #define P2                       CORE_TASK_QUEUE_PAD_2
    int64_t                          Public;                /* The public end of the deque, updated by steal operations. */
    uint8_t                          Pad0[P0];              /* Padding separating the public and private ends of the deque. */
    int64_t                          Private;               /* The private end of the deque, updated by push and take operations. */
    uint8_t                          Pad1[P1];              /* Padding separating the private and shared data. */
    int64_t                          Mask;                  /* The mask value used to map the Public and Private indices into the storage array. */
    CORE_TASK_ID                    *TaskIds;               /* The identifiers of the ready-to-run tasks in the queue. */
    uint8_t                          Pad2[P2];              /* Padding separating task queue instances. */
    #undef  P2
    #undef  P1
    #undef  P0
} CORE_TASK_QUEUE;

typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_DATA {
    #define NUM_DATA                 CORE_MAX_TASK_DATA_BYTES
    #define NUM_PERMITS              CORE_MAX_TASK_PERMITS
    int32_t                          WaitCount;             /* */
    CORE_TASK_ID                     ParentId;              /* */
    CORE_TaskMain_Func               TaskMain;              /* */
    uint8_t                          TaskData[NUM_DATA];    /* */
    int32_t                          WorkCount;             /* */
    int32_t                          PermitCount;           /* */
    CORE_TASK_ID                     PermitIds[NUM_PERMITS];/* */
    #undef  NUM_PERMITS
    #undef  NUM_DATA
} CORE_TASK_DATA;

typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_POOL {
    uint32_t                        *SlotStatus;            /* */
    uint32_t                         IndexMask;             /* */
    uint32_t                         NextIndex;             /* */
    uint32_t                         PoolIndex;             /* */
    uint32_t                         PoolUsage;             /* */
    uint32_t                         ThreadId;              /* */
    int32_t                          LastError;             /* */
    uint32_t                         PoolId;                /* */
    uint16_t                         NextWorker;            /* */
    uint16_t                         WorkerCount;           /* */
    struct _CORE_TASK_POOL          *TaskPoolList;          /* */
    struct _CORE_TASK_DATA          *TaskPoolData;          /* */
    struct _CORE_TASK_POOL          *NextFreePool;          /* */
    CORE_TASK_QUEUE                  WorkQueue;             /* */
} CORE_TASK_POOL;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_TASK_H__ */

#ifdef CORE_TASK_IMPLEMENTATION

#endif /* CORE_TASK_IMPLEMENTATION */

