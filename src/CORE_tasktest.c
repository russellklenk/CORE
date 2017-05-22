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
ResetMPMCQueue
(
    CORE__TASK_MPMC_QUEUE *freeq, 
    uint32_t            capacity, 
    void                 *memory, 
    size_t           memory_size, 
    int             default_char
)
{   /* initialize the memory to a known byte pattern */
    memset(memory, default_char, memory_size);
    return CORE__InitTaskMPMCQueue(freeq, capacity, memory, memory_size);
}

static int
ResetSPMCQueue
(
    CORE__TASK_SPMC_QUEUE *workq, 
    uint32_t            capacity, 
    void                 *memory, 
    size_t           memory_size, 
    int             default_char
)
{   /* initialize the memory to a known byte pattern */
    memset(memory, default_char, memory_size);
    return CORE__InitTaskSPMCQueue(workq, capacity, memory, memory_size);
}

static int
EnsureMPMCQueueMeetsCapacity
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Can push Capacity (%u) items successfully: ", n);
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskMPMCQueuePush of the same item Capacity times succeeds */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskMPMCQueuePush(freeq, 'A') == 0)
        {   /* a push failed; this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueCannotExceedCapacity
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Cannot exceed Capacity (%u) items        : ", n);
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskMPMCQueuePush of the same item Capacity times succeeds */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueuePush(freeq, 'A');
    }
    if (CORE__TaskMPMCQueuePush(freeq, 'B') != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueTakeFailsWhenEmpty
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Cannot take from empty queue                : ");
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskMPMCQueueTake(freeq, &v) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueCanDrain
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Can drain a full queue                      : ");
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskMPMCQueueTake(freeq, &v) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueTakeFailsWhenDrained
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = 0;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Cannot take from drained queue              : ");
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueueTake(freeq, &v);
    }
    if (CORE__TaskMPMCQueueTake(freeq, &v) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueTakeProducesExpectedResult
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = CORE_TASK_TEST_DEFAULT_CHAR;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Take operation produces expected result     : ");
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueuePush(freeq, 'A');
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueueTake(freeq, &v);
        if (v != 'A')
            res = -1;
        v = CORE_TASK_TEST_DEFAULT_CHAR;
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureMPMCQueueTakeProducesItemsInFifoOrder
(
    CORE__TASK_MPMC_QUEUE *freeq
)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t n = freeq->Capacity;
    uint32_t v = CORE_TASK_TEST_DEFAULT_CHAR;
    int    res = 0;

    ConsoleOutput("CORE__TASK_MPMC_QUEUE: Take operation produces items in FIFO order : ");
    ResetMPMCQueue(freeq, freeq->Capacity, freeq->MemoryStart, freeq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0, j = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueuePush(freeq, j + 'A');
        j = (j + 1) % 26;
    }
    /* drain the queue */
    for (i = 0, j = 0; i < n; ++i)
    {
        CORE__TaskMPMCQueueTake(freeq, &v);
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
EnsureSPMCQueueMeetsCapacity
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    uint32_t i = 0;
    uint32_t n = workq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Can push Capacity (%u) items successfully: ", n);
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskSPMCQueuePush of the same item Capacity times succeeds */
    /* pushing more than Capacity items will overwrite the oldest items, but the work 
     * queue has only a fixed number of available items, so this cannot happen. */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueCanExceedCapacity
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    uint32_t i = 0;
    uint32_t n = workq->Capacity;
    int    res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Can exceed Capacity (%u) items           : ", n);
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* make sure that CORE__TaskSPMCQueuePush of the same item Capacity times succeeds */
    /* pushing more than Capacity items will overwrite the oldest items, but the work 
     * queue has only a fixed number of available items, so this cannot happen. */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_INTERNAL, 0, 0, CORE_TASK_ID_VALID));
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueTakeFailsWhenEmpty
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = CORE_INVALID_TASK_ID;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Cannot take from empty queue                : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskSPMCQueueTake(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueStealFailsWhenEmpty
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Cannot steal from empty queue               : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    if (CORE__TaskSPMCQueueSteal(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueCanDrainByTake
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Can drain a full queue by take              : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskSPMCQueueTake(workq, &v, &more) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueCanDrainBySteal
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Can drain a full queue by steal             : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        if (CORE__TaskSPMCQueueSteal(workq, &v, &more) == 0)
        {   /* a take failed - this is unexpected */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueTakeFailsWhenDrained
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Cannot take from drained queue              : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueueTake(workq, &v, &more);
    }
    if (CORE__TaskSPMCQueueTake(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueStealFailsWhenDrained
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Cannot steal from drained queue             : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueueSteal(workq, &v, &more);
    }
    if (CORE__TaskSPMCQueueSteal(workq, &v, &more) != 0)
        res = -1;
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureSPMCQueueTakeProducesExpectedResult
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Take operation produces expected result     : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has the same value */
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueueTake(workq, &v, &more);
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
EnsureSPMCQueueStealProducesExpectedResult
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Steal operation produces expected result    : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has the same value */
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, 1, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueueSteal(workq, &v, &more);
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
EnsureSPMCQueueTakeProducesItemsInLifoOrder
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    uint32_t     j = 0;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Take operation produces items in LIFO order : ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has an increasing slot index value */
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0, j = n-1; i < n; ++i, --j)
    {
        CORE__TaskSPMCQueueTake(workq, &v, &more);
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
EnsureSPMCQueueStealProducesItemsInFifoOrder
(
    CORE__TASK_SPMC_QUEUE *workq
)
{
    CORE_TASK_ID v = 0;
    uint32_t     i = 0;
    uint32_t     n = workq->Capacity;
    int       more = 0;
    int        res = 0;

    ConsoleOutput("CORE__TASK_SPMC_QUEUE: Steal operation produces items in FIFO order: ");
    ResetSPMCQueue(workq, workq->Capacity, workq->MemoryStart, workq->MemorySize, CORE_TASK_TEST_DEFAULT_CHAR);
    /* fill the queue */
    for (i = 0; i < n; ++i)
    {   /* each pushed item has an increasing slot index value */
        CORE__TaskSPMCQueuePush(workq, CORE_MakeTaskId(CORE_TASK_ID_EXTERNAL, 0, i, CORE_TASK_ID_VALID));
    }
    /* drain the queue */
    for (i = 0; i < n; ++i)
    {
        CORE__TaskSPMCQueueSteal(workq, &v, &more);
        if (CORE_TaskIndexInPool(v) != i)
        {   /* the task ID slot index doesn't match the expected value */
            res = -1;
        }
    }
    if (res == 0) ConsoleOutput("PASS.\n");
    else          ConsoleOutput("FAIL.\n");
    return res;
}

static int
EnsureAllPoolsCanBeAcquiredAndReleased
(
    struct _CORE_TASK_POOL_STORAGE *storage, 
    struct _CORE_TASK_POOL_INIT *pool_types,
    uint32_t                pool_type_count
)
{
    struct _CORE_TASK_POOL **pool_list = NULL;
    uint32_t                pool_count = CORE_QueryTaskPoolTotalCount(storage);
    uint32_t                pool_index = 0;
    uint32_t                   i, j, n;
    int                            res = 0;

    ConsoleOutput("CORE__TASK_POOL_STORAGE: Can acquire and release all pools         : ");
    /* allocate temporary memory */
    if ((pool_list = (struct _CORE_TASK_POOL**) malloc(pool_count * sizeof(struct _CORE_TASK_POOL*))) == NULL)
    {
        ConsoleOutput("FAIL.\n");
        return -1;
    }
    /* acquire all pools and collect them in pool_list */
    for (i = 0; i < pool_type_count; ++i)
    {
        for (j = 0, n = pool_types[i].PoolCount; j < n; ++j)
        {
            if ((pool_list[pool_index++] = CORE_AcquireTaskPool(storage, pool_types[i].PoolId)) == NULL)
            {   /* this should have been successful */
                res = -1;
            }
        }
    }
    /* ensure that the free lists are correct (they should all be NULL/empty) */
    for (i = 0; i < pool_type_count; ++i)
    {
        if (storage->PoolFreeList[i] != NULL)
        {   /* expected an empty free list */
            res = -2;
        }
    }
    /* release all pools back to the storage object */
    for (i = 0; i < pool_count; ++i)
    {
        CORE_ReleaseTaskPool(pool_list[i]);
    }
    /* ensure that the free lists are correct */
    for (i = 0; i < pool_type_count; ++i)
    {   /* find the free list for the pool type */
        for (j = 0; j < pool_type_count; ++j)
        {
            if (storage->PoolTypeIds[j] == pool_types[i].PoolId)
            {   /* found the matching free list at index j */
                struct _CORE_TASK_POOL *iter = storage->PoolFreeList[j];
                /* walk the free list for the pool type */
                n = 0;
                while (iter != NULL)
                {
                    iter = iter->NextPool;
                    n++;
                }
                /* make sure that the number of items in the free list match the PoolCount */
                if (n != pool_types[i].PoolCount)
                {   /* the count is unexpected */
                    res = -3;
                }
                break;
            }
        }
    }
    /* free temporary memory */
    free(pool_list);
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
    uint32_t  pool_type_count = 3;
    size_t  pool_storage_size = 0;
    size_t    free_queue_size = CORE__QueryTaskMPMCQueueMemorySize(free_queue_count);
    size_t    work_queue_size = CORE__QueryTaskSPMCQueueMemorySize(work_queue_count);
    void     *free_queue_stor = malloc(free_queue_size);
    void     *work_queue_stor = malloc(work_queue_size);
    void    *pool_storage_mem = NULL;
    int          default_char = '!';
    int                result = 0;  /* success */
    
    CORE__TASK_MPMC_QUEUE     freeq;
    CORE__TASK_SPMC_QUEUE     workq;
    CORE_TASK_CPU_INFO     cpu_info;
    
    CORE_TASK_POOL_INIT  pool_types[3];
    int32_t              type_valid[3];
    int32_t             global_valid;

    struct _CORE_TASK_POOL_STORAGE *storage = NULL;
    CORE_TASK_POOL_STORAGE_INIT storage_init;

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
    if (ResetMPMCQueue(&freeq, free_queue_count, free_queue_stor, free_queue_size, default_char) < 0)
    {
        ConsoleError("ERROR: Failed to initialize CORE__TASK_MPMC_QUEUE.\n");
        result = 1;
    }
    if (ResetSPMCQueue(&workq, work_queue_count, work_queue_stor, work_queue_size, default_char) < 0)
    {
        ConsoleError("ERROR: Failed to initialize CORE__TASK_SPMC_QUEUE.\n");
        result = 1;
    }

    /* end of initial state tests - bail here if anything failed */
    if (result) goto cleanup_and_exit;

    if (EnsureMPMCQueueMeetsCapacity(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueCannotExceedCapacity(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueTakeFailsWhenEmpty(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueCanDrain(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueTakeFailsWhenDrained(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueTakeProducesExpectedResult(&freeq) < 0)
        result = -1;
    if (EnsureMPMCQueueTakeProducesItemsInFifoOrder(&freeq) < 0)
        result = -1;

    if (EnsureSPMCQueueMeetsCapacity(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueCanExceedCapacity(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueTakeFailsWhenEmpty(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueStealFailsWhenEmpty(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueCanDrainByTake(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueCanDrainBySteal(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueTakeFailsWhenDrained(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueStealFailsWhenDrained(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueTakeProducesExpectedResult(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueStealProducesExpectedResult(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueTakeProducesItemsInLifoOrder(&workq) < 0)
        result = -1;
    if (EnsureSPMCQueueStealProducesItemsInFifoOrder(&workq) < 0)
        result = -1;

    if (result) goto cleanup_and_exit;

    /* test pool type validation and pool storage routines */
    ConsoleOutput("\n");
    ConsoleOutput("tasktest: Testing task pool storage functionality.\n");
    pool_types[0].PoolId         = CORE_TASK_POOL_ID_MAIN;
    pool_types[0].PoolCount      = 1;
    pool_types[0].StealThreshold = 0;      /* wake up workers ASAP */
    pool_types[0].MaxActiveTasks = 65536;  /* must be a power-of-two >= 2 */
    pool_types[1].PoolId         = CORE_TASK_POOL_ID_WORKER;
    pool_types[1].PoolCount      = 7;      /* set to number of worker threads */
    pool_types[1].StealThreshold = 1;      /* keep one task for the worker */
    pool_types[1].MaxActiveTasks = 65536;  /* must be a power-of-two >= 2 */
    pool_types[2].PoolId         = CORE_TASK_POOL_ID_USER + 0;
    pool_types[2].PoolCount      = 4;      /* whatever */
    pool_types[2].StealThreshold = 0;      /* these threads never execute tasks */
    pool_types[2].MaxActiveTasks = 512;    /* these threads don't define many tasks */
    if (CORE_ValidateTaskPoolConfiguration(pool_types, type_valid, pool_type_count, &global_valid) < 0)
    {
        ConsoleError("ERROR: Task pool type definitions FAILED to validate.\n");
        result = -1;
    }
    if (result) goto cleanup_and_exit;

    pool_storage_size = CORE_QueryTaskPoolStorageMemorySize(pool_types, pool_type_count);
    ConsoleOutput("_CORE_TASK_POOL_STORAGE: %u tasks requiring %Iu bytes (%IuMB).\n", 526336, pool_storage_size, pool_storage_size / (1024*1024));
    ConsoleOutput("NOTE ******************: It is highly unlikely you need this many tasks.\n");

    if ((pool_storage_mem = malloc(pool_storage_size)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate %Iu bytes for pool storage.\n", pool_storage_size);
        result = -1;
    }
    if (result) goto cleanup_and_exit;

    storage_init.TaskPoolTypes = pool_types;
    storage_init.PoolTypeCount = pool_type_count;
    storage_init.MemoryStart   = pool_storage_mem;
    storage_init.MemorySize    = pool_storage_size;
    if (CORE_CreateTaskPoolStorage(&storage, &storage_init) < 0)
    {
        ConsoleError("ERROR: Failed to create task pool storage.\n");
        result = -1;
    }
    if (result) goto cleanup_and_exit;

    /* now we can perform specific tests that just acquire and release pools from the storage */
    if (EnsureAllPoolsCanBeAcquiredAndReleased(storage, pool_types, pool_type_count) < 0)
        result = -1;

    /* other tests here */
    
    CORE_DeleteTaskPoolStorage(storage);
    free(pool_storage_mem);
    storage = NULL;

cleanup_and_exit:
    ConsoleOutput("\n");
    if (storage != NULL)
    {
        CORE_DeleteTaskPoolStorage(storage);
        free(pool_storage_mem);
    }
    free(work_queue_stor);
    free(free_queue_stor);
    return result;
}

