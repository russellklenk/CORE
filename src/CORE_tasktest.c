/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implement test routines for the CORE_task module.
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

#define CORE_TASK_IMPLEMENTATION

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
#include <intrin.h>

#include "CORE_task.h"

/*//////////////////
//   Data Types   //
//////////////////*/

/*///////////////
//   Globals   //
///////////////*/
#ifndef CORE_TASK_TEST_DEFAULT_CHAR
#define CORE_TASK_TEST_DEFAULT_CHAR    '!'
#endif

/*//////////////////////////
//   Internal Functions   //
//////////////////////////*/
static int
ResetFreeQueue
(
    CORE__TASK_FREE_QUEUE *freeq, 
    uint32_t            capacity, 
    void                 *memory, 
    size_t           memory_size, 
    int             default_char
)
{   /* initialize the memory to a known byte pattern */
    memset(memory, default_char, memory_size);
    return CORE__InitTaskFreeQueue(freeq, capacity, memory, memory_size);
}

static int
ResetWorkQueue
(
    CORE__TASK_WORK_QUEUE *workq, 
    uint32_t            capacity, 
    void                 *memory, 
    size_t           memory_size, 
    int             default_char
)
{   /* initialize the memory to a known byte pattern */
    memset(memory, default_char, memory_size);
    return CORE__InitTaskWorkQueue(workq, capacity, memory, memory_size);
}

static int
EnsureFreeQueueMeetsCapacity
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Can push Capacity (%u) items successfully: ", n);
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskFreeQueuePush of the same item Capacity times succeeds */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskFreeQueuePush(freeq, 'A') == 0)
        {   /* a push failed; this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueCannotExceedCapacity
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Cannot exceed Capacity (%u) items        : ", n);
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskFreeQueuePush of the same item Capacity times succeeds */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueuePush(freeq, 'A');
    }
    if (CORE__TaskFreeQueuePush(freeq, 'B') != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueTakeFailsWhenEmpty
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Cannot take from empty queue                : ");
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskFreeQueueTake(freeq, &v) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueCanDrain
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Can drain a full queue                      : ");
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskFreeQueueTake(freeq, &v) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueTakeFailsWhenDrained
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Cannot take from drained queue              : ");
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueueTake(freeq, &v);
    }
    if (CORE__TaskFreeQueueTake(freeq, &v) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueTakeProducesExpectedResult
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = CORE_TASK_TEST_DEFAULT_CHAR;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Take operation produces expected result     : ");
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskFreeQueueTake(freeq, &v);
        if (v != 'A')
            res = -1;
        v = CORE_TASK_TEST_DEFAULT_CHAR;
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureFreeQueueTakeProducesItemsInFifoOrder
(
    CORE__TASK_FREE_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = CORE_TASK_TEST_DEFAULT_CHAR;
    int    res = 0;

    ConsoleOutput("CORE__TASK_FREE_QUEUE: Take operation produces items in FIFO order : ");
    ResetFreeQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0, j = 0; i < n; ++i)
    {
        CORE__TaskFreeQueuePush(freeq, j + 'A');
        j = (j + 1) % 26;
    }
    /* drain the queue */
    for (i = 0, j = 0; i < n; ++i)
    {
        CORE__TaskFreeQueueTake(freeq, &v);
        if (v != (j + 'A'))
            res = -1;
        v = CORE_TASK_TEST_DEFAULT_CHAR;
        j = (j + 1) % 26;
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueMeetsCapacity
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    uint32_t i = 0;
    uint32_t n = workq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Can push Capacity (%u) items successfully: ", n);
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskWorkQueuePush of the same item Capacity times succeeds */
    /* pushing more than Capacity items will overwrite the oldest items, but the work 
     * queue has only a fixed number of available items, so this cannot happen. */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueCanExceedCapacity
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    uint32_t i = 0;
    uint32_t n = workq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Can exceed Capacity (%u) items           : ", n);
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskWorkQueuePush of the same item Capacity times succeeds */
    /* pushing more than Capacity items will overwrite the oldest items, but the work 
     * queue has only a fixed number of available items, so this cannot happen. */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_INTERNAL, 0, 0, CORE_TASK_ID_VALID));
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueTakeFailsWhenEmpty
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = CORE_INVALID_TASK_ID;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Cannot take from empty queue                : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskWorkQueueTake(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueStealFailsWhenEmpty
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Cannot steal from empty queue               : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskWorkQueueSteal(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueCanDrainByTake
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Can drain a full queue by take              : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskWorkQueueTake(workq, &v, &more) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueCanDrainBySteal
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Can drain a full queue by steal             : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskWorkQueueSteal(workq, &v, &more) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueTakeFailsWhenDrained
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Cannot take from drained queue              : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueueTake(workq, &v, &more);
    }
    if (CORE__TaskWorkQueueTake(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueStealFailsWhenDrained
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Cannot steal from drained queue             : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueueSteal(workq, &v, &more);
    }
    if (CORE__TaskWorkQueueSteal(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueTakeProducesExpectedResult
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Take operation produces expected result     : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has the same value */
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueueTake(workq, &v, &more);
        if (v != CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID))
        {   /* the task ID doesn't match the expected value */
            res = -1;
        }
        if (more == 0 && i != (n-1))
        {   /* the value of more doesn't match the expected value */
            res = -2;
        }
        if (more != 0 && i == (n-1))
        {   /* the value of more doesn't match the expected value */
            res = -3;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL %d.\n", res);
    return res;
}

static int
EnsureWorkQueueStealProducesExpectedResult
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Steal operation produces expected result    : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has the same value */
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueueSteal(workq, &v, &more);
        if (v != CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID))
        {   /* the task ID doesn't match the expected value */
            res = -1;
        }
        if (more == 0 && i != (n-1))
        {   /* the value of more doesn't match the expected value */
            res = -2;
        }
        if (more != 0 && i == (n-1))
        {   /* the value of more doesn't match the expected value */
            res = -3;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL %d.\n", res);
    return res;
}

static int
EnsureWorkQueueTakeProducesItemsInLifoOrder
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    uint32_t     j = 0;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Take operation produces items in LIFO order : ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has an increasing slot index value */
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0, j = n-1; i < n; ++i, --j)
    {
        CORE__TaskWorkQueueTake(workq, &v, &more);
        if (CORE_TaskIndexInPool(v) != j)
        {   /* the task ID slot index doesn't match the expected value */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureWorkQueueStealProducesItemsInFifoOrder
(
    CORE__TASK_WORK_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_WORK_QUEUE: Steal operation produces items in FIFO order: ");
    ResetWorkQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has an increasing slot index value */
        CORE__TaskWorkQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskWorkQueueSteal(workq, &v, &more);
        if (CORE_TaskIndexInPool(v) != i)
        {   /* the task ID slot index doesn't match the expected value */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

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
    uint32_t free_queue_count = 65536;
    uint32_t work_queue_count = 65536;
    size_t    free_queue_size = CORE__QueryTaskFreeQueueMemorySize(free_queue_count);
    size_t    work_queue_size = CORE__QueryTaskWorkQueueMemorySize(work_queue_count);
    void     *free_queue_stor = malloc(free_queue_size);
    void     *work_queue_stor = malloc(work_queue_size);
    int          default_char = '!';
    int                result = 0;  /* success */
    CORE__TASK_FREE_QUEUE     freeq;
    CORE__TASK_WORK_QUEUE     workq;
    CORE_TASK_CPU_INFO     cpu_info;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    ConsoleOutput("tasktest: Reporting host system CPU information.\n");
    if (CORE_QueryHostCpuInfo(&cpu_info) < 0)
    {
        ConsoleOutput("ERROR: Failed to retrieve host system CPU information.\n");
        result = 1;
    }
    else
    {
        ConsoleOutput("NUMA node count    : %u\n", cpu_info.NumaNodes);
        ConsoleOutput("Physical CPU count : %u\n", cpu_info.PhysicalCPUs);
        ConsoleOutput("Physical CPU cores : %u\n", cpu_info.PhysicalCores);
        ConsoleOutput("Hardware threads   : %u\n", cpu_info.HardwareThreads);
        ConsoleOutput("Threads-per-core   : %u\n", cpu_info.ThreadsPerCore);
        ConsoleOutput("L1 cache size      : %u\n", cpu_info.CacheSizeL1);
        ConsoleOutput("L1 cacheline size  : %u\n", cpu_info.CacheLineSizeL1);
        ConsoleOutput("L2 cache size      : %u\n", cpu_info.CacheSizeL2);
        ConsoleOutput("L2 cacheline size  : %u\n", cpu_info.CacheLineSizeL2);
        ConsoleOutput("Prefer AMD OpenCL  : %d\n", cpu_info.PreferAMD);
        ConsoleOutput("Prefer Intel OpenCL: %d\n", cpu_info.PreferIntel);
        ConsoleOutput("Is virtual machine : %d\n", cpu_info.IsVirtualMachine);
        ConsoleOutput("CPU vendor string  : %S\n", cpu_info.VendorName);
        if (cpu_info.CacheLineSizeL1 > CORE_TASK_L1_CACHELINE_SIZE)
        {
            ConsoleOutput("WARNING: For best performance, recompiled with /DCORE_TASK_L1_CACHELINE_SIZE=%u\n", cpu_info.CacheLineSizeL1);
        }
        ConsoleOutput("\n");
    }

    if (result) goto cleanup_and_exit;

    ConsoleOutput("tasktest: Testing underlying queue functionality.\n");
    ConsoleOutput("freeq capacity: %u items requiring %Iu bytes.\n", free_queue_count, free_queue_size);
    ConsoleOutput("workq capacity: %u items requiring %Iu bytes.\n", work_queue_count, work_queue_size);
    if (ResetFreeQueue(&freeq, free_queue_count, free_queue_stor, free_queue_size, default_char) < 0)
    {
        ConsoleError("ERROR: Failed to initialize CORE__TASK_FREE_QUEUE.\n");
        result = 1;
    }
    if (ResetWorkQueue(&workq, work_queue_count, work_queue_stor, work_queue_size, default_char) < 0)
    {
        ConsoleError("ERROR: Failed to initialize CORE__TASK_WORK_QUEUE.\n");
        result = 1;
    }

    /* end of initial state tests - bail here if anything failed */
    if (result) goto cleanup_and_exit;

    if (EnsureFreeQueueMeetsCapacity(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueCannotExceedCapacity(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueTakeFailsWhenEmpty(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueCanDrain(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueTakeFailsWhenDrained(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueTakeProducesExpectedResult(&freeq) < 0)
        result = -1;
    if (EnsureFreeQueueTakeProducesItemsInFifoOrder(&freeq) < 0)
        result = -1;

    if (EnsureWorkQueueMeetsCapacity(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueCanExceedCapacity(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueTakeFailsWhenEmpty(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueStealFailsWhenEmpty(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueCanDrainByTake(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueCanDrainBySteal(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueTakeFailsWhenDrained(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueStealFailsWhenDrained(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueTakeProducesExpectedResult(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueStealProducesExpectedResult(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueTakeProducesItemsInLifoOrder(&workq) < 0)
        result = -1;
    if (EnsureWorkQueueStealProducesItemsInFifoOrder(&workq) < 0)
        result = -1;

    if (result) goto cleanup_and_exit;

cleanup_and_exit:
    ConsoleOutput("\n");
    free(work_queue_stor);
    free(free_queue_stor);
    return result;
}

