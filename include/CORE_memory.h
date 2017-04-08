/*
 * CORE_memory.h: A single-file library for performing host memory management.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit,
 *
 */
#ifndef __CORE_MEMORY_H__
#define __CORE_MEMORY_H__

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                    static _rt
#else
#define CORE_API(_rt)                    extern _rt
#endif

/* Define the appropriate restrict keyword for your compiler. */
#ifndef CORE_RESTRICT
#define CORE_RESTRICT                    __restrict
#endif

/* @summary Retrieve the alignment of a particular type, in bytes.
 * @param _t A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignOf
#define CORE_AlignOf(_t)                 __alignof(_t)
#endif

/* @summary Align a non-zero size up to the nearest even multiple of a given power-of-two.
 * @param _q is the size value to align up.
 * @param _a is the desired power-of-two alignment.
 */
#ifndef CORE_AlignUp
#define CORE_AlignUp(_q, _a)             (((_q) + ((_a)-1)) & ~((_a)-1))
#endif

/* @summary For a given address, return the address aligned for a particular type.
 * @param _p The unaligned address.
 * @param _t A typename, such as int, specifying the type whose alignment is to be retrieved.
 */
#ifndef CORE_AlignFor
#define CORE_AlignFor(_p, _t)            ((void*)((((uintptr_t)(_p)) + ((_a)-1)) & ~((_a)-1)))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an instance of that type, accounting for the padding required for proper alignment.
 * @param _t A typename, such as int, specifying the type whose allocation size is being queried.
 */
#ifndef CORE_AllocationSizeType
#define CORE_AllocationSizeType(_t)      ((sizeof(_t))        + (__alignof(_t)-1))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an array of instances of that type, accounting for the padding required for proper alignment.
 * @param _t A typename, such as int, specifying the type whose allocation size is being queried.
 * @param _n The number of elements in the array.
 */
#ifndef CORE_AllocationSizeArray
#define CORE_AllocationSizeArray(_t, _n) ((sizeof(_t) * (_n)) + (__alignof(_t)-1))
#endif

/* Forward-declare types exported by the library */
struct _CORE_HOST_MEMORY_POOL;
struct _CORE_HOST_MEMORY_POOL_INIT;
struct _CORE_HOST_MEMORY_ALLOCATION;

/* Define the data representing a pool of host memory allocations. Each pool can be accessed from a single thread only. */
typedef struct _CORE_HOST_MEMORY_POOL {
    char const                          *PoolName;           /* A nul-terminated string specifying the name of the pool. This value is used for debugging only. */
    struct _CORE_HOST_MEMORY_ALLOCATION *FreeList;           /* A pointer to the first free _CORE_HOST_MEMORY_ALLOCATION node, or NULL of the free list is empty. */
    uint32_t                             Capacity;           /* The maximum number of allocations that can be made from the pool. */
    uint32_t                             OsPageSize;         /* The size of the OS virtual memory manager page, in bytes. */
    uint32_t                             MinAllocationSize;  /* The minimum number of bytes that can be associated with any individual allocation. */
    uint32_t                             MinCommitIncrease;  /* The minimum number of bytes that a memory commitment can increase by. */
    uint64_t                             MaxTotalCommitment; /* The maximum number of bytes of process address space that can be committed across all allocations in the pool. */
    uint64_t                             PoolTotalCommitment;/* The number of bytes currently committed across all active allocations in the pool */
    uint32_t                             OsGranularity;      /* The allocation granularity (alignment) of allocations made through the OS virtual memory manager, in bytes. */
    struct _CORE_HOST_MEMORY_ALLOCATION *NodeList;           /* An array of Capacity _CORE_HOST_MEMORY_ALLOCATION nodes representing the preallocated node pool. */
} CORE_HOST_MEMORY_POOL;

/* Define the data used to configure a host memory pool */
typedef struct _CORE_HOST_MEMORY_POOL_INIT {
    char const                          *PoolName;           /* A nul-terminated string specifying the name of the pool.  */
    uint32_t                             PoolCapacity;       /* The maximum number of allocations that can be made from the pool. */
    uint32_t                             MinAllocationSize;  /* The minimum number of bytes that can be associated with any individual allocation. */
    uint32_t                             MinCommitIncrease;  /* The minimum number of bytes that a memory commitment can increase by. */
    uint64_t                             MaxTotalCommitment; /* The maximum number of bytes of process address space that can be committed across all allocations in the pool. */
} CORE_HOST_MEMORY_POOL_INIT;

/* Define the data associated with a single host memory allocation. The allocation represents a single allocation from the host virtual memory system. */
typedef struct _CORE_HOST_MEMORY_ALLOCATION {
    struct _CORE_HOST_MEMORY_POOL       *SourcePool;         /* The host memory pool from which the allocation was made. */
    struct _CORE_HOST_MEMORY_ALLOCATION *NextAllocation;     /* A field used by the host memory pool for free list management. The application may use this field for its own purposes for the lifetime of the allocation. */
    uint8_t                             *BaseAddress;        /* The address of the first accessible (committed) byte of the allocation. */
    uint64_t                             BytesReserved;      /* The number of bytes of process address space reserved for the allocation. */
    uint64_t                             BytesCommitted;     /* The number of bytes of process address space committed to the allocation. */
    uint32_t                             AllocationFlags;    /* One or more values from the _CORE_HOST_MEMORY_ALLOCATION_FLAGS enumeration. */
} CORE_HOST_MEMORY_ALLOCATION;

/* Define various flags that can be bitwise OR'd to control the allocation attributes for a single host memory allocation. */
typedef enum _CORE_HOST_MEMORY_ALLOCATION_FLAGS {
    CORE_HOST_MEMORY_ALLOCATION_FLAGS_DEFAULT   = (0 << 0),  /* The memory can be read and written by the host, and ends with a guard page. */
    CORE_HOST_MEMORY_ALLOCATION_FLAG_READ       = (1 << 0),  /* The memory can be read by the host. */
    CORE_HOST_MEMORY_ALLOCATION_FLAG_WRITE      = (1 << 1),  /* The memory can be written by the host. */
    CORE_HOST_MEMORY_ALLOCATION_FLAG_EXECUTE    = (1 << 2),  /* The allocation can contain code that can be executed by the host. */
    CORE_HOST_MEMORY_ALLOCATION_FLAG_NOGUARD    = (1 << 3),  /* The allocation will not end with a guard page. */
    CORE_HOST_MEMORY_ALLOCATION_FLAGS_READWRITE =            /* The committed memory can be read and written by the host. */
        CORE_HOST_MEMORY_ALLOCATION_FLAG_READ   | 
        CORE_HOST_MEMORY_ALLOCATION_FLAG_WRITE
} CORE_HOST_MEMORY_ALLOCATION_FLAGS;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Zero a block of memory.
 * @param dst The starting address of the block to zero-fill.
 * @param len The number of bytes to zero-fill.
 */
CORE_API(void)
CORE_ZeroMemory
(
    void  *dst, 
    size_t len
);

/* @summary Zero a block of memory, ensuring the operation is not optimized away.
 * @param dst The starting address of the block to zero-fill.
 * @param len The number of bytes to zero-fill.
 */
CORE_API(void)
CORE_ZeroMemorySecure
(
    void  *dst, 
    size_t len
);

/* @summary Copy a non-overlapping region of memory.
 * @param dst The address of the first byte to write.
 * @param src The address of the first byte to read.
 * @param len The number of bytes to copy from the source to the destination.
 */
CORE_API(void)
CORE_CopyMemory
(
    void       * CORE_RESTRICT dst, 
    void const * CORE_RESTRICT src, 
    size_t                     len
);

/* @summary Copy a possibly-overlapping region of memory.
 * @param dst The address of the first byte to write.
 * @param src The address of the first byte to read.
 * @param len The number of bytes to copy from the source to the destination.
 */
CORE_API(void)
CORE_MoveMemory
(
    void       *dst,
    void const *src, 
    size_t      len
);

/* @summary Set bytes in a memory block to given value.
 * @param dst The address of the first byte to write.
 * @param len The number of bytes to write.
 * @param val The value to write to each byte in the memory region.
 */
CORE_API(void)
CORE_FillMemory
(
    void   *dst, 
    size_t  len, 
    uint8_t val
);

/* @summary Initialize a pre-allocated pool of host memory allocation nodes.
 * @param pool The CORE_HOST_MEMORY_POOL to initialize.
 * @param init The attributes of the pool.
 * @return Zero if the pool is initialized successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateHostMemoryPool
(
    CORE_HOST_MEMORY_POOL      *pool, 
    CORE_HOST_MEMORY_POOL_INIT *init
);

/* @summary Free all memory allocated to a host memory allocation pool.
 * @param pool The CORE_HOST_MEMORY_POOL to delete. All existing allocations are invalidated.
 */
CORE_API(void)
CORE_DeleteHostMemoryPool
(
    CORE_HOST_MEMORY_POOL *pool
);

/* @summary Reserve, and optionally commit, address space within a process.
 * @param pool The CORE_HOST_MEMORY_POOL from which the memory allocation will be acquired.
 * @param reserve_size The number of bytes of process address space to reserve. This value is rounded up to the nearest multiple of the system virtual memory page size.
 * @param commit_size The number of bytes of preocess address space to commit. This value is rounded up to the nearest multiple of the system virtual memory page size.
 * @param alloc_flags One or more of the values of CORE_HOST_MEMORY_ALLOCATION_FLAGS, or 0 if no special behavior is desired, in which case the memory is allocated as readable, writable, and ends with a guard page.
 * @return The CORE_HOST_MEMORY_ALLOCATION representing the allocated address space. The returned address is aligned to a multiple of the system virtual memory allocation granularity. The call returns NULL if the allocation fails.
 */
CORE_API(CORE_HOST_MEMORY_ALLOCATION*)
CORE_HostMemoryPoolAllocate
(
    CORE_HOST_MEMORY_POOL *pool, 
    size_t         reserve_size, 
    size_t          commit_size, 
    uint32_t        alloc_flags
);

/* @summary Release all address space reserved for a single allocation and return it to the memory pool.
 * @param pool The pool to which the allocation will be returned. This must be the same pool from which the allocation was acquired.
 * @param alloc The host memory allocation to return.
 */
CORE_API(void)
CORE_HostMemoryPoolRelease
(
    CORE_HOST_MEMORY_POOL        *pool, 
    CORE_HOST_MEMORY_ALLOCATION *alloc
);

/* @summary Invalidate all existing allocations from a host memory pool and return them to the pool, without deleting the pool itself.
 * @param pool The CORE_HOST_MEMORY_POOL to reset to empty.
 */
CORE_API(void)
CORE_HostMemoryPoolReset
(
    CORE_HOST_MEMORY_POOL *pool
);

/* @summary Reserve and optionally commit address space within a process. Call CORE_HostMemoryRelease if the allocation already holds address space.
 * @param alloc The CORE_HOST_MEMORY_ALLOCATION to initialize.
 * @param reserve_size The amount of process address space to reserve, in bytes. This value is rounded up to the next multiple of the operating system page size.
 * @param commit_size The amount of process address space to commit, in bytes. This value is rounded up to the next multiple of the operating system page size, unless it is zero.
 * @param alloc_flags One or more CORE_HOST_MEMORY_ALLOCATION_FLAGS specifying the access and behavior of the memory region.
 * @return Zero if the allocation is initialized successfully, or -1 if an error occurred.
 */
CORE_API(int)
CORE_HostMemoryReserveAndCommit
(
    CORE_HOST_MEMORY_ALLOCATION *alloc, 
    size_t                reserve_size, 
    size_t                 commit_size, 
    uint32_t               alloc_flags
);

/* @summary Increase the amount of committed address space within an existing allocation. The commit size cannot exceed the reservation size.
 * @param alloc The CORE_HOST_MEMORY_ALLOCATION to modify.
 * @param commit_size The total amount of process address space within the allocation that should be committed.
 * @return Zero if at least the specified amount of address space is committed within the allocation, or -1 if an error occurred.
 */
CORE_API(int)
CORE_HostMemoryIncreaseCommitment
(
    CORE_HOST_MEMORY_ALLOCATION *alloc, 
    size_t                 commit_size
);

/* @summary Flush the CPU instruction cache after writing dynamically-generated code to a memory block.
 * @param alloc The CORE_HOST_MEMORY_ALLOCATION representing the memory block containing the dynamically-generated code.
 */
CORE_API(void)
CORE_HostMemoryFlush
(
    CORE_HOST_MEMORY_ALLOCATION *alloc
);

/* @summary Decommit and release the process address space associated with a host memory allocation.
 * @param alloc The CORE_HOST_MEMORY_ALLOCATION representing the process address space to release.
 */
CORE_API(void)
CORE_HostMemoryRelease
(
    CORE_HOST_MEMORY_ALLOCATION *alloc
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_MEMORY_H__ */

#ifdef CORE_MEMORY_IMPLEMENTATION

/* Internal functions are declared static ReturnType CORE__ (double-underscore) */

CORE_API(void)
CORE_ZeroMemory
(
    void  *dst, 
    size_t len
)
{
    ZeroMemory(dst, len);
}

CORE_API(void)
CORE_ZeroMemorySecure
(
    void  *dst, 
    size_t len
)
{
    (void) SecureZeroMemory(dst, len);
}

CORE_API(void)
CORE_CopyMemory
(
    void       * CORE_RESTRICT dst, 
    void const * CORE_RESTRICT src, 
    size_t                     len
)
{
    CopyMemory(dst, src, len);
}

CORE_API(void)
CORE_MoveMemory
(
    void       *dst,
    void const *src, 
    size_t      len
)
{
    MoveMemory(dst, src, len);
}

CORE_API(void)
CORE_FillMemory
(
    void   *dst, 
    size_t  len, 
    uint8_t val
)
{
    FillMemory(dst, len, val);
}

CORE_API(int)
CORE_CreateHostMemoryPool
(
    CORE_HOST_MEMORY_POOL      *pool, 
    CORE_HOST_MEMORY_POOL_INIT *init
)
{
    SYSTEM_INFO    sysinfo;
    size_t      total_size = 0;
    size_t actual_capacity = 0;
    size_t               i = 0;
    void            *array = NULL;

    /* retrieve the OS page size and allocation granularity */
    GetNativeSystemInfo(&sysinfo);

    /* it doesn't make much sense to limit the pool size and waste memory so figure out how
       many nodes we can fit when the size is rounded up to the allocation granularity. */
    total_size      = CORE_AlignUp(init->PoolCapacity * sizeof(CORE_HOST_MEMORY_ALLOCATION), sysinfo.dwPageSize);
    actual_capacity = total_size / sizeof(CORE_HOST_MEMORY_ALLOCATION);

    /* allocate the memory for the node pool as one large contiguous block */
    if ((array = VirtualAlloc(NULL, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) == NULL)
    {
        ZeroMemory(pool, sizeof(CORE_HOST_MEMORY_POOL));
        return -1;
    }

    /* apply limits to the configuration arguments */
    if (init->MinAllocationSize == 0)
        init->MinAllocationSize  =(uint32_t) sysinfo.dwPageSize;
    if (init->MinCommitIncrease == 0)
        init->MinCommitIncrease  =(uint32_t) sysinfo.dwPageSize;

    /* initialize the fields of the pool structure */
    pool->PoolName               = init->PoolName;
    pool->FreeList               = NULL;
    pool->Capacity               =(uint32_t) actual_capacity;
    pool->OsPageSize             =(uint32_t) sysinfo.dwPageSize;
    pool->MinAllocationSize      = init->MinAllocationSize;
    pool->MinCommitIncrease      = init->MinCommitIncrease;
    pool->MaxTotalCommitment     = init->MaxTotalCommitment;
    pool->PoolTotalCommitment    = 0;
    pool->OsGranularity          =(uint32_t) sysinfo.dwAllocationGranularity;
    pool->NodeList               =(CORE_HOST_MEMORY_ALLOCATION *) array;

    /* all pool nodes start out as free */
    for (i = 0; i < actual_capacity; ++i)
    {
        CORE_HOST_MEMORY_ALLOCATION *node = &pool->NodeList[i];
        node->SourcePool     = pool;
        node->NextAllocation = pool->FreeList;
        node->BaseAddress    = NULL;
        node->BytesReserved  = 0;
        node->BytesCommitted = 0;
        node->AllocationFlags= 0;
        pool->FreeList = node;
    }
    return 0;
}

CORE_API(void)
CORE_DeleteHostMemoryPool
(
    CORE_HOST_MEMORY_POOL *pool
)
{
    size_t i, n;

    /* free all of the individual allocations */
    for (i = 0, n = pool->Capacity; i < n; ++i)
    {
        CORE_HostMemoryRelease(&pool->NodeList[i]);
    }
    /* free the memory allocated for the pool */
    if (pool->NodeList != NULL)
    {
        VirtualFree(pool->NodeList, 0, MEM_RELEASE);
    }
    /* zero specific fields of the pool, but leave others (for post-mortem debugging) */
    pool->FreeList = NULL;
    pool->Capacity = 0;
    pool->NodeList = NULL;
}

CORE_API(CORE_HOST_MEMORY_ALLOCATION*)
CORE_HostMemoryPoolAllocate
(
    CORE_HOST_MEMORY_POOL *pool, 
    size_t         reserve_size, 
    size_t          commit_size, 
    uint32_t        alloc_flags
)
{
    if (pool->FreeList != NULL)
    {   /* attempt to initialize the object at the head of the free list */
        CORE_HOST_MEMORY_ALLOCATION *alloc = pool->FreeList;
        if (CORE_HostMemoryReserveAndCommit(alloc, reserve_size, commit_size, alloc_flags) < 0)
        {
            return NULL;
        }
        /* pop the object from the front of the free list */
        pool->FreeList = alloc->NextAllocation;
        alloc->NextAllocation = NULL;
        return alloc;
    }
    else
    {
        SetLastError(ERROR_OUT_OF_STRUCTURES);
        return NULL;
    }
}

CORE_API(void)
CORE_HostMemoryPoolRelease
(
    CORE_HOST_MEMORY_POOL        *pool, 
    CORE_HOST_MEMORY_ALLOCATION *alloc
)
{
    if (alloc == NULL)
        return;
    if (alloc->BaseAddress != NULL)
    {
        assert(alloc->SourcePool == pool);
        CORE_HostMemoryRelease(alloc);
        alloc->NextAllocation = pool->FreeList;
        pool->FreeList = alloc;
    }
}

CORE_API(void)
CORE_HostMemoryPoolReset
(
    CORE_HOST_MEMORY_POOL *pool
)
{   
    size_t i, n;

    /* clear the free list */
    pool->FreeList = NULL;
    /* return all nodes to the free list */
    for (i = 0, n = pool->Capacity; i < n; ++i)
    {
        CORE_HOST_MEMORY_ALLOCATION *node = &pool->NodeList[i];
        CORE_HostMemoryRelease(node);
        node->SourcePool     = pool;
        node->NextAllocation = pool->FreeList;
        node->BaseAddress    = NULL;
        node->BytesReserved  = 0;
        node->BytesCommitted = 0;
        node->AllocationFlags= 0;
        pool->FreeList = node;
    }
}

CORE_API(int)
CORE_HostMemoryReserveAndCommit
(
    CORE_HOST_MEMORY_ALLOCATION *alloc, 
    size_t                reserve_size, 
    size_t                 commit_size, 
    uint32_t               alloc_flags
)
{
    SYSTEM_INFO   sysinfo;
    void            *base = NULL;
    size_t      page_size = 0;
    size_t          extra = 0;
    size_t    min_reserve = 0;
    DWORD          access = 0;
    DWORD           flags = MEM_RESERVE;

    if (alloc->SourcePool != NULL)
    {   /* use the values cached on the source pool */
        min_reserve = alloc->SourcePool->MinAllocationSize;
        page_size   = alloc->SourcePool->OsPageSize;
    }
    else
    {   /* query the OS for the page size and allocation granularity */
        GetNativeSystemInfo(&sysinfo);
        min_reserve = sysinfo.dwPageSize;
        page_size   = sysinfo.dwPageSize;
    }
    if (reserve_size < min_reserve)
    {   /* limit to the minimum allocation size */
        reserve_size = min_reserve;
    }
    if (commit_size > reserve_size)
    {   assert(commit_size <= reserve_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* VMM allocations are rounded up to the next even multiple of the system 
     * page size, and have a starting address that is an even multiple of the
     * system allocation granularity (typically 64KB). */
    reserve_size = CORE_AlignUp(reserve_size, page_size);

    /* map CORE_HOST_MEMORY_ALLOCATION_FLAGS to Win32 access and protection flags */
    if (alloc_flags & CORE_HOST_MEMORY_ALLOCATION_FLAG_READ)
        access = PAGE_READONLY;
    if (alloc_flags & CORE_HOST_MEMORY_ALLOCATION_FLAG_WRITE)
        access = PAGE_READWRITE;
    if (alloc_flags & CORE_HOST_MEMORY_ALLOCATION_FLAG_EXECUTE)
    {   /* this also commits the entire reservation */
        access = PAGE_EXECUTE_READWRITE;
        commit_size = reserve_size;
    }

    /* determine whether additional space will be added for a trailing guard page */
    extra = (alloc_flags & CORE_HOST_MEMORY_ALLOCATION_FLAG_NOGUARD) ? 0 : page_size;

    /* is address space being committed in addition to being reserved? */
    if (commit_size > 0)
    {
        commit_size = CORE_AlignUp(commit_size, page_size);
        flags |= MEM_COMMIT;
    }
    if (alloc->SourcePool != NULL && alloc->SourcePool->MaxTotalCommitment != 0 && commit_size > 0)
    {   /* ensure that this request won't cause the pool limit to be exceeded */
        size_t new_commit = alloc->SourcePool->PoolTotalCommitment + commit_size;
        if    (new_commit > alloc->SourcePool->MaxTotalCommitment)
        {   SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return -1;
        }
    }

    /* reserve and possibly commit contiguous virtual address space */
    if ((base = VirtualAlloc(NULL, reserve_size + extra, flags, access)) == NULL)
    {
        return -1;
    }
    if (extra > 0)
    {   /* change the protection flags for the guard page only */
        if (VirtualAlloc((uint8_t*) base + reserve_size, page_size, MEM_COMMIT, access | PAGE_GUARD) == NULL)
        {   DWORD error = GetLastError();
            VirtualFree(base, 0, MEM_RELEASE);
            SetLastError(error);
            return -1;
        }
    }

    /* if address space was committed, adjust the total commit on the source pool */
    if (alloc->SourcePool != NULL && commit_size > 0)
    {
        alloc->SourcePool->PoolTotalCommitment += commit_size;
    }

    /* the allocation process completed successfully */
    alloc->BaseAddress     =(uint8_t*) base;
    alloc->BytesReserved   = reserve_size;
    alloc->BytesCommitted  = commit_size;
    alloc->AllocationFlags = alloc_flags;
    return 0;
}

CORE_API(int)
CORE_HostMemoryIncreaseCommitment
(
    CORE_HOST_MEMORY_ALLOCATION *alloc, 
    size_t                 commit_size
)
{
    if (alloc->BytesReserved == 0)
    {   /* need to use CORE_HostMemoryReserveAndCommit instead */
        SetLastError(ERROR_INVALID_FUNCTION);
        return -1;
    }
    if (alloc->BytesCommitted < commit_size)
    {
        SYSTEM_INFO        sysinfo;
        size_t max_commit_increase = alloc->BytesReserved - alloc->BytesCommitted;
        size_t req_commit_increase = commit_size - alloc->BytesCommitted;
        size_t old_bytes_committed = alloc->BytesCommitted;
        size_t min_commit_increase = 0;
        size_t new_bytes_committed = 0;
        size_t           page_size = 0;
        DWORD               access = 0;

        if (alloc->SourcePool != NULL)
        {   /* use the minimum commit increase specified on the source pool */
            min_commit_increase = alloc->SourcePool->MinCommitIncrease;
            page_size = alloc->SourcePool->OsPageSize;
        }
        else
        {   /* query the OS for the system page size */
            GetNativeSystemInfo(&sysinfo);
            page_size = sysinfo.dwPageSize;
        }
        if (req_commit_increase < min_commit_increase)
        {   /* increase the commitment to the minimum value */
            req_commit_increase = min_commit_increase;
        }
        if (req_commit_increase > max_commit_increase)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return -1;
        }

        /* figure out how much we're asking for, taking into account that VMM allocations round up to a page size multiple */
        new_bytes_committed = CORE_AlignUp(old_bytes_committed + req_commit_increase, page_size);
        req_commit_increase = new_bytes_committed - old_bytes_committed;
        if (alloc->SourcePool != NULL && alloc->SourcePool->MaxTotalCommitment != 0)
        {   /* ensure that the pool limit isn't exceeded */
            if (alloc->SourcePool->PoolTotalCommitment + req_commit_increase > alloc->SourcePool->MaxTotalCommitment)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return -1;
            }
        }

        /* convert the allocation flags into Win32 protection flags */
        if (alloc->AllocationFlags & CORE_HOST_MEMORY_ALLOCATION_FLAG_READ)
            access = PAGE_READONLY;
        if (alloc->AllocationFlags & CORE_HOST_MEMORY_ALLOCATION_FLAG_WRITE)
            access = PAGE_READWRITE;
        if (alloc->AllocationFlags & CORE_HOST_MEMORY_ALLOCATION_FLAG_EXECUTE)
            access = PAGE_EXECUTE_READWRITE;

        /* request that an additional portion of the pre-reserved address space be committed.
         * executable allocations are entirely committed up-front, so don't worry about that. */
        if (VirtualAlloc((uint8_t*) alloc->BaseAddress, new_bytes_committed, MEM_COMMIT, access) == NULL)
        {
            return -1;
        }
        if (alloc->SourcePool != NULL)
        {
            alloc->SourcePool->PoolTotalCommitment += req_commit_increase;
        }
        alloc->BytesCommitted = new_bytes_committed;
        return 0;
    }
    else
    {   /* the requested commit amount has already been met */
        return 0;
    }
}

CORE_API(void)
CORE_HostMemoryFlush
(
    CORE_HOST_MEMORY_ALLOCATION *alloc
)
{
    if (alloc->AllocationFlags & CORE_HOST_MEMORY_ALLOCATION_FLAG_EXECUTE)
    {
        (void) FlushInstructionCache(GetCurrentProcess(), alloc->BaseAddress, alloc->BytesCommitted);
    }
}

CORE_API(void)
CORE_HostMemoryRelease
(
    CORE_HOST_MEMORY_ALLOCATION *alloc
)
{
    if (alloc->BaseAddress != NULL)
    {   /* free the entire range of address space */
        VirtualFree(alloc->BaseAddress, 0, MEM_RELEASE);
        /* update the total commitment on the source pool */
        if (alloc->SourcePool != NULL)
        {   assert(alloc->BytesCommitted < alloc->SourcePool->PoolTotalCommitment);
            alloc->SourcePool->PoolTotalCommitment -= alloc->BytesCommitted;
        }
    }
    alloc->BaseAddress    = NULL;
    alloc->BytesReserved  = 0;
    alloc->BytesCommitted = 0;
}

#endif /* CORE_MEMORY_IMPLEMENTATION */

