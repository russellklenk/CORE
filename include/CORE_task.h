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
#define PAD0   CORE_TASK_QUEUE_PAD_01
#define PAD1   CORE_TASK_QUEUE_PAD_01
#define PAD2   CORE_TASK_QUEUE_PAD_2
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_QUEUE {
    int64_t                          Public;                /* The public end of the deque, updated by steal operations. */
    uint8_t                          Pad0[PAD0];            /* Padding separating the public and private ends of the deque. */
    int64_t                          Private;               /* The private end of the deque, updated by push and take operations. */
    uint8_t                          Pad1[PAD1];            /* Padding separating the private and shared data. */
    int64_t                          Mask;                  /* The mask value used to map the Public and Private indices into the storage array. */
    CORE_TASK_ID                    *TaskIds;               /* The identifiers of the ready-to-run tasks in the queue. */
    uint8_t                          Pad2[PAD2];            /* Padding separating task queue instances. */
} CORE_TASK_QUEUE;
#undef PAD2
#undef PAD1
#undef PAD0

/* @summary Define the data tracked internally for each task. Aligned to and limited to one cacheline.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_DATA {
    #define NUM_DATA                 CORE_MAX_TASK_DATA_BYTES
    #define NUM_PERMITS              CORE_MAX_TASK_PERMITS
    int32_t                          WaitCount;             /* The number of tasks that must complete before this task can run. */
    CORE_TASK_ID                     ParentId;              /* The identifier of the parent task, or CORE_INVALID_TASK_ID. */
    CORE_TaskMain_Func               TaskMain;              /* The function to call to execute the task workload. */
    uint8_t                          TaskData[NUM_DATA];    /* Argument data to pass to the task entrypoint. */
    int32_t                          WorkCount;             /* The number of outstanding work items (one for the task, plus one for each child task.) */
    int32_t                          PermitCount;           /* The number of tasks that this task permits to run (the number of valid permits.) */
    CORE_TASK_ID                     PermitIds[NUM_PERMITS];/* The task ID of each task permitted to run when this task completes. */
    #undef  NUM_PERMITS
    #undef  NUM_DATA
} CORE_TASK_DATA;

// Multiple threads can concurrently write to the free set.
// Items are not returned to the free set in any particular order.
// Only one thread can take from the free set.
// Take and Return can happen concurrently.
// Ideally, we do not want Take and Return to access the same cacheline (bucket).
// Within a single 32-bit word, a set bit indicates a free slot, while a clear bit indicates a used slot.
// Return only ever sets bits; take only ever clears bits.
// There are a maximum of 65536 tasks, which is 2048 32-bit words, which is 128 cacheline-sized buckets
// -or-                   32768 tasks, which is 1024 32-bit words, which is 64 cacheline-sized buckets
// -or-                   16384 tasks, which is 512 32-bit words, which is 32 cacheline-sized buckets
// -or-                   8192 tasks, which is 256 32-bit words, which is 16 cacheline-sized buckets
// -or-                   4096 tasks, which is 128 32-bit words, which is 8 cacheline-sized buckets
// -or-                   2048 tasks, which is 64 32-bit words, which is 4 cacheline-sized buckets
// -or-                   1024 tasks, which is 32 32-bit words, which is 2 cacheline-sized buckets
// -or-                   512 tasks, which is 16 32-bit words, which is 1 cacheline-sized bucket
// We should probably have a minimum of 512 tasks per-pool, which is 32KB of task data.
// One bit vector is maintained (which is 8KB).
// Each bucket (128 max) has a FreeCount which is modified atomically.
// The pool itself (or the free list, take your pick) maintains a fusem with initial count = MaxTaskCount.
//   This nicely solves the "no tasks available" problem; the thread must stall until tasks complete.
//   https://github.com/cdwfs/cds_sync/blob/master/cds_sync.h
// Once the thread has claimed items from the semaphore, it knows that there are *at least* that many free IDs.
// It then begins claiming task IDs, aiming to minimize the number of atomic operations/fences:
// int bucket_index = 0;
// int num_free = 0;
// do {
//     /* find a bucket with free items */
//     while (bucket_index < bucket_count)
//     {
//         num_free = BucketFreeCount[bucket_index];
//         if (num_free > 0)
//         {
//           break;
//         }
//         bucket_index++;
//     }
//     /* claim as many slots as possible, up to num_ids_to_claim */
//     int claimed = min(num_free, num_ids_to_claim);
//     InterlockedAdd(&BucketFreeCount[bucket_index], -claimed);
//     num_ids_to_claim -= claimed;
//     /* mark indices as being in-use, one at a time.
//      * other threads could be concurrently setting additional bits within these words. */
//     int32_t   slot_index = bucket_index * 512; /* 32 bits per-word, 16 words per-bucket */
//     int32_t *bucket_word = &StatusBits[bucket_index * 16]; /* 16 32-bit words per-bucket */
//     do {
//         int32_t       word = *bucket_word;
//         int32_t      clear = 0;
//         while (_BitScanForward(word, &bit_index))
//         { /* claim slot bit_index */
//           id_list[num_ids++] = slot_index + bit_index;
//           clear |= (1 << bit_index);
//           word &= ~(1 << bit_index);
//           claimed--;
//         }
//         if (clear != 0)
//         {
//             _InterlockedAnd(bucket_word, ~clear); /* clear claimed bits */
//         }
//         slot_index += 32;
//         bucket_word++;
//         word = *bucket_word;
//     } while (claimed > 0);
// } while (num_ids_to_claim > 0);
// need to figure out where to put fences instead of _Interlocked*.
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_POOL_FREE_LIST {
    uint16_t                        *FreeIndices;
    int32_t                          IndexMask;
} CORE_TASK_POOL_FREE_LIST;

/* @summary Define the data associated with a pre-allocated, fixed-size pool of tasks.
 * Each task pool is associated with a single thread that submits and optionally executes tasks.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE_TASK_POOL {
    uint16_t                        *TaskFreeList;          /* An array of indices of free task slots. */
    uint32_t                         IndexMask;             /* A mask value used to map a task index value into the task data arrays. This is the array size minus one. */
    uint32_t                         PoolIndex;             /* The zero-based index of this task pool within the scheduler's array of task pools. */
    uint32_t                         PoolUsage;             /* One or more of _CORE_TASK_POOL_USAGE indicating, among other things, whether the pool can be used to execute tasks. */
    uint32_t                         ThreadId;              /* The operating system identifier of the thread that owns the pool. */
    int32_t                          LastError;             /* The error code reported by the most recent attempt to define a task within the pool. */
    uint32_t                         PoolId;                /* The application-defined identifier of the associated pool type. */
    uint32_t                         Reserved1;             /* Reserved for future use. Set to zero. */
    uint32_t                         Reserved2;             /* Reserved for future use. Set to zero. */
    uint16_t                         NextWorker;            /* The zero-based index of the next worker thread to notify of waiting tasks that can be stolen. */
    uint16_t                         WorkerCount;           /* The total number of worker threads in the scheduler thread pool. */
    struct _CORE_TASK_POOL          *TaskPoolList;          /* A local pointer to the set of all task pools owned by the scheduler. */
    struct _CORE_TASK_DATA          *TaskPoolData;          /* The buffer used for storing per-task data. */
    struct _CORE_TASK_POOL          *NextFreePool;          /* A pointer to the next free pool in the free list, or NULL if this pool is currently allocated. */
    CORE_TASK_QUEUE                  WorkQueue;             /* The work-stealing deque of task IDs that are ready-to-run. */
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

