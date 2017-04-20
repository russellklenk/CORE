/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implement the entry point and main loop of the test application.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Define some useful macros for specifying common resource sizes.
#ifndef Kilobytes
    #define Kilobytes(x)                            (((size_t)(x)) * ((size_t)1024))
#endif
#ifndef Megabytes
    #define Megabytes(x)                            (((size_t)(x)) * ((size_t)1024) * ((size_t)1024))
#endif
#ifndef Gigabytes
    #define Gigabytes(x)                            (((size_t)(x)) * ((size_t)1024) * ((size_t)1024) * ((size_t)1024))
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

#define CORE_MEMORY_IMPLEMENTATION

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

#include "CORE_memory.h"

/*//////////////////
//   Data Types   //
//////////////////*/

/*///////////////
//   Globals   //
///////////////*/

/*//////////////////////////
//   Internal Functions   //
//////////////////////////*/
static uint64_t
TimestampInTicks
(
    void
)
{
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return (uint64_t) ticks.QuadPart;
}

static uint64_t
TimestampInNanoseconds
(
    void
)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    QueryPerformanceFrequency(&freq);
    return (1000000000ULL * (uint64_t)(ticks.QuadPart)) / (uint64_t)(freq.QuadPart);
}

static uint64_t
NanosecondSliceOfSecond
(
    uint64_t fraction
)
{
    return 1000000000ULL / fraction;
}

static uint64_t
ElapsedNanoseconds
(
    uint64_t start_ticks, 
    uint64_t   end_ticks
)
{   
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    // scale the tick value by the nanoseconds-per-second multiplier
    // before scaling back down by ticks-per-second to avoid loss of precision.
    return (1000000000ULL * (end_ticks - start_ticks)) / (uint64_t)(freq.QuadPart);
}

static uint64_t
MillisecondsToNanoseconds
(
    uint32_t milliseconds
)
{
    return ((uint64_t)(milliseconds)) * 1000000ULL;
}

static uint32_t
NanosecondsToWholeMilliseconds
(
    uint64_t nanoseconds
)
{
    return (uint32_t)(nanoseconds / 1000000ULL);
}

static void
DumpBlock
(
    CORE_MEMORY_BLOCK *b
)
{
    ConsoleOutput("BLOCK @ %10I64u 0x%08p %10I64u bytes\n", b->BlockOffset, b->HostAddress, b->SizeInBytes);
}

static void
InitStatusIndex
(
    void  *status_index, /* the raw status index data */
    uint64_t index_size  /* in bytes */
)
{
    CORE_FillMemory(status_index, (size_t) index_size, 0xFF);
}

static void
DumpStatusIndex
(
    uint32_t level_count,  /* the number of levels in the allocator */
    void   *status_index,  /* the raw status index data */
    uint64_t  index_size   /* in bytes */
)
{   assert(level_count   > 0);
    assert(index_size    > 0);
    assert(status_index != NULL);

    uint8_t        *status_bits =(uint8_t*) status_index;
    uint32_t global_block_index = 0;
    uint32_t global_block_count = 1UL << level_count;
    uint32_t  level_block_count = 0;
    uint32_t  level_block_index = 0;
    uint32_t        level_index = 0;
    uint32_t          bit_index = 0;
    uint8_t                bits = *status_bits;

    (void) index_size;
    (void) global_block_count;

    ConsoleOutput("Status Index (%02u levels): ", level_count);
    while (level_index < level_count)
    {   assert(global_block_index < global_block_count);
        level_block_count  = 1UL << level_index;
        level_block_index  = 0;
        while (level_block_index  < level_block_count)
        {
            if (bits & (1 << bit_index++))
                ConsoleOutput("1");
            else
                ConsoleOutput("0");
            
            if (bit_index == 8)
            {
                bit_index = 0;
                status_bits++;
                bits = *status_bits;
            }
            global_block_index++;
            level_block_index++;
        }
        if (level_index != (level_count - 1))
            ConsoleOutput(" | ");
        level_index++;
    }
    ConsoleOutput("\n");
}

static void
DumpSplitIndex
(
    uint32_t level_count,  /* the number of levels in the allocator */
    void    *split_index,  /* the raw split index data */
    uint64_t  index_size   /* in bytes */
)
{   assert(level_count  > 0);
    assert(index_size   > 0);
    assert(split_index != NULL);

    uint8_t         *split_bits =(uint8_t*) split_index;
    uint32_t global_block_index = 0;
    uint32_t global_block_count = 1UL << (level_count - 1); /* no split index data for last level */
    uint32_t  level_block_count = 0;
    uint32_t  level_block_index = 0;
    uint32_t        level_index = 0;
    uint32_t          bit_index = 0;
    uint8_t                bits = *split_bits;

    (void) index_size;
    (void) global_block_count;

    ConsoleOutput("Split  Index (%02u levels): ", (level_count-1));
    while (level_index < (level_count-1))
    {   assert(global_block_index < global_block_count);
        level_block_count  = 1UL << level_index;
        level_block_index  = 0;
        while (level_block_index  < level_block_count)
        {
            if (bits & (1 << bit_index++))
                ConsoleOutput("1");
            else
                ConsoleOutput("0");
            
            if (bit_index == 8)
            {
                bit_index = 0;
                split_bits++;
                bits = *split_bits;
            }
            global_block_index++;
            level_block_index++;
        }
        if ((level_index + 1) != level_count)
            ConsoleOutput(" | ");
        level_index++;
    }
    ConsoleOutput("\n");
}

static void
DumpFreeCounts
(
    uint32_t *free_counts, 
    uint32_t  level_count
)
{
    uint32_t level_index;

    ConsoleOutput("Free  Counts (%02u levels): ", level_count);
    for (level_index = 0; level_index < level_count; ++level_index)
    {
        ConsoleOutput("%06u ", free_counts[level_index]);
    }
    ConsoleOutput("\n");
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
    void                       **addrs;
    void                        *state_data;
    size_t                       state_size;
    size_t                       max_blocks;
    size_t                       block_index;
    CORE_HOST_MEMORY_POOL        host_pool;
    CORE_HOST_MEMORY_POOL_INIT   host_pool_init;
    CORE_HOST_MEMORY_ALLOCATION *host_metadata_mem;
    CORE_HOST_MEMORY_ALLOCATION *host_mem;
    CORE_MEMORY_ARENA            host_metadata_arena;
    CORE_MEMORY_ARENA_INIT       host_metadata_arena_init;
    CORE_MEMORY_ALLOCATOR        host_alloc;
    CORE_MEMORY_ALLOCATOR_INIT   host_alloc_init;
    CORE_MEMORY_BLOCK            b, b1;
    uint64_t                     time_s;
    uint64_t                     time_e;
    uint64_t                     block_size;
    uint64_t                     block_offset;
    uint32_t                     global_level_index;
    uint32_t                     global_level_count;
    uint32_t                     level_block_count;
    uint32_t                     level_block_index;
    uint32_t                     prior_block_count;
    uint32_t                     i;
    int                          res;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    /* each thread typically has its own CORE_HOST_MEMORY_POOL. set one up. */
    host_pool_init.PoolName           = "Main Thread Host Pool"; /* PoolName should point to a string literal. */
    host_pool_init.PoolCapacity       = 512;                     /* The maximum number of VMM allocations this thread will have live at any one time. */
    host_pool_init.MinAllocationSize  = 0;                       /* Use the default (VMM page size). */
    host_pool_init.MinCommitIncrease  = 0;                       /* No minimum commit increase.      */
    host_pool_init.MaxTotalCommitment = Megabytes(16);           /* Don't exceed 16MB of committed memory across all allocations in the pool. */
    if (CORE_CreateHostMemoryPool(&host_pool, &host_pool_init) < 0)
    {
        ConsoleError("ERROR: Failed to initialize host memory pool (%08X).\n", GetLastError());
        return 1;
    }
    /* allocate two blocks of memory.
     * the first is used for data structure maintenence.
     * the second is used for storing actual data used by the application.
     */
    if ((host_metadata_mem = CORE_HostMemoryPoolAllocate(&host_pool, Megabytes(2), Megabytes(2), 0)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate 2MB metadata memory (%08X).\n", GetLastError());
        return 1;
    }
    if ((host_mem = CORE_HostMemoryPoolAllocate(&host_pool, Megabytes(14), Megabytes(14), 0)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate 14MB host memory (%08X).\n", GetLastError());
        return 1;
    }

    /* initialize a memory arena used for allocating overhead memory for data structures */
    host_metadata_arena_init.AllocatorName = "Metadata";                                   /* Assign a string literal for debugging */
    host_metadata_arena_init.AllocatorType = CORE_MEMORY_ALLOCATOR_TYPE_HOST;              /* The arena manages host-visible memory */
    host_metadata_arena_init.MemoryStart   =(uint64_t) host_metadata_mem->BaseAddress;     /* The host-visible address representing the start of the memory block */
    host_metadata_arena_init.MemorySize    =(uint64_t) host_metadata_mem->BytesCommitted;  /* The size of the memory block, in bytes */
    host_metadata_arena_init.UserData      = &host_metadata_mem;                           /* Specify any data you want, up to CORE_MEMORY_ALLOCATOR_MAX_USER bytes */
    host_metadata_arena_init.UserDataSize  = sizeof(CORE_HOST_MEMORY_ALLOCATION*);         /* The size of the extra data you want to associate with the allocator instance */
    if (CORE_InitMemoryArena(&host_metadata_arena, &host_metadata_arena_init) < 0)
    {
        ConsoleError("ERROR: Failed to initialize host metadata memory arena (%08X).\n", GetLastError());
        return 1;
    }

#define UNITS_MIN    Kilobytes
#define UNITS_MAX    Kilobytes
#define VIRTUAL_SIZE 16 * 1024
#define RESERVE_SIZE 0
#define MINIMUM_SIZE 16
#define MAXIMUM_SIZE VIRTUAL_SIZE
#define ACTUAL_SIZE (UNITS_MAX(VIRTUAL_SIZE)-UNITS_MAX(RESERVE_SIZE))

    /* figure out how much memory we need to store the allocator state for our general-purpose allocator config.
     * the minimum and maximum allocation size must be powers of two, but we only have 14MB to work with.
     * to get around this, we specify a maximum size of 16MB (a power of two) and then use BytesReserved.
     */
    if ((state_size = CORE_QueryMemoryAllocatorStateSize(UNITS_MIN(MINIMUM_SIZE), UNITS_MAX(MAXIMUM_SIZE))) == 0)
    {
        ConsoleError("ERROR: Failed to determine memory allocator state data requirement.\n");
        return 1;
    }
    if ((state_data = CORE_MemoryArenaAllocateHost(&host_metadata_arena, state_size, CORE_AlignOf(CORE_MEMORY_ALLOCATOR), NULL)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate %Iu bytes for general-purpose allocator state.\n", state_size);
        return 1;
    }

    /* allocate some memory for storing host addresses for performance testing purposes */
    if ((addrs = CORE_MemoryArenaAllocateHost(&host_metadata_arena, UNITS_MAX(MAXIMUM_SIZE) / UNITS_MIN(MINIMUM_SIZE) * sizeof(void*), CORE_AlignOf(void*), NULL)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate performance testing memory.\n");
        return 1;
    }


    /* initialize a general-purpose memory allocator */
    host_alloc_init.AllocatorName     = "Main Data";                           /* Assign a string literal for debugging */
    host_alloc_init.AllocatorType     = CORE_MEMORY_ALLOCATOR_TYPE_HOST;       /* The allocator manages host-visible memory */
    host_alloc_init.AllocationSizeMin = UNITS_MIN(MINIMUM_SIZE);               /* Minimum allocation size. Must be a power-of-two. */
    host_alloc_init.AllocationSizeMax = UNITS_MAX(MAXIMUM_SIZE);               /* Maximum allocation size. Must be a power-of-two. */
    host_alloc_init.BytesReserved     = UNITS_MIN(RESERVE_SIZE);               /* Reserve 2MB, because we only have 14MB to work with. */
    host_alloc_init.MemoryStart       =(uint64_t) host_mem->BaseAddress;       /* The host-visible address of representing the start of the memory block. */
    host_alloc_init.MemorySize        =(uint64_t) ACTUAL_SIZE;                 /* The actual amount of addressable memory. */
    host_alloc_init.StateData         = state_data;                            /* Space for the allocator state data. */
    host_alloc_init.StateDataSize     = state_size;                            /* The number of bytes allocated for allocator state data. */
    host_alloc_init.UserData          = &host_mem;                             /* Specify any data you want here, up to CORE_MEMORY_ALLOCATOR_MAX_USER bytes. */
    host_alloc_init.UserDataSize      = sizeof(CORE_HOST_MEMORY_ALLOCATION*);  /* The size of the extra data you want to associate with the allocator instance. */
    if (CORE_InitMemoryAllocator(&host_alloc, &host_alloc_init) < 0)
    {
        ConsoleError("ERROR: Failed to initialize host memory allocator (%08X).\n", GetLastError());
        return 1;
    }

    /* sanity check all of the internal functions first.
     * some of these tests don't make sense from the perspective of an allocator, 
     * but we want to verify that the index manipulation functions work as intended.
     */
#if 0
    block_index        = 0; (void) b; (void) max_blocks;
    global_level_count = host_alloc.LevelCount;
    global_level_index = 0;
    prior_block_count  = 0;
    InitStatusIndex(host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    while (global_level_index < global_level_count)
    {
        level_block_count = 1UL << global_level_index;
        level_block_index = 0;
        block_offset      = 0;
        block_size        = host_alloc.MemorySize / level_block_count;
        while (level_block_index < level_block_count)
        {
            CORE__MEMORY_BLOCK_INFO b1, b2;
            res = CORE__FindFreeMemoryBlockAtLevel(&b1, &host_alloc, global_level_index);
            assert(res == 1 && "Failed to find free block when we should have");
            assert(b1.BlockSize == block_size);
            assert(b1.BlockOffset == block_offset);
            assert(b1.LevelIndex == global_level_index);
            assert(b1.BlockCount == level_block_count);
            assert(b1.AbsoluteIndexOffset == prior_block_count);
            assert(b1.BlockAbsoluteIndex == block_index);

            /* make sure that both query routines return the same information for the same block */
            CORE__QueryMemoryBlockInfoWithKnownLevel(&b2, &host_alloc, block_offset, global_level_index);
            assert(b1.BlockOffset == b2.BlockOffset);
            assert(b1.BlockSize == b2.BlockSize);
            assert(b1.LevelIndex == b2.LevelIndex);
            assert(b1.BlockCount == b2.BlockCount);
            assert(b1.AbsoluteIndexOffset == b2.AbsoluteIndexOffset);
            assert(b1.BlockAbsoluteIndex == b2.BlockAbsoluteIndex);
            assert(b1.BuddyAbsoluteIndex == b2.BuddyAbsoluteIndex);
            assert(b1.LowerAbsoluteIndex == b2.LowerAbsoluteIndex);
            assert(b1.IndexWord == b2.IndexWord);
            assert(b1.IndexMask == b2.IndexMask);

            /* mark the block as being in-use */
            CORE__MarkMemoryBlockUsed(&b1, &host_alloc);

            DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
            block_offset += block_size;
            level_block_index++;
            block_index++;
        }
        prior_block_count += level_block_count;
        global_level_index++;
    }
#endif

    /* re-initialize the memory allocator state since we mucked with it above */
#if 0
    CORE_InitMemoryAllocator(&host_alloc, &host_alloc_init);
#endif

    /*ConsoleOutput("INITIAL STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/

    /* allocate as many blocks as will fit in the available storage */
    /*ConsoleOutput("ALLOCATIONS\n\n");*/
    max_blocks = ACTUAL_SIZE / UNITS_MIN(MINIMUM_SIZE);
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        res = CORE_MemoryAllocate(&host_alloc, UNITS_MIN(MINIMUM_SIZE), 0, &b);
        assert(res == 0 && "Memory allocation failed when it should have succeeded");
        assert((uint64_t) b.HostAddress >= host_alloc_init.MemoryStart);
        assert(b.BlockOffset >= 0);
        assert(b.BlockOffset < (host_alloc_init.MemoryStart + host_alloc_init.MemorySize));
        /*DumpBlock(&b);
        DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
        DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
        DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
        ConsoleOutput("\n");*/
    }
    /* the next allocation attempt had better fail */
    res = CORE_MemoryAllocate(&host_alloc, UNITS_MIN(MINIMUM_SIZE), 0, &b);
    assert(res == -1 && "Memory allocation succeeded when it should have failed");
    /*ConsoleOutput("FREES\n\n");*/
    /* free all blocks */
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        b.SizeInBytes   =  UNITS_MIN(MINIMUM_SIZE);
        b.BlockOffset   = (block_index) * UNITS_MIN(MINIMUM_SIZE);
        b.HostAddress   = (void*)(uintptr_t)(host_alloc_init.MemoryStart + b.BlockOffset);
        b.AllocatorType =  CORE_MEMORY_ALLOCATOR_TYPE_HOST;
        CORE_MemoryFreeHost(&host_alloc, b.HostAddress);
        /*DumpBlock(&b);
        DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
        DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
        DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
        ConsoleOutput("\n");*/
    }

    /*ConsoleOutput("FINAL STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/

    CORE_MemoryAllocatorReset(&host_alloc);

    /*ConsoleOutput("RESET STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/

    /* test block demotion - a large block is demoted to a smaller block */
    res = CORE_MemoryAllocate(&host_alloc, UNITS_MAX(MAXIMUM_SIZE)>>1, 0, &b);
    assert(res == 0 && "Memory allocation failed when it should have succeeded");
    /*ConsoleOutput("PRE-DEMOTION STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/
    res = CORE_MemoryReallocate(&host_alloc, &b, UNITS_MIN(MINIMUM_SIZE), 0, &b1);
    assert(res == 0 && "Memory reallocation (demotion) failed when it should have succeeded");
    assert(b1.BlockOffset == b.BlockOffset && "Demoted block changed offsets");
    assert(b1.SizeInBytes == UNITS_MIN(MINIMUM_SIZE) && "Demoted block has unexpected size");
    /*ConsoleOutput("POST-DEMOTION STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/
    /* test block promotion - a smaller block is promoted to a larger block */
    res = CORE_MemoryReallocate(&host_alloc, &b1, (size_t)(b1.SizeInBytes * 2), 0, &b);
    assert(res == 0 && "Memory reallocation (promotion) failed when it should have succeeded");
    assert(b.SizeInBytes == b1.SizeInBytes * 2 && "Promoted block has unexpected size");
    assert(b.BlockOffset == b1.BlockOffset && "Promoted block changed offsets"); /* though this may happen in some cases */
    /*ConsoleOutput("POST-PROMOTION STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/

    /* free the allocated block */
    CORE_MemoryFree(&host_alloc, &b);
    /*ConsoleOutput("PRE-DOUBLE FREE STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/
    /* test a double-free */
    CORE_MemoryFree(&host_alloc, &b);
    /*ConsoleOutput("POST-DOUBLE FREE STATE\n");
    DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
    DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    DumpFreeCounts (host_alloc.FreeCount , host_alloc.LevelCount);
    ConsoleOutput("\n");*/

    /* ... */

    /* performance testing */
    CORE_MemoryAllocatorReset(&host_alloc);
    max_blocks = ACTUAL_SIZE / UNITS_MIN(MINIMUM_SIZE);
    time_s = TimestampInTicks();
    for (i = 0; i < 100000; ++i)
    {
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            if ((res = CORE_MemoryAllocate(&host_alloc, UNITS_MIN(MINIMUM_SIZE), 0, &b)) == 0)
            {   /* save the address so it can be freed later */
                addrs[block_index] = b.HostAddress;
            }
            {
                assert(res == 0 && "Memory allocation failed when it should have succeeded");
            }
        }
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            b.SizeInBytes   =  UNITS_MIN(MINIMUM_SIZE);
            b.BlockOffset   = (block_index) * UNITS_MIN(MINIMUM_SIZE);
            b.HostAddress   = (void*)(uintptr_t)(host_alloc_init.MemoryStart + b.BlockOffset);
            b.AllocatorType =  CORE_MEMORY_ALLOCATOR_TYPE_HOST;
            CORE_MemoryFree(&host_alloc, &b);
        }
    }
    time_e = TimestampInTicks();
    ConsoleOutput("CORE allocator took %u milliseconds.\n", NanosecondsToWholeMilliseconds(ElapsedNanoseconds(time_s, time_e)));
    CORE_MemoryAllocatorReset(&host_alloc);

    time_s = TimestampInTicks();
    for (i = 0; i < 10000; ++i)
    {
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            void *p;
            if  ((p = malloc(UNITS_MIN(MINIMUM_SIZE))) != NULL)
            {
                addrs[block_index] = p;
            }
            else
            {
                assert(p != NULL && "Memory allocation failed when it should have succeeded");
            }
        }
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            free(addrs[block_index]);
        }
    }
    time_e = TimestampInTicks();
    ConsoleOutput("malloc took %u milliseconds.\n", NanosecondsToWholeMilliseconds(ElapsedNanoseconds(time_s, time_e)));

    time_s = TimestampInTicks();
    for (i = 0; i < 10000; ++i)
    {
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            void *p;
            if  ((p = VirtualAlloc(NULL, UNITS_MIN(MINIMUM_SIZE), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE)) != NULL)
            {
                addrs[block_index] = p;
            }
            else
            {
                assert(p != NULL && "Memory allocation failed when it should have succeeded");
            }
        }
        for (block_index = 0; block_index < max_blocks; ++block_index)
        {
            VirtualFree(addrs[block_index], 0, MEM_RELEASE);
        }
    }
    time_e = TimestampInTicks();
    ConsoleOutput("VirtualAlloc took %u milliseconds.\n", NanosecondsToWholeMilliseconds(ElapsedNanoseconds(time_s, time_e)));

    /* cleanup. generally you can just call CORE_DeleteHostMemoryPool, but this is test code. */
    CORE_HostMemoryRelease(host_mem);
    CORE_HostMemoryRelease(host_metadata_mem);
    CORE_DeleteHostMemoryPool(&host_pool);
    return 0;
}

