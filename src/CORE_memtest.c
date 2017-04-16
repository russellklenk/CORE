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

    ConsoleOutput("Split Index  (%02u levels): ", (level_count-1));
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

#if 0
static void
DumpMergeIndex
(
    CORE_MEMORY_ALLOCATOR *alloc
)
{
    uint32_t    i, j;
    uint32_t     bit;
    uint32_t bit_idx = 0;
    uint32_t   nbits = 0;
    uint32_t nlevels = alloc->LevelCount;
    uint32_t  *index = alloc->MergeIndex;
    uint32_t    word =*index;
    ConsoleOutput("MERGE INDEX:\n");
    ConsoleOutput("L00: -\n");
    for (i = 1; i < nlevels; ++i)
    {
        nbits = 1 << (i - 1);
        ConsoleOutput("L%02u:",  i);
        for (j = 0; j < nbits; ++j)
        {
            bit = (word >> bit_idx) & 0x1;
            bit_idx++;
            ConsoleOutput("%u", bit);
            if (bit_idx == 32)
            {
                index++;
                word = *index;
                bit_idx = 0;
            }
        }
        ConsoleOutput("\n");
    }
}

static void
DumpSplitIndex
(
    CORE_MEMORY_ALLOCATOR *alloc
)
{
    uint32_t    i, j;
    uint32_t     bit;
    uint32_t bit_idx = 0;
    uint32_t   nbits = 0;
    uint32_t nlevels = alloc->LevelCount;
    uint32_t  *index = alloc->SplitIndex;
    uint32_t    word =*index;
    ConsoleOutput("SPLIT INDEX:\n");
    for (i = 0; i < nlevels - 1; ++i)
    {
        nbits = 1 << i;
        ConsoleOutput("L%02u:",  i);
        for (j = 0; j < nbits; ++j)
        {
            bit = (word >> bit_idx) & 0x1;
            bit_idx++;
            ConsoleOutput("%u", bit);
            if (bit_idx == 32)
            {
                index++;
                word = *index;
                bit_idx = 0;
            }
        }
        ConsoleOutput("\n");
    }
}

static void
DumpFreeLists
(
    CORE_MEMORY_ALLOCATOR *alloc
)
{
    uint32_t i, j, n;
    uint32_t nlevels = alloc->LevelCount;
    ConsoleOutput("FREE LISTS:\n");
    for (i = 0; i < nlevels; ++i)
    {
        ConsoleOutput("L%02u: %u [", i, alloc->FreeCount[i]);
        for (j = 0, n = alloc->FreeCount[i]; j < n; ++j)
        {
            ConsoleOutput("%u", alloc->FreeLists[i][j]);
            if (j != (n-1))
                ConsoleOutput(", ");
        }
        ConsoleOutput("]\n");
    }
}

static void
DumpAllocatorState
(
    CORE_MEMORY_ALLOCATOR *alloc
)
{
    DumpMergeIndex(alloc);
    ConsoleOutput("-------------------------------------------------------------\n");
    DumpSplitIndex(alloc);
    ConsoleOutput("-------------------------------------------------------------\n");
    DumpFreeLists(alloc);
    ConsoleOutput("=============================================================\n\n");
}
#endif

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
    CORE_MEMORY_BLOCK            b;
    uint64_t                     block_size;
    uint64_t                     block_offset;
    uint32_t                     global_level_index;
    uint32_t                     global_level_count;
    uint32_t                     level_block_count;
    uint32_t                     level_block_index;
    uint32_t                     prior_block_count;
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

    /* figure out how much memory we need to store the allocator state for our general-purpose allocator config.
     * the minimum and maximum allocation size must be powers of two, but we only have 14MB to work with.
     * to get around this, we specify a maximum size of 16MB (a power of two) and then use BytesReserved.
     */
    if ((state_size = CORE_QueryMemoryAllocatorStateSize(Kilobytes(16), /*Megabytes(16)*/Kilobytes(64))) == 0)
    {
        ConsoleError("ERROR: Failed to determine memory allocator state data requirement.\n");
        return 1;
    }
    if ((state_data = CORE_MemoryArenaAllocateHost(&host_metadata_arena, state_size, CORE_AlignOf(CORE_MEMORY_ALLOCATOR), NULL)) == NULL)
    {
        ConsoleError("ERROR: Failed to allocate %Iu bytes for general-purpose allocator state.\n", state_size);
        return 1;
    }


    /* initialize a general-purpose memory allocator */
    host_alloc_init.AllocatorName     = "Main Data";                           /* Assign a string literal for debugging */
    host_alloc_init.AllocatorType     = CORE_MEMORY_ALLOCATOR_TYPE_HOST;       /* The allocator manages host-visible memory */
    host_alloc_init.AllocationSizeMin = Kilobytes(16);                         /* Minimum allocation size. Must be a power-of-two. */
    //host_alloc_init.AllocationSizeMax = Megabytes(16);                         /* Maximum allocation size. Must be a power-of-two. */
    //host_alloc_init.BytesReserved     = Megabytes(2);                          /* Reserve 2MB, because we only have 14MB to work with. */
    host_alloc_init.AllocationSizeMax = Kilobytes(64);
    host_alloc_init.BytesReserved     = 0;
    host_alloc_init.MemoryStart       =(uint64_t) host_mem->BaseAddress;       /* The host-visible address of representing the start of the memory block. */
    //host_alloc_init.MemorySize        =(uint64_t) host_mem->BytesCommitted;    /* The size of the memory block, in bytes. */
    host_alloc_init.MemorySize        =(uint64_t) Kilobytes(64);
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

    /* re-initialize the memory allocator state since we mucked with it above */
    CORE_InitMemoryAllocator(&host_alloc, &host_alloc_init);

    /* allocate as many 16KB blocks as will fit in the 14MB we have available */
    ConsoleOutput("ALLOCATIONS\n");
    //max_blocks = Megabytes(14) / Kilobytes(16);
    max_blocks = Kilobytes(64) / Kilobytes(16);
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        res = CORE_MemoryAllocate(&host_alloc, Kilobytes(16), CORE_AlignOf(uint32_t), &b);
        assert(res == 0 && "Memory allocation failed when it should have succeeded");
        assert((uint64_t) b.HostAddress >= host_alloc_init.MemoryStart);
        assert(b.BlockOffset >= 0);
        assert(b.BlockOffset < (host_alloc_init.MemoryStart + host_alloc_init.MemorySize));
        //DumpBlock(&b);
        DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
        DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    }
    /* the next allocation attempt had better fail */
    res = CORE_MemoryAllocate(&host_alloc, Kilobytes(16), CORE_AlignOf(uint32_t), &b);
    assert(res == -1 && "Memory allocation succeeded when it should have failed");
    ConsoleOutput("FREES\n");
    /* free all blocks */
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        b.SizeInBytes   =  Kilobytes(16);
        b.BlockOffset   = (block_index) * Kilobytes(16);
        b.HostAddress   = (void*)(host_alloc_init.MemoryStart + b.BlockOffset);
        b.AllocatorType =  CORE_MEMORY_ALLOCATOR_TYPE_HOST;
        CORE_MemoryFree(&host_alloc, &b);
        //DumpBlock(&b);
        DumpSplitIndex (host_alloc.LevelCount, host_alloc.SplitIndex , host_alloc.SplitIndexSize);
        DumpStatusIndex(host_alloc.LevelCount, host_alloc.StatusIndex, host_alloc.StatusIndexSize);
    }
    /* a 16MB allocation should fail, because we only really have 14MB */
    //res = CORE_MemoryAllocate(&host_alloc, Megabytes(16), CORE_AlignOf(uint32_t), &b);
    //assert(res == -1 && "Memory allocation for 16MB succeeded when it should have failed");
    /* a 14MB allocation should succeed, because we actually have that memory */
    //res = CORE_MemoryAllocate(&host_alloc, Megabytes(14), CORE_AlignOf(uint32_t), &b);
    //assert(res ==  0 && "Memory allocation failed when it should have succeeded");
    //assert((uint64_t) b.HostAddress == host_alloc_init.MemoryStart);
    //assert(b.BlockOffset == 0);

    /* ... */

    /* cleanup. generally you can just call CORE_DeleteHostMemoryPool, but this is test code. */
    CORE_HostMemoryRelease(host_mem);
    CORE_HostMemoryRelease(host_metadata_mem);
    CORE_DeleteHostMemoryPool(&host_pool);
    return 0;
}

