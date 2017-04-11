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
    if ((state_size = CORE_QueryMemoryAllocatorStateSize(Kilobytes(16), Megabytes(16))) == 0)
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
    host_alloc_init.AllocationSizeMax = Megabytes(16);                         /* Maximum allocation size. Must be a power-of-two. */
    host_alloc_init.BytesReserved     = Megabytes(2);                          /* Reserve 2MB, because we only have 14MB to work with. */
    host_alloc_init.MemoryStart       =(uint64_t) host_mem->BaseAddress;       /* The host-visible address of representing the start of the memory block. */
    host_alloc_init.MemorySize        =(uint64_t) host_mem->BytesCommitted;    /* The size of the memory block, in bytes. */
    host_alloc_init.StateData         = state_data;                            /* Space for the allocator state data. */
    host_alloc_init.StateDataSize     = state_size;                            /* The number of bytes allocated for allocator state data. */
    host_alloc_init.UserData          = &host_mem;                             /* Specify any data you want here, up to CORE_MEMORY_ALLOCATOR_MAX_USER bytes. */
    host_alloc_init.UserDataSize      = sizeof(CORE_HOST_MEMORY_ALLOCATION*);  /* The size of the extra data you want to associate with the allocator instance. */
    if (CORE_InitMemoryAllocator(&host_alloc, &host_alloc_init) < 0)
    {
        ConsoleError("ERROR: Failed to initialize host memory allocator (%08X).\n", GetLastError());
        return 1;
    }

    /* allocate as many 16KB blocks as will fit in the 14MB we have available */
    max_blocks = Megabytes(14) / Kilobytes(16);
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        res = CORE_MemoryAllocate(&host_alloc, Kilobytes(16), CORE_AlignOf(uint32_t), &b);
        assert(res == 0 && "Memory allocation failed when it should have succeeded");
        assert((uint64_t) b.HostAddress >= host_alloc_init.MemoryStart);
        assert(b.BlockOffset >= 0);
        assert(b.BlockOffset < (host_alloc_init.MemoryStart + host_alloc_init.MemorySize));
    }
    /* the next allocation attempt had better fail */
    res = CORE_MemoryAllocate(&host_alloc, Kilobytes(16), CORE_AlignOf(uint32_t), &b);
    assert(res == -1 && "Memory allocation succeeded when it should have failed");
    /* free all blocks */
    for (block_index = 0; block_index < max_blocks; ++block_index)
    {
        b.SizeInBytes   =  Kilobytes(16);
        b.BlockOffset   = (block_index) * Kilobytes(16);
        b.HostAddress   = (void*)(host_alloc_init.MemoryStart + b.BlockOffset);
        b.AllocatorType =  CORE_MEMORY_ALLOCATOR_TYPE_HOST;
        CORE_MemoryFree(&host_alloc, &b);
    }
    assert(host_alloc.FreeCount[0] == 1);
    /* a 16MB allocation should fail, because we only really have 14MB */
    res = CORE_MemoryAllocate(&host_alloc, Megabytes(16), CORE_AlignOf(uint32_t), &b);
    assert(res == -1 && "Memory allocation for 16MB succeeded when it should have failed");
    /* a 14MB allocation should succeed, because we actually have that memory */
    res = CORE_MemoryAllocate(&host_alloc, Megabytes(14), CORE_AlignOf(uint32_t), &b);
    assert(res ==  0 && "Memory allocation failed when it should have succeeded");
    assert((uint64_t) b.HostAddress == host_alloc_init.MemoryStart);
    assert(b.BlockOffset == 0);

    /* ... */

    /* cleanup. generally you can just call CORE_DeleteHostMemoryPool, but this is test code. */
    CORE_HostMemoryRelease(host_mem);
    CORE_HostMemoryRelease(host_metadata_mem);
    CORE_DeleteHostMemoryPool(&host_pool);
    return 0;
}

