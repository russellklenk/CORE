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

/* @summary Define the data associated with a pre-allocated, fixed-size pool of tasks.
 * Each task pool is associated with a single thread that submits and optionally executes tasks.
 */
#if 0
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
#endif

/* @summary Define some identifiers that may be passed to CORE_MakeTaskId for the _type argument to make the code more readable.
 */
typedef enum _CORE_TASK_ID_TYPE {
    CORE_TASK_ID_EXTERNAL            =  0,                  /* The task is completed by an external event, such as an I/O operation. */
    CORE_TASK_ID_INTERNAL            =  1,                  /* The task is completed internally by executing the task entry point. */
} CORE_TASK_ID_TYPE;

/* @summary Define some identifiers that may be passed to CORE_MakeTaskId for the _valid argument to make the code more readable.
 */
typedef enum _CORE_TASK_ID_VALIDITY {
    CORE_TASK_ID_INVALID             =  0,                  /* The task identifier is not valid. */
    CORE_TASK_ID_VALID               =  1,                  /* The task identifier is valid. */
} CORE_TASK_ID_VALIDITY;

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

/* @summary Define the data associated with a single item in an MPMC concurrent queue.
 * These items store the zero-based index of a free task slot within the task pool.
 */
typedef struct _CORE__TASK_FREE_CELL {
    uint32_t                              Sequence;          /* The sequence number assigned to the cell. */
    uint32_t                              TaskIndex;         /* The value stored in the cell. This is the zero-based index of an available task slot. */
} CORE__TASK_FREE_CELL;

/* @summary Define the data associated with a semaphore guaranteed to stay in userspace unless a thread needs to be downed or woken.
 */
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_SEMAPHORE {
    #define PAD_SIZE                      CORE__TASK_SEMAPHORE_PADDING_SIZE
    HANDLE                                Semaphore;         /* The operating system semaphore object. */
    int32_t                               Count;             /* The current count. */
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
typedef struct CORE_TASK_CACHELINE_ALIGN _CORE__TASK_FREE_QUEUE {
    #define PAD_SHARED                    CORE__TASK_FREE_QUEUE_PADDING_SIZE_SHARED
    #define PAD_ENQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    #define PAD_DEQUEUE                   CORE__TASK_FREE_QUEUE_PADDING_SIZE_INDEX
    CORE__TASK_FREE_CELL                 *Storage;           /* Storage for queue items. Fixed-size, power-of-two capacity. */
    uint32_t                              StorageMask;       /* The mask used to map EnqueuePos and DequeuePos into the storage array. */
    uint32_t                              Capacity;          /* The maximum number of items that can be stored in the queue. */
    void                                 *MemoryStart;       /* The pointer to the start of the allocated memory block. */
    size_t                                MemorySize;        /* The size of the allocated memory block, in bytes. */
    uint8_t                               Pad0[PAD_SHARED];  /* Padding separating shared data from producer-only data. */
    uint32_t                              EnqueuePos;        /* A monotonically-increasing integer representing the position of the next enqueue operation. */
    uint8_t                               Pad1[PAD_ENQUEUE]; /* Padding separating the producer-only data from the consumer-only data. */
    uint32_t                              DequeuePos;        /* A monotonically-increasing integer representing the position of the next dequeue operation. */
    uint8_t                               Pad2[PAD_DEQUEUE]; /* Padding separating the consumer-only data from adjacent data. */
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
    CORE_TASK_ID                         *Storage;           /* Storage for queue items (ready-to-run task identifiers.) Fixed-size, power-of-two capacity. */
    uint32_t                              StorageMask;       /* The mask used to map PublicPos and PrivatePos into the storage array. */
    uint32_t                              Capacity;          /* The maximum number of items that can be stored in the queue. */
    void                                 *MemoryStart;       /* The pointer to the start of the allocated memory block. */
    size_t                                MemorySize;        /* The size of the allocated memory block, in bytes. */
    uint8_t                               Pad0[PAD_SHARED];  /* Padding separating shared data from the public-only end of the queue. */
    int64_t                               PublicPos;         /* A monotonically-increasing integer representing the public end of the dequeue, updated by Steal operations. */
    uint8_t                               Pad1[PAD_PUBLIC];  /* Padding separating the public-only data from the private-only data. */
    int64_t                               PrivatePos;        /* A monotonically-increasing integer representing the private end of the dequeue, updated by Push and Take operations. */
    uint8_t                               Pad2[PAD_PRIVATE]; /* Padding separating the private-only data from adjacent data. */
    #undef  PAD_SHARED
    #undef  PAD_PUBLIC
    #undef  PAD_PRIVATE
} CORE__TASK_WORK_QUEUE;

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
 * @return Zero if the semaphore is successfully initialized, or -1 if an error occurred.
 */
static int
CORE__TaskCreateSemaphore
(
    CORE__TASK_SEMAPHORE *sem, 
    int32_t             count
)
{
    if ((sem->Semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL)) != NULL)
    {   /* the semaphore is successfully initialized */
        sem->Count = count;
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
    return (capacity * sizeof(CORE__TASK_FREE_CELL));
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
    if (memory_size < (capacity * sizeof(CORE__TASK_FREE_CELL)))
    {   /* the caller did not supply enough memory */
        assert(memory_size >= (capacity * sizeof(CORE__TASK_FREE_CELL)));
        ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
        return -1;
    }

    ZeroMemory(fifo, sizeof(CORE__TASK_FREE_QUEUE));
    fifo->Storage     =(CORE__TASK_FREE_CELL*) memory;
    fifo->StorageMask =(uint32_t) (capacity-1);
    fifo->Capacity    = capacity;
    fifo->MemoryStart = memory;
    fifo->MemorySize  = memory_size;
    for (i = 0; i < capacity; ++i)
    {   /* set the sequence number for each cell */
        CORE__TaskAtomicStore_u32(&fifo->Storage[i].Sequence, i, CORE__TASK_ATOMIC_ORDERING_RELAXED);
        fifo->Storage[i].TaskIndex = i;
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
    CORE__TASK_FREE_CELL *cell;
    CORE__TASK_FREE_CELL *stor = fifo->Storage;
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
    cell->TaskIndex = task_index;
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
    CORE__TASK_FREE_CELL *cell;
    CORE__TASK_FREE_CELL *stor = fifo->Storage;
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
   *task_index = cell->TaskIndex;
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

#endif /* CORE_TASK_IMPLEMENTATION */

