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
#ifndef CORE_TASK_L1_CACHELINE_SIZE
#define CORE_TASK_L1_CACHELINE_SIZE       64
#define CORE_TASK_QUEUE_PAD_01           (CORE_TASK_L1_CACHELINE_SIZE-8)
#define CORE_TASK_QUEUE_PAD_2            (CORE_TASK_L1_CACHELINE_SIZE-8-CORE_TASK_POINTER_SIZE)
#endif

/* @summary Indicate that a type or field should be aligned to a cacheline boundary.
 */
#ifndef CORE_TASK_CACHELINE_ALIGN
#define CORE_TASK_CACHELINE_ALIGN         __declspec(align(CORE_TASK_L1_CACHELINE_SIZE))
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

/* @summary Define the amount of padding, in bytes, used for a CORE__TASK_SEMAPHORE object.
 */
#ifndef CORE__TASK_SEMAPHORE_PADDING_SIZE
#define CORE__TASK_SEMAPHORE_PADDING_SIZE            (CORE_TASK_L1_CACHELINE_SIZE-sizeof(int32_t)-sizeof(HANDLE))
#endif

/* @summary Define the amount of padding, in bytes, separating the enqueue or dequeue index from adjacent data in an MPMC bounded queue.
 */
#ifndef CORE__TASK_MPMC_QUEUE_PADDING_SIZE_INDEX
#define CORE__TASK_MPMC_QUEUE_PADDING_SIZE_INDEX     (CORE_TASK_L1_CACHELINE_SIZE-sizeof(int32_t))
#endif

/* @summary Define the amount of padding, in bytes, separating the shared data from adjacent data in an MPMC bounded queue.
 */
#ifndef CORE__TASK_MPMC_QUEUE_PADDING_SIZE_SHARED
#define CORE__TASK_MPMC_QUEUE_PADDING_SIZE_SHARED    (CORE_TASK_L1_CACHELINE_SIZE-sizeof(int32_t)-CORE_TASK_POINTER_SIZE)
#endif

/* @summary Define the data associated with a single item in an MPMC concurrent queue.
 * These items store the zero-based index of a free task slot within the task pool.
 */
typedef struct _CORE__TASK_MPMC_CELL {
    uint32_t                              Sequence;          /* The sequence number assigned to the cell. */
    uint32_t                              Value;             /* The value stored in the cell. This is the zero-based index of an available task slot. */
} CORE__TASK_MPMC_CELL;

/* @summary Define the data associated with a semaphore guaranteed to stay in userspace unless a thread needs to be downed or woken.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_SEMAPHORE {
    #define PAD_SIZE                      CORE__TASK_SEMAPHORE_PADDING_SIZE
    int32_t                               Count;             /* The current count. */
    HANDLE                                Semaphore;         /* The operating system semaphore object. */
    uint8_t                               Pad[PAD_SIZE];     /* Padding out to a cacheline boundary. */
    #undef  PAD_SIZE
} CORE__TASK_SEMAPHORE;

/* @summary Define the data associated with a fixed-size, MPMC concurrent queue. The queue capacity must be a power-of-two.
 * The MPMC queue is used in an MPSC fashion to return completed task indices to the owning task pool.
 * Threads that complete tasks are the producers, and the thread that owns the task pool is the consumer.
 * The queue capacity matches the task pool capacity.
 * The queue implementation (C++) is originally by Dmitry Vyukov and is available here:
 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_MPMC_QUEUE {
    #define PAD_SHARED                    CORE__TASK_MPMC_QUEUE_PADDING_SIZE_SHARED
    #define PAD_ENQUEUE                   CORE__TASK_MPMC_QUEUE_PADDING_SIZE_INDEX
    #define PAD_DEQUEUE                   CORE__TASK_MPMC_QUEUE_PADDING_SIZE_INDEX
    CORE__TASK_MPMC_CELL                 *Storage;           /* Storage for queue items. Fixed-size, power-of-two. */
    uint32_t                              StorageMask;       /* The mask used to map EnqueuePos and DequeuePos into the storage array. */
    uint8_t                               Pad0[PAD_SHARED];  /* Padding separating shared data from producer-only data. */
    uint32_t                              EnqueuePos;        /* A monotonically-increasing integer representing the position of the next enqueue operation. */
    uint8_t                               Pad1[PAD_ENQUEUE]; /* Padding separating the producer-only data from the consumer-only data. */
    uint32_t                              DequeuePos;        /* A monotonically-increasing integer representing the position of the next dequeue operation. */
    uint8_t                               Pad2[PAD_DEQUEUE]; /* Padding separating the consumer-only data from adjacent data. */
    #undef  PAD_SHARED
    #undef  PAD_ENQUEUE
    #undef  PAD_DEQUEUE
} CORE__TASK_MPMC_QUEUE;

/* @summary Define the supported memory ordering constraints for atomic operations.
 */
typedef enum _CORE__TASK_ATOMIC_ORDERING {
    CORE__TASK_ATOMIC_ORDERING_RELAXED   = 0,                /* No inter-thread ordering constraints. */
    CORE__TASK_ATOMIC_ORDERING_ACQUIRE   = 1,                /* Imposes a happens-before constraint from a store-release. Subsequent loads are not hoisted. */
    CORE__TASK_ATOMIC_ORDERING_RELEASE   = 2,                /* Imposes a happens-before constraint to a load-acquire. Preceeding stores are not sunk. */
    CORE__TASK_ATOMIC_ORDERING_ACQ_REL   = 3,                /* Combine ACQUIRE and RELEASE semantics. Subsequent loads are not hoisted, preceeding stores are not sunk. */
    CORE__TASK_ATOMIC_ORDERING_SEQ_CST   = 4,                /* Enforce total ordering (sequential consistency) with all other SEQ_CST operations. */
} CORE__TASK_ATOMIC_ORDERING;

/* @summary Atomically load a 32-bit value from a memory location.
 * @param address The memory location to read.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 * @return The 32-bit value stored at the specified memory location.
 */
static int32_t
CORE__TaskAtomicLoad_s32
(
    int32_t *address, 
    int    /*order*/
)
{   /* on x86 and x86-64 loads of aligned 32-bit values are atomic */
    int32_t v;
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_ACQUIRE:
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { v = *address;
              _ReadWriteBarrier();
            } break;
        default:
            { assert(false && "Unsupported memory ordering on load");
              v = *address;
              _ReadWriteBarrier();
            } break;
    }
    return v;
}

/* @summary Atomically load a 32-bit value from a memory location.
 * @param address The memory location to read.
 * @param order One of CORE__TASK_ATOMIC_ORDERING specifying the memory ordering constraint.
 * @return The 32-bit value stored at the specified memory location.
 */
static uint32_t
CORE__TaskAtomicLoad_u32
(
    uint32_t *address, 
    int     /*order*/
)
{   /* on x86 and x86-64 loads of aligned 32-bit values are atomic */
    uint32_t v;
    switch (order)
    {
        case CORE__TASK_ATOMIC_ORDERING_RELAXED:
        case CORE__TASK_ATOMIC_ORDERING_ACQUIRE:
        case CORE__TASK_ATOMIC_ORDERING_SEQ_CST:
            { v = *address;
              _ReadWriteBarrier();
            } break;
        default:
            { assert(false && "Unsupported memory ordering on load");
              v = *address;
              _ReadWriteBarrier();
            } break;
    }
    return v;
}

/* @summary Atomically store a 32-bit value to a memory location.
 * @param address The memory location to write.
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
            { assert(false && "Unsupported memory ordering constraint on store");
              _ReadWriteBarrier();
              *address = value;
            } break;
    }
}

/* @summary Atomically store a 32-bit value to a memory location.
 * @param address The memory location to write.
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
            { assert(false && "Unsupported memory ordering constraint on store");
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
    UNREFERENCED_PARAMETER(order);
    return _InterlockedExchangeAdd((volatile LONG*) address, (LONG) value);
}

/* @summary Load a 32-bit value from a memory location, compare it with an expected value, and if they match, store a new value in the memory location as a single atomic operation.
 * @param address The memory location to load and potentially update. The address must be 32-bit aligned.
 * @param expected A memory location containing the expected value. On return, this memory location contains the original value stored at address.
 * @param desired The value that will be stored to the memory location if the current value matches the expected value.
 * @param weak Specify non-zero to ...
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
    original = _InterlockedCompareExchange((volatile LONG*) address, desired, ex);
    success  = (original == ex) ? 1 : 0;
   *expected =  original;
    return success;
}

/* @summary Load a 32-bit value from a memory location, compare it with an expected value, and if they match, store a new value in the memory location as a single atomic operation.
 * @param address The memory location to load and potentially update. The address must be 32-bit aligned.
 * @param expected A memory location containing the expected value. On return, this memory location contains the original value stored at address.
 * @param desired The value that will be stored to the memory location if the current value matches the expected value.
 * @param weak Specify non-zero to ...
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
    original = (uint32_t) _InterlockedCompareExchange((volatile LONG*) address, (LONG) desired, (LONG) ex);
    success  = (original == ex) ? 1 : 0;
   *expected =  original;
    return success;
}

/* @summary Create a semaphore and initialize it with the specified number of available resources.
 * @param sem The semaphore object to initialize.
 * @param count The number of available resources.
 * @return Zero if the semaphore is successfully initialized, or -1 if an error occurred.
 */
static int
CORE__TaskCreateSemaphore
(
    CORE__TASK_SEMAPHORE *sem, 
    int32_t             count
)
{
    sem->Count = count;
    if ((sem->Semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL)) != NULL)
    {   /* the semaphore is successfully initialized */
        return  0;
    }
    else
    {   /* failed to create the operating system semaphore object */
        return -1;
    }
}

/* @summary Destroy a semaphore.
 * @param sem The semaphore object to destroy.
 */
static void
CORE__TaskDestroySemaphore
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

static size_t
CORE__QueryTaskMPMCQueueMemorySize
(
    uint32_t capacity
)
{   assert(capacity >= 2); /* minimum capacity is 2 */
    assert((capacity & (capacity-1)) == 0); /* must be a power-of-two */
    return (capacity * sizeof(CORE__TASK_MPMC_CELL));
}

/* @summary Initialize a multi-producer, multi-consumer concurrent queue.
 * @param fifo The MPMC queue object to initialize.
 * @param capacity The capacity of the queue. This value must be a power-of-two greater than 2.
 * @param memory The memory block to use for the queue storage.
 * @param memory_size The size of the memory block to use for the queue storage. This value must be at least the size returned by CORE__QueryTaskMPMCQueueMemorySize(capacity).
 * @return Zero if the queue is successfully initialized, or non-zero if an error occurred.
 */
static int
CORE__InitTaskMPMCQueue
(
    CORE__TASK_MPMC_QUEUE *fifo,
    uint32_t           capacity, 
    void                *memory, 
    size_t          memory_size
)
{
    uint32_t i;

    if (capacity < 2)
    {   /* the minimum supported capacity is two elements */
        assert(capacity >= 2);
        ZeroMemory(fifo, sizeof(CORE__TASK_MPMC_QUEUE));
        return -1;
    }
    if ((capacity & (capacity-1)) != 0)
    {   /* the capacity must be a power-of-two */
        assert((capacity & (capacity-1)) == 0);
        ZeroMemory(fifo, sizeof(CORE__TASK_MPMC_QUEUE));
        return -1;
    }
    if (memory_size < (capacity * sizeof(CORE__TASK_MPMC_CELL)))
    {   /* the caller did not supply enough memory */
        assert(memory_size >= (capacity * sizeof(CORE__TASK_MPMC_CELL)));
        ZeroMemory(fifo, sizeof(CORE__TASK_MPMC_QUEUE));
        return -1;
    }

    ZeroMemory(fifo, sizeof(CORE__TASK_MPMC_QUEUE));
    fifo->Storage     = (CORE__TASK_MPMC_CELL*) memory;
    fifo->StorageMask = (uint32_t)(capacity - 1);
    for (i = 0; i < capacity; ++i)
    {   /* set the sequence number for each cell */
        CORE__TaskAtomicStore_u32(&fifo->Storage[i].Sequence, i, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        fifo->Storage[i].Value = i;
    }
    CORE__TaskAtomicStore_u32(&fifo->EnqueuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    CORE__TaskAtomicStore_u32(&fifo->DequeuePos, 0, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    return 0;
}

/* @summary Attempt to enqueue a value.
 * @param fifo The MPMC concurrent queue to receive the value.
 * @param value The value to enqueue.
 * @return Non-zero if the value was enqueued, or zero if the queue is currently full.
 */
static int
CORE__TaskMPMCQueuePut
(
    CORE__TASK_MPMC_QUEUE *fifo, 
    uint32_t              value
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos = CORE__TaskAtomicLoad_u32(&fifo->EnqueuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    uint32_t               seq;
    int32_t               diff;
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int32_t) seq - (int32_t) pos;
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
    cell->Value = value;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    return 1;
}

/* @summary Attempt to dequeue a value.
 * @param fifo The MPMC concurrent queue from which the oldest value should be retrieved.
 * @param value If the function returns non-zero, on return, this location stores the dequeued value.
 * @return Non-zero if a value was dequeued, or zero if the queue is currently empty.
 */
static int
CORE__TaskMPMCQueueGet
(
    CORE__TASK_MPMC_QUEUE *fifo, 
    uint32_t             *value
)
{
    CORE__TASK_MPMC_CELL *cell;
    CORE__TASK_MPMC_CELL *stor = fifo->Storage;
    uint32_t              mask = fifo->StorageMask;
    uint32_t               pos = CORE__TaskAtomicLoad_u32(&fifo->DequeuePos, CORE__TASK_ATOMIC_ORDERING_RELAXED);
    uint32_t               seq;
    int32_t               diff;
    for ( ; ; )
    {
        cell = &stor[pos & mask];
        seq  = CORE__TaskAtomicLoad_u32(&cell->Sequence, CORE__TASK_ATOMIC_ORDERING_ACQUIRE);
        diff =(int32_t) seq - (int32_t)(pos + 1);
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
    *value = cell->Value;
    CORE__TaskAtomicStore_u32(&cell->Sequence, pos + mask + 1, CORE__TASK_ATOMIC_ORDERING_RELEASE);
    return 1;
}

#endif /* CORE_TASK_IMPLEMENTATION */

