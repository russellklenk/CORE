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
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* Define the appropriate restrict keyword for your compiler. */
#ifndef CORE_RESTRICT
#define CORE_RESTRICT                     __restrict
#endif

/* Define the maximum number of opaque "user data" bytes that can be stored with a memory arena or memory allocator. */
#ifndef CORE_MEMORY_ALLOCATOR_MAX_USER
#define CORE_MEMORY_ALLOCATOR_MAX_USER    64
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
    ((void*)((((uint8_t*)(_address)) + ((__alignof(_type))-1)) & ~((__alignof(_type))-1)))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an instance of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 */
#ifndef CORE_AllocationSizeType
#define CORE_AllocationSizeType(_type)                                         \
    ((sizeof(_type)) + (__alignof(_type)-1))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an array of instances of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 * @param _count The number of elements in the array.
 */
#ifndef CORE_AllocationSizeArray
#define CORE_AllocationSizeArray(_type, _count)                                \
    ((sizeof(_type) * (_count)) + (__alignof(_type)-1))
#endif

/* @summary Allocate host memory with the correct size and alignment for an instance of a given type from a memory arena.
 * @param _arena The CORE_MEMORY_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _blk A CORE_MEMORY_BLOCK to be populated with information about the allocation, or NULL.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE_MemoryArenaAllocateHostType
#define CORE_MemoryArenaAllocateHostType(_arena, _type, _blk)                  \
    ((_type*) CORE_MemoryArenaAllocateHost((_arena), sizeof(_type), __alignof(_type), (_blk)))
#endif

/* @summary Allocate memory with the correct size and alignment for an array of instance of a given type from a memory arena.
 * @param _arena The CORE_MEMORY_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the array.
 * @param _blk A CORE_MEMORY_BLOCK to be populated with information about the allocation, or NULL.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE_MemoryArenaAllocateHostArray
#define CORE_MemoryArenaAllocateHostArray(_arena, _type, _count, _blk)         \
    ((_type*) CORE_MemoryArenaAllocateHost((_arena), sizeof(_type) * (_count), __alignof(_type), (_blk)))
#endif

/* @summary Allocate host memory with the correct size and alignment for an instance of a given type from a general-purpose memory allocator.
 * @param _alloc The CORE_MEMORY_ALLOCATOR from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _blk A CORE_MEMORY_BLOCK to be populated with information about the allocation. Must not be NULL.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE_MemoryAllocateHostType
#define CORE_MemoryAllocateHostType(_alloc, _type, _blk)                       \
    ((_type*) CORE_MemoryAllocateHost((_alloc), sizeof(_type), __alignof(_type), (_blk)))
#endif

/* @summary Allocate memory with the correct size and alignment for an array of instance of a given type from a general-purpose memory allocator.
 * @param _alloc The CORE_MEMORY_ALLOCATOR from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the array.
 * @param _blk A CORE_MEMORY_BLOCK to be populated with information about the allocation. Must not be NULL.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE_MemoryAllocateHostArray
#define CORE_MemoryAllocateHostArray(_arena, _type, _count, _blk)              \
    ((_type*) CORE_MemoryAllocateHost((_alloc), sizeof(_type) * (_count), __alignof(_type), (_blk)))
#endif

/* @summary Grow or shrink an existing host-visible array.
 * @param _alloc The CORE_MEMORY_ALLOCATOR from which the existing allocation was obtained.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the resized array.
 * @param _oldblk The CORE_MEMORY_BLOCK representing the existing allocation.
 * @param _newblk The CORE_MEMORY_BLOCK specifying attributes of the resized allocation.
 * @return A pointer to the start of the resized memory block, or NULL.
 */
#ifndef CORE_MemoryReallocHostArray
#define CORE_MemoryReallocHostArray(_arena, _type, _count, _oldblk, _newblk)   \
    ((_type*) CORE_MemoryReallocateHost((_alloc), (_oldblk), sizeof(_type) * (_count), __alignof(_type), (_oldblk)))
#endif

/* Forward-declare types exported by the library */
struct _CORE_HOST_MEMORY_POOL;
struct _CORE_HOST_MEMORY_POOL_INIT;
struct _CORE_HOST_MEMORY_ALLOCATION;
struct _CORE_MEMORY_BLOCK;
struct _CORE_MEMORY_ARENA;
struct _CORE_MEMORY_ALLOCATOR;
struct _CORE_MEMORY_ARENA_INIT;
struct _CORE_MEMORY_ALLOCATOR_INIT;

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

/* Define the data returned from a memory allocation request. */
typedef struct _CORE_MEMORY_BLOCK {
    uint64_t                             SizeInBytes;        /* The size of the memory block, in bytes. */
    uint64_t                             BlockOffset;        /* The allocation offset. This field is set for host and device allocations. */
    void                                *HostAddress;        /* The host-visible memory address. This field is set for host allocations only. */
    uint32_t                             AllocatorType;      /* One of _CORE_MEMORY_ALLOCATOR_TYPE indicating whether the block is a host or device allocation. */
} CORE_MEMORY_BLOCK;

/* Define the data associated with a memory arena allocator.
 * An arena allocator can manage any amount of memory, but supports only allocation and freeing back to a marked point in time.
 */
typedef struct _CORE_MEMORY_ARENA {
    #define USER CORE_MEMORY_ALLOCATOR_MAX_USER
    char const                          *AllocatorName;      /* A nul-terminated string specifying the name of the allocator. Used for debugging purposes only. */
    uint32_t                             AllocatorType;      /* One of _CORE_MEMORY_ALLOCATOR_TYPE indicating whether this is a host or device memory allocator. */
    uint64_t                             MemoryStart;        /* The address or offset of the start of the memory block from which sub-allocations are returned. */
    uint64_t                             MemorySize;         /* The size of the memory block from which sub-allocations are returned, in bytes. */
    uint64_t                             NextOffset;         /* The byte offset, relative to the start of the associated memory range, of the next free byte. */
    uint64_t                             MaximumOffset;      /* The maximum value of NextOffset. NextOffset is always in [0, MaximumOffset]. */
    uint8_t                              UserData[USER];     /* Extra storage for data the user wants to associate with the allocator instance. */
    #undef  USER
} CORE_MEMORY_ARENA;

/* Define the data used to configure a memory arena allocator when it is initialized. */
typedef struct _CORE_MEMORY_ARENA_INIT {
    char const                          *AllocatorName;      /* A nul-terminated string specifying the name of the allocator. Used for debugging purposes only. */
    uint32_t                             AllocatorType;      /* One of _CORE_MEMORY_ALLOCATOR_TYPE indicating whether this is a host or device memory allocator. */
    uint64_t                             MemoryStart;        /* The address or offset of the start of the memory block from which sub-allocations are returned. */
    uint64_t                             MemorySize;         /* The size of the memory block from which sub-allocations are returned, in bytes. */
    void                                *UserData;           /* The user data to be copied into the allocator instance. */
    uint64_t                             UserDataSize;       /* The number of bytes of user data to copy into the allocator instance. */
} CORE_MEMORY_ARENA_INIT;

/* An arena marker represents the allocator state at a specific point in time. */
typedef uint64_t CORE_MEMORY_ARENA_MARKER;                   /* This value holds the NextOffset value of the arena allocator state. */

/* Define the data associated with a general-purpose memory allocator based on a buddy allocation scheme. 
 * The buddy allocator can manage a maximum of 4GB, which is divided into power-of-two sized chunks between a minimum and maximum size.
 * CORE_BUDDY_ALLOCATOR_MAX_LEVELS defines the maximum number of power-of-two steps between the minimum and maximum size.
 * If aligned allocations will be required, the minimum block size should be set to the required alignment.
 * The CORE_MEMORY_ALLOCATOR supports general-purpose allocation operations such as malloc, realloc and free.
 * See http://bitsquid.blogspot.com/2015/08/allocation-adventures-3-buddy-allocator.html
 */
#ifndef CORE_BUDDY_ALLOCATOR_MAX_LEVELS
#define CORE_BUDDY_ALLOCATOR_MAX_LEVELS  16
#endif
typedef struct _CORE_MEMORY_ALLOCATOR {
    #define NLVL CORE_BUDDY_ALLOCATOR_MAX_LEVELS
    #define USER CORE_MEMORY_ALLOCATOR_MAX_USER
    char const                          *AllocatorName;      /* A nul-terminated string specifying the name of the allocator. Used for debugging purposes only. */
    uint32_t                             AllocatorType;      /* One of _CORE_MEMORY_ALLOCATOR_TYPE indicating whether this is a host or device memory allocator. */
    uint64_t                             MemoryStart;        /* The address or offset of the start of the memory block from which sub-allocations are returned. */
    uint64_t                             MemorySize;         /* The size of the memory block from which sub-allocations are returned, in bytes. */
    uint64_t                             AllocationSizeMin;  /* The size of the smallest memory block that can be returned by the buddy allocator, in bytes. */
    uint64_t                             AllocationSizeMax;  /* The size of the largest memory block that can be returned by the buddy allocator, in bytes. */
    uint64_t                             BytesReserved;      /* The number of bytes marked as reserved. These bytes can never be allocated to the application. */
    uint8_t                             *MetadataBase;       /* The base address of the metadata storage allocation. */
    uint32_t                            *FreeListData;       /* Storage for the free list arrays, allocated as a single contiguous block. There are 1 << LevelCount uint32_t values. */
    uint32_t                            *MergeIndex;         /* An array of 1 << (LevelCount-1) bits with each bit storing the state of a buddy pair. */
    uint32_t                            *SplitIndex;         /* An array of 1 << (LevelCount-1) bits with each bit set if the block at bit index i has been split. */
    uint32_t                             Reserved;           /* Reserved for future use. Set to zero. */
    uint32_t                             LevelCount;         /* The total number of levels used by the allocator, with level 0 representing the largest level. */
    uint32_t                             LevelBits[NLVL];    /* The zero-based index of the set bit for each level. LevelCount entries are valid. */
    uint32_t                             FreeCount[NLVL];    /* The number of entries in the free list for each level. LevelCount entries are valid. */
    uint32_t                            *FreeLists[NLVL];    /* Each of LevelCount entries points to an array of 1 << LevelIndex values specifying free block offsets for that level. */
    void                                *StateData;          /* The caller-allocated memory to be used for storing allocator state data. */
    uint64_t                             StateDataSize;      /* The number of bytes of state data available for use by the allocator instance. */
    uint8_t                              UserData[USER];     /* Extra storage for data the user wants to associate with the allocator instance. */
    #undef  NLVL
    #undef  USER
} CORE_MEMORY_ALLOCATOR;

/* Define the data used to configure a general-purpose memory allocator when it is initialized. */
typedef struct _CORE_MEMORY_ALLOCATOR_INIT {
    char const                          *AllocatorName;      /* A nul-terminated string specifying the name of the allocator. Used for debugging purposes only. */
    uint32_t                             AllocatorType;      /* One of _CORE_MEMORY_ALLOCATOR_TYPE indicating whether this is a host or device memory allocator. */
    uint64_t                             AllocationSizeMin;  /* The size of the smallest memory block that can be returned from the allocator, in bytes. */
    uint64_t                             AllocationSizeMax;  /* The size of the largest memory block that can be returned from the allocator, in bytes. */
    uint64_t                             BytesReserved;      /* The number of bytes marked as reserved. These bytes can never be allocated to the application. */
    uint64_t                             MemoryStart;        /* The address or offset of the start of the memory block from which sub-allocations are returned. */
    uint64_t                             MemorySize;         /* The size of the memory block from which sub-allocations are returned, in bytes. */
    void                                *StateData;          /* The caller-allocated memory to be used for storing allocator state data. This value must be non-NULL. */
    uint64_t                             StateDataSize;      /* The number of bytes of state data available for use by the allocator instance. This value must be at least the size returned by CORE_QueryMemoryAllocatorStateSize. */
    void                                *UserData;           /* The user data to be copied into the allocator instance. */
    uint64_t                             UserDataSize;       /* The number of bytes of user data to copy into the allocator instance. */
} CORE_MEMORY_ALLOCATOR_INIT;

/* Define the allowed values for memory allocator type. An allocator can manage either host or device memory. */
typedef enum _CORE_MEMORY_ALLOCATOR_TYPE {
    CORE_MEMORY_ALLOCATOR_TYPE_INVALID          =  0,        /* This value is invalid and should not be used. */
    CORE_MEMORY_ALLOCATOR_TYPE_HOST             =  1,        /* The allocator is a host memory allocator. */
    CORE_MEMORY_ALLOCATOR_TYPE_DEVICE           =  2,        /* The allocator is a device memory allocator. */
} CORE_MEMORY_ALLOCATOR_TYPE;

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

/* @summary Determine whether a CORE_MEMORY_BLOCK specifies a valid allocation.
 * @param block The CORE_MEMORY_BLOCK to examine.
 * @return Non-zero if the memory block specifies a valid allocation, or zero if the memory block specifies an invalid allocation.
 */
CORE_API(int)
CORE_MemoryBlockIsValid
(
    CORE_MEMORY_BLOCK *block
);

/* @summary Determine whether old_block and new_block point to the same memory location.
 * @param old_block The CORE_MEMORY_BLOCK representing an existing allocation.
 * @param new_block The CORE_MEMORY_BLOCK representing the modified allocation.
 * @return Non-zero if the memory block specifies a valid allocation, or zero if the memory block specifies an invalid allocation.
 */
CORE_API(int)
CORE_MemoryBlockDidMove
(
    CORE_MEMORY_BLOCK *old_block, 
    CORE_MEMORY_BLOCK *new_block
);

/* @summary Initialize an arena memory allocator.
 * @param arena The CORE_MEMORY_ARENA allocator to initialize.
 * @param init The attributes used to initialize the arena allocator.
 * @return Zero if the arena was successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_InitMemoryArena
(
    CORE_MEMORY_ARENA     *arena, 
    CORE_MEMORY_ARENA_INIT *init
);

/* @summary Sub-allocate memory from an arena.
 * @param arena The CORE_MEMORY_ARENA from which the memory is being requested.
 * @param size The minimum number of bytes to allocate from the arena.
 * @param alignment The desired alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param block On return, information about the allocated memory block is copied to this location. Required.
 * @return Zero if the allocation request was successful, or -1 if the allocation request failed.
 */
CORE_API(int)
CORE_MemoryArenaAllocate
(
    CORE_MEMORY_ARENA *arena, 
    size_t              size, 
    size_t         alignment, 
    CORE_MEMORY_BLOCK *block
);

/* @summary Sub-allocate memory from an arena managing host memory.
 * @param arena The CORE_MEMORY_ARENA from which the memory is being requested.
 * @param size The minimum number of bytes to allocate from the arena.
 * @param alignment The desired alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param block On return, information about the allocated memory block is copied to this location. Optional.
 * @return A pointer to the host memory, or NULL.
 */
CORE_API(void*)
CORE_MemoryArenaAllocateHost
(
    CORE_MEMORY_ARENA *arena, 
    size_t              size, 
    size_t         alignment, 
    CORE_MEMORY_BLOCK *block
);

/* @summary Retrieve a marker that can be used to roll back all memory allocations made after the marker was obtained.
 * @param arena The CORE_MEMORY_ARENA to query.
 */
CORE_API(CORE_MEMORY_ARENA_MARKER)
CORE_MemoryArenaMark
(
    CORE_MEMORY_ARENA *arena
);

/* @summary Roll back all memory allocations made from an arena since a previously marked point.
 * @param arena The CORE_MEMORY_ARENA to roll back.
 * @param marker A marker obtained from a prior call to CORE_MemoryArenaMark against the same arena.
 */
CORE_API(void)
CORE_MemoryArenaResetToMarker
(
    CORE_MEMORY_ARENA        *arena,
    CORE_MEMORY_ARENA_MARKER marker
);

/* @summary Roll back all memory allocations made against an arena.
 * @param arena The CORE_MEMORY_ARENA to reset.
 */
CORE_API(void)
CORE_MemoryArenaReset
(
    CORE_MEMORY_ARENA *arena
);

/* @summary Determine the number of bytes that need to be allocated to store memory allocator state data.
 * @param allocation_size_min A power of two, greater than zero, specifying the smallest size allocation that can be returned by the allocator.
 * @param allocation_size_max A power of two, greater than zero, specifying the largest size allocation that can be returned by the allocator.
 * @return The number of bytes that must be allocated by the caller for storing allocator state data.
 */
CORE_API(size_t)
CORE_QueryMemoryAllocatorStateSize
(
    size_t allocation_size_min, 
    size_t allocation_size_max
);

/* @summary Initialize a general-purpose memory allocator.
 * @param alloc The CORE_MEMORY_ALLOCATOR to initialize.
 * @param init The attributes used to configure the memory allocator.
 * @return Zero if the allocator is successfully initialized, or -1 if an error occurred.
 */
CORE_API(int)
CORE_InitMemoryAllocator
(
    CORE_MEMORY_ALLOCATOR     *alloc, 
    CORE_MEMORY_ALLOCATOR_INIT *init
);

/* @summary Allocate memory from a general-purpose allocator.
 * @param alloc The CORE_MEMORY_ALLOCATOR from which the memory will be allocated.
 * @param size The minimum number of bytes to allocate.
 * @param alignment The desired alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param block On return, information about the allocated memory block is copied to this location. Required.
 * @return Zero if the allocation request was successful, or -1 if the allocation request failed.
 */
CORE_API(int)
CORE_MemoryAllocate
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    size_t                  size, 
    size_t             alignment, 
    CORE_MEMORY_BLOCK     *block
);

/* @summary Allocate host memory from a general-purpose allocator managing host memory.
 * @param alloc The CORE_MEMORY_ALLOCATOR from which the memory will be allocated.
 * @param size The minimum number of bytes to allocate.
 * @param alignment The desired alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param block On return, information about the allocated memory block is copied to this location. Required.
 * @return A pointer to the host memory, or NULL.
 */
CORE_API(void*)
CORE_MemoryAllocateHost
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    size_t                  size, 
    size_t             alignment, 
    CORE_MEMORY_BLOCK     *block
);

/* @summary Grow or shrink a memory block to meet a desired size.
 * This function differs from realloc in that the caller must compare the BlockOffset or HostAddress returned in new_block to the BlockOffset or HostAddress in existing.
 * If the new_block specifies a different memory location, the caller must copy data from the old location to the new location. 
 * It is still valid to access the memory at the existing location even though the block has been marked free.
 * @param alloc The CORE_MEMORY_ALLOCATOR that returned the existing block.
 * @param existing The CORE_MEMORY_BLOCK representing the existing allocation.
 * @param new_size The new required minimum allocation size, in bytes.
 * @param alignment The required alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param new_block On return, information about the allocated memory block is copied to this location. Required.
 * @return Zero if the allocation request was successful, or -1 if the allocation request failed.
 */
CORE_API(int)
CORE_MemoryReallocate
(
    CORE_MEMORY_ALLOCATOR *                   alloc, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT  existing, 
    size_t                                 new_size, 
    size_t                                alignment, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT new_block
);

/* @summary Grow or shrink a memory block to meet a desired size.
 * @param alloc The CORE_MEMORY_ALLOCATOR that returned the existing block.
 * @param existing The CORE_MEMORY_BLOCK representing the existing allocation.
 * @param new_size The new required minimum allocation size, in bytes.
 * @param alignment The required alignment of the returned address or offset, in bytes. This must be a non-zero power-of-two.
 * @param new_block On return, information about the allocated memory block is copied to this location. Required.
 * @return A pointer to the host memory, which may or may not be the same as existing->HostAddress, or NULL if the reallocation request failed.
 */
CORE_API(void*)
CORE_MemoryReallocateHost
(
    CORE_MEMORY_ALLOCATOR *                   alloc, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT  existing, 
    size_t                                 new_size, 
    size_t                                alignment, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT new_block
);

/* @summary Free a general-purpose memory allocation.
 * @param alloc The CORE_MEMORY_ALLOCATOR that returned the existing block.
 * @param existing The CORE_MEMORY_BLOCK representing the allocation to free.
 */
CORE_API(void)
CORE_MemoryFree
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    CORE_MEMORY_BLOCK  *existing
);

/* @summary Invalidate all existing allocations and reset a memory allocator to its initial state.
 * @param alloc The CORE_MEMORY_ALLOCATOR to reset.
 */
CORE_API(void)
CORE_MemoryAllocatorReset
(
    CORE_MEMORY_ALLOCATOR *alloc
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_MEMORY_H__ */

#ifdef CORE_MEMORY_IMPLEMENTATION

/* Define the information used to look up the status of a block in the buddy allocator merge index.
 * The merge index contains one bit per buddy-pair. 
 * The bit is clear if both blocks are free, or bother are allocated.
 * The bit is set if only one block is allocated.
 */
typedef struct _CORE__BUDDY_BLOCK_MERGE_INFO {
    uint32_t WordIndex;          /* The zero-based index of the uint32_t value in the CORE_MEMORY_ALLOCATOR::MergeIndex field. */
    uint32_t Mask;               /* The mask value used to test or manipulate the state of the bit. */
} CORE__BUDDY_BLOCK_MERGE_INFO;

/* Define the information used to look up the status of a block in the buddy allocator split index.
 * The split index contains one bit per-block for each level not including the leaf level.
 * The bit is set if the corresponding block has been split.
 */
typedef struct _CORE__BUDDY_BLOCK_SPLIT_INFO {
    uint32_t WordIndex;          /* The zero-based index of the uint32_t value in the CORE_MEMORY_ALLOCATOR::SplitIndex field. */
    uint32_t Mask;               /* The mask value used to test or manipulate the state of the bit. */
} CORE__BUDDY_BLOCK_SPLIT_INFO;

/* Define the data returned from a buddy allocator block query. */
typedef struct _CORE__BUDDY_BLOCK_INFO {
    uint32_t LevelIndex;         /* The zero-based index of the level at which the block was allocated, with level 0 being the largest level. */
    uint32_t BitIndex;           /* The zero-based index of the bit that is set for blocks in this level. */
    uint32_t BlockSize;          /* The size of the blocks in this level, in bytes. */
    uint32_t BlockCount;         /* The maximum number of blocks in this level. */
    uint32_t IndexOffset;        /* The offset used to transform an absolute index into a relative index. */
    uint32_t LeftAbsoluteIndex;  /* The absolute block index of the leftmost block of the buddy pair, either BlockAbsoluteIndex or BuddyAbsoluteIndex. */
    uint32_t BlockAbsoluteIndex; /* The absolute block index of the input block. */
    uint32_t BuddyAbsoluteIndex; /* The absolute block index of the buddy of the input block. */
} CORE__BUDDY_BLOCK_INFO;

/* @summary Calculate the next power-of-two value greater than or equal to a given value.
 * @param n The input value.
 */
static size_t
CORE__MemoryNextPow2GreaterOrEqual
(
    size_t n
)
{
    size_t i, k;
    --n;
    for (i = 1, k = sizeof(size_t) * CHAR_BIT; i < k; i <<= 1)
    {
        n |= n >> i;
    }
    return n+1;
}

/* @summary Push a block offset onto the free list for a given level.
 * @param alloc The CORE_BUDDY_MEMORY_ALLOCATOR_STATE instance to which the free block is being returned.
 * @param offset The byte offset of the start of the allocated block.
 * @param level The zero-based index of the level to which the block belongs.
 */
static void
CORE__BuddyAllocatorPushFreeOffset
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    uint32_t              offset,
    uint32_t               level
)
{
    uint32_t count = alloc->FreeCount[level];
    alloc->FreeLists[level][count] = offset;
    alloc->FreeCount[level] = count+1;
}

/* @summary Pop a block offset from the free list for a given level. The caller must ensure that the free list for the specified level is non-empty.
 * @param alloc The CORE_BUDDY_MEMORY_ALLOCATOR_STATE instance from which the free block is being obtained.
 * @param level The zero-based index of the level to which the block belongs.
 * @return The byte offset of the free block.
 */
static uint32_t
CORE__BuddyAllocatorPopFreeOffset
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    uint32_t               level
)
{
    uint32_t   count = alloc->FreeCount[level];
    uint32_t  offset = alloc->FreeLists[level][count-1];
    alloc->FreeCount[level] = count-1;
    return offset;
}

/* @summary Retrieve the information necessary to look up the status bit in the buddy allocator merge index for a block.
 * @param block Information about the memory block, retrieved by a prior call to CORE_BuddyAllocatorBlockInfo.
 * @return The merge index information, indicating which uint32_t to access and the mask used to access the bit for the buddy pair in the merge index.
 */
static CORE__BUDDY_BLOCK_MERGE_INFO
CORE__BuddyAllocatorMergeIndexInfo
(
    CORE__BUDDY_BLOCK_INFO *block
)
{
    CORE__BUDDY_BLOCK_MERGE_INFO info;
    info.WordIndex =  block->LeftAbsoluteIndex >> 5;
    info.Mask = 1 << (block->LeftAbsoluteIndex & 31);
    return info;
}

/* @summary Retrieve the information necessary to look up the status bit in the buddy allocator split index for a block.
 * @param block Information about the memory block, retrieved by a prior call to CORE_BuddyAllocatorBlockInfo.
 * @return The split index information, indicating which uint32_t to access and the mask used to access the bit for the block in the split index.
 */
static CORE__BUDDY_BLOCK_SPLIT_INFO
CORE__BuddyAllocatorSplitIndexInfo
(
    CORE__BUDDY_BLOCK_INFO *block
)
{
    CORE__BUDDY_BLOCK_SPLIT_INFO info;
    info.WordIndex =  block->BlockAbsoluteIndex >> 5;
    info.Mask = 1 << (block->BlockAbsoluteIndex & 31);
    return info;
}

/* @summary Query a memory allocator for attributes of an allocated block where the block size is known.
 * @param info The CORE__BUDDY_BLOCK_INFO to populate with information about the block at offset p.
 * @param alloc The CORE_MEMORY_ALLOCATOR to query. This must be the same allocator that returned the block.
 * @param p The offset of the block to query.
 * @param level The zero-based index of the level at which the block was allocated.
 */
static void
CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel
(
    CORE__BUDDY_BLOCK_INFO  *info,
    CORE_MEMORY_ALLOCATOR  *alloc,
    uint32_t                    p, 
    uint32_t                level
)
{
    uint32_t     level_shift = alloc->LevelBits[level];
    uint32_t     block_count = 1 << level;
    uint32_t     local_index = p >> level_shift;
    uint32_t       odd_index = local_index &  1;
    int32_t     buddy_offset = odd_index   ? -1 : +1; /* can remove this branch? */
    info->LevelIndex         = level;
    info->BitIndex           = level_shift;
    info->BlockSize          = 1 << level_shift;
    info->BlockCount         = block_count;
    info->IndexOffset        = block_count - 1;
    info->LeftAbsoluteIndex  = block_count + (local_index - odd_index) - 1;
    info->BlockAbsoluteIndex = block_count +  local_index - 1;
    info->BuddyAbsoluteIndex = block_count + (local_index + buddy_offset) - 1;
}

/* @summary Query a memory allocator for attributes of an allocated block where the block size is not known.
 * @param info The CORE__BUDDY_BLOCK_INFO to populate with information about the block at offset p.
 * @param alloc The CORE_MEMORY_ALLOCATOR to query. This must be the same allocator that returned the block.
 * @param p The offset of the block to query.
 * @return Zero if the block information was retrieved, or -1 if the block offset is invalid.
 */
static int
CORE__QueryBuddyAllocatorBlockInfo
(
    CORE__BUDDY_BLOCK_INFO *info, 
    CORE_MEMORY_ALLOCATOR *alloc, 
    uint32_t                   p
)
{
    CORE__BUDDY_BLOCK_INFO       block;
    CORE__BUDDY_BLOCK_SPLIT_INFO split;
    uint32_t level_index = alloc->LevelCount - 1;
    while   (level_index > 0)
    {   /* check the parent level to see if it's been split */
        CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(info, alloc, p, level_index-1);
        split = CORE__BuddyAllocatorSplitIndexInfo(&block);
        if ((alloc->SplitIndex[split.WordIndex] & split.Mask) != 0)
        {   /* reached a split parent; the block level is level_index */
            CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(info, alloc, p, level_index);
            return 0;
        }
        /* the parent has not been split, so check the next-largest level */
        level_index--;
    }
    if (p == alloc->MemoryStart)
    {   /* this is a level-0 block (there's only one) */
        CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(info, alloc, p, 0);
        return  0;
    }
    else
    {   /* the offset or address is invalid */
        return -1;
    }
}

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
    if (alloc_flags ==CORE_HOST_MEMORY_ALLOCATION_FLAGS_DEFAULT)
        alloc_flags = CORE_HOST_MEMORY_ALLOCATION_FLAGS_READWRITE;
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

CORE_API(int)
CORE_MemoryBlockIsValid
(
    CORE_MEMORY_BLOCK *block
)
{
    if (block->AllocatorType == CORE_MEMORY_ALLOCATOR_TYPE_HOST)
        return (block->HostAddress != NULL) ? 1 : 0;
    if (block->AllocatorType == CORE_MEMORY_ALLOCATOR_TYPE_DEVICE)
        return  1;
    else
        return  0;
}

CORE_API(int)
CORE_MemoryBlockDidMove
(
    CORE_MEMORY_BLOCK *old_block, 
    CORE_MEMORY_BLOCK *new_block
)
{
    return (new_block->BlockOffset == old_block->BlockOffset) ? 0 : 1;
}

CORE_API(int)
CORE_InitMemoryArena
(
    CORE_MEMORY_ARENA     *arena, 
    CORE_MEMORY_ARENA_INIT *init
)
{
    if (init->AllocatorType != CORE_MEMORY_ALLOCATOR_TYPE_HOST && 
        init->AllocatorType != CORE_MEMORY_ALLOCATOR_TYPE_DEVICE)
    {
        ZeroMemory(arena, sizeof(CORE_MEMORY_ARENA));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MemorySize == 0)
    {   assert(init->MemorySize != 0);
        ZeroMemory(arena, sizeof(CORE_MEMORY_ARENA));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->UserDataSize > CORE_MEMORY_ALLOCATOR_MAX_USER)
    {   assert(init->UserDataSize <= CORE_MEMORY_ALLOCATOR_MAX_USER);
        ZeroMemory(arena, sizeof(CORE_MEMORY_ARENA));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    ZeroMemory(arena, sizeof(CORE_MEMORY_ARENA));
    arena->AllocatorName = init->AllocatorName;
    arena->AllocatorType = init->AllocatorType;
    arena->MemoryStart   = init->MemoryStart;
    arena->MemorySize    = init->MemorySize;
    arena->NextOffset    = 0;
    arena->MaximumOffset = init->MemorySize;
    if (init->UserDataSize > 0)
    {
        CopyMemory(arena->UserData, init->UserData, (size_t) init->UserDataSize);
    }
    return 0;
}

CORE_API(int)
CORE_MemoryArenaAllocate
(
    CORE_MEMORY_ARENA *arena, 
    size_t              size, 
    size_t         alignment, 
    CORE_MEMORY_BLOCK *block
)
{
    uint64_t    base_address = arena->MemoryStart + arena->NextOffset;
    uint64_t aligned_address = base_address != 0 ? CORE_AlignUp(base_address, alignment) : 0;
    uint64_t     align_bytes = aligned_address - base_address;
    uint64_t     alloc_bytes = size + align_bytes;
    uint64_t      new_offset = arena->NextOffset + alloc_bytes;
    if (new_offset <= arena->MaximumOffset)
    {   /* the arena can satisfy the allocation */
        arena->NextOffset    = new_offset;
        block->SizeInBytes   = size;
        block->BlockOffset   = arena->NextOffset + align_bytes;
        block->HostAddress   = arena->AllocatorType == CORE_MEMORY_ALLOCATOR_TYPE_HOST ? (void*) aligned_address : NULL;
        block->AllocatorType = arena->AllocatorType;
        return 0;
    }
    else
    {   /* the arena cannot satisfy the allocation */
        block->SizeInBytes   = 0;
        block->BlockOffset   = 0;
        block->HostAddress   = NULL;
        block->AllocatorType = CORE_MEMORY_ALLOCATOR_TYPE_INVALID;
        return -1;
    }
}

CORE_API(void*)
CORE_MemoryArenaAllocateHost
(
    CORE_MEMORY_ARENA *arena, 
    size_t              size, 
    size_t         alignment, 
    CORE_MEMORY_BLOCK *block
)
{
    CORE_MEMORY_BLOCK  dummy;
    if (block == NULL) block = &dummy;
    if (CORE_MemoryArenaAllocate(arena, size, alignment, block) == 0)
    {   /* the allocation request was satisfied */
        return block->HostAddress;
    }
    else return NULL;
}

CORE_API(CORE_MEMORY_ARENA_MARKER)
CORE_MemoryArenaMark
(
    CORE_MEMORY_ARENA *arena
)
{
    return (CORE_MEMORY_ARENA_MARKER) arena->NextOffset;
}

CORE_API(void)
CORE_MemoryArenaResetToMarker
(
    CORE_MEMORY_ARENA        *arena,
    CORE_MEMORY_ARENA_MARKER marker
)
{   assert(marker <= arena->NextOffset);
    arena->NextOffset = marker;
}

CORE_API(void)
CORE_MemoryArenaReset
(
    CORE_MEMORY_ARENA *arena
)
{
    arena->NextOffset = 0;
}

CORE_API(size_t)
CORE_QueryMemoryAllocatorStateSize
(
    size_t allocation_size_min, 
    size_t allocation_size_max
)
{   
    unsigned long max_bit = 0;
    unsigned long min_bit = 0;
    size_t    level_count;
    size_t free_list_size;
    size_t     index_size;
    size_t     total_size;

    /* arguments must be powers of two greater than zero */
    if ((allocation_size_min & (allocation_size_min-1)) != 0 || allocation_size_min < 16)
        return 0;
    if ((allocation_size_max & (allocation_size_max-1)) != 0 || allocation_size_max < allocation_size_min)
        return 0;

    /* determine the number of levels and ensure the count doesn't exceed the limit */
#ifdef _M_X64
    _BitScanReverse64(&min_bit, allocation_size_min);
    _BitScanReverse64(&max_bit, allocation_size_max);
#else
    _BitScanReverse  (&min_bit, allocation_size_min);
    _BitScanReverse  (&max_bit, allocation_size_max);
#endif
    level_count    = ( max_bit-min_bit);
    free_list_size = (1 << level_count)    * 4; /* 4 = sizeof(uint32_t), magic number to avoid conversion warning */
    index_size     = (1 <<(level_count-1)) / 8; /* 8 = number of bits per-byte */
    total_size     = (2 *  index_size) + free_list_size;
    return total_size;
}

CORE_API(int)
CORE_InitMemoryAllocator
(
    CORE_MEMORY_ALLOCATOR     *alloc, 
    CORE_MEMORY_ALLOCATOR_INIT *init
)
{
    unsigned long  max_bit = 0;
    unsigned long  min_bit = 0;
    size_t       level_bit;
    size_t     level_index;
    size_t     level_count;
    size_t  free_list_size;
    size_t      index_size;
    size_t   required_size;
    size_t  total_mem_size = init->MemorySize + init->BytesReserved;

    /* basic parameter validation */
    if (init->AllocatorType != CORE_MEMORY_ALLOCATOR_TYPE_HOST && 
        init->AllocatorType != CORE_MEMORY_ALLOCATOR_TYPE_DEVICE)
    {
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MemorySize == 0)
    {   assert(init->MemorySize != 0);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if ((total_mem_size & (total_mem_size-1)) != 0)
    {   assert((total_mem_size & (total_mem_size-1)) == 0 && "init->MemorySize+init->BytesReserved must be a power-of-two");
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->UserDataSize > CORE_MEMORY_ALLOCATOR_MAX_USER)
    {   assert(init->UserDataSize <= CORE_MEMORY_ALLOCATOR_MAX_USER);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->StateData == NULL || init->StateDataSize == 0)
    {   assert(init->StateData != NULL);
        assert(init->SatteDataSize > 0);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    /* arguments must be powers of two greater than zero */
    if ((init->AllocationSizeMin & (init->AllocationSizeMin-1)) != 0 || init->AllocationSizeMin < 16)
    {   assert((init->AllocationSizeMin & (init->AllocationSizeMin-1) != 0) && "AllocationSizeMin must be a power-of-two");
        assert(init->AllocationSizeMin >= 16);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if ((init->AllocationSizeMax & (init->AllocationSizeMax-1)) != 0 || init->AllocationSizeMax < init->AllocationSizeMin)
    {   assert((init->AllocationSizeMax & (init->AllocationSizeMax-1) != 0) && "AllocationSizeMax must be a power-of-two");
        assert(init->AllocationSizeMin < init->AllocationSizeMax);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->BytesReserved >= init->AllocationSizeMax)
    {   assert(init->BytesReserved < init->AllocationSizeMax);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* determine the number of levels and ensure the count doesn't exceed the limit */
#ifdef _M_X64
    _BitScanReverse64(&min_bit, init->AllocationSizeMin);
    _BitScanReverse64(&max_bit, init->AllocationSizeMax);
#else
    _BitScanReverse  (&min_bit, init->AllocationSizeMin);
    _BitScanReverse  (&max_bit, init->AllocationSizeMax);
#endif
    level_bit      =   max_bit;
    level_count    = ( max_bit-min_bit);
    free_list_size = (1 << level_count)    * 4; /* 4 = sizeof(uint32_t), magic number to avoid conversion warning */
    index_size     = (1 <<(level_count-1)) / 8; /* 8 = number of bits per-byte */
    required_size  = (2 *  index_size) + free_list_size;
    if (init->StateDataSize < required_size)
    {   assert(init->StateDataSize >= required_size);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (level_count > CORE_BUDDY_ALLOCATOR_MAX_LEVELS)
    {   assert(level_count <= CORE_BUDDY_ALLOCATOR_MAX_LEVELS);
        ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* set up the allocator state */
    ZeroMemory(alloc, sizeof(CORE_MEMORY_ALLOCATOR));
    alloc->AllocatorName     = init->AllocatorName;
    alloc->AllocatorType     = init->AllocatorType;
    alloc->MemoryStart       = init->MemoryStart - init->BytesReserved;
    alloc->MemorySize        = init->MemorySize  + init->BytesReserved;
    alloc->AllocationSizeMin = init->AllocationSizeMin;
    alloc->AllocationSizeMax = init->AllocationSizeMax;
    alloc->BytesReserved     = init->BytesReserved;
    alloc->MetadataBase      =(uint8_t *)            init->StateData;
    alloc->FreeListData      =(uint32_t*)((uint8_t*) init->StateData + (index_size * 2));
    alloc->MergeIndex        =(uint32_t*)((uint8_t*) init->StateData + (index_size * 1));
    alloc->SplitIndex        =(uint32_t*)((uint8_t*) init->StateData + (index_size * 0));
    alloc->Reserved          = 0;
    alloc->LevelCount        =(uint32_t ) level_count;
    for (level_index = 0; level_index < level_count; ++level_index)
    {
        alloc->FreeCount[level_index] = 0;
        alloc->LevelBits[level_index] =(uint32_t) level_bit--;
        alloc->FreeLists[level_index] = alloc->FreeListData + ((1 << level_index) - 1);
    }
    CORE__BuddyAllocatorPushFreeOffset(alloc, 0, 0);

    /* sometimes the requirement of AllocationSizeMax being a power-of-two leads to 
     * significant memory waste, so allow the caller to specify a BytesReserved 
     * value to mark a portion of memory as unusable and make use of non-pow2 memory chunks. */
    if (init->BytesReserved > 0)
    {   /* allocate small blocks until BytesReserved is met. 
           allocating the smallest block size ensures the least amount of waste.
           contiguous blocks will be allocated, starting from offset 0. */
        uint32_t  level_size = 1 << min_bit;
        uint32_t block_count =(uint32_t)((init->BytesReserved + level_size) / level_size);
        uint32_t block_index;
        CORE_MEMORY_BLOCK  b;
        for (block_index = 0; block_index < block_count; ++block_index)
        {
            (void) CORE_MemoryAllocate(alloc, level_size, level_size, &b);
        }
    }
    if (init->UserData != NULL && init->UserDataSize > 0)
    {
        CopyMemory(alloc->UserData, init->UserData, init->UserDataSize);
    }
    return 0;
}

CORE_API(int)
CORE_MemoryAllocate
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    size_t                  size, 
    size_t             alignment, 
    CORE_MEMORY_BLOCK     *block
)
{
    uint32_t      pow2_size;
    uint32_t      level_idx;
    uint32_t      check_idx;
    uint32_t  parent_offset;
    uint32_t  return_offset;
    unsigned long bit_index;
    CORE__BUDDY_BLOCK_INFO      parent_info;
    CORE__BUDDY_BLOCK_INFO       block_info;
    CORE__BUDDY_BLOCK_MERGE_INFO merge_info;
    CORE__BUDDY_BLOCK_SPLIT_INFO split_info;

    if (size < alignment)
    {   /* round up to the requested alignment */
        size = alignment;
    }
    if (size < alloc->AllocationSizeMin)
    {   /* round up to the minimum possible block size */
        size =(size_t) alloc->AllocationSizeMin;
    }
    if (alignment > alloc->AllocationSizeMin)
    {   assert(alignment <= alloc->AllocationSizeMin);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* round the requested allocation size up to the nearest power of two >= size */
    if ((pow2_size =(uint32_t) CORE__MemoryNextPow2GreaterOrEqual(size)) > alloc->AllocationSizeMax)
    {   assert(pow2_size <= alloc->AllocationSizeMax);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* figure out what level the specified size corresponds to */
    _BitScanReverse((DWORD*)&bit_index, pow2_size);
    level_idx = alloc->LevelBits[0] - bit_index;
    check_idx = level_idx;

    /* search for a free block to satisfy the allocation request */
    for ( ; ; )
    {   /* if the free list is not empty, the allocation can be satisfied at this level */
        if (alloc->FreeCount[check_idx] > 0)
        {   /* perform splits from check_idx (<= level_idx) to level_idx.
               this process splits larger blocks into multiple smaller blocks. */
            while (check_idx < level_idx)
            {   /* pop an offset from the parent level's free list and split */
                parent_offset = CORE__BuddyAllocatorPopFreeOffset(alloc, check_idx);
                CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&parent_info, alloc, parent_offset, check_idx);
                split_info = CORE__BuddyAllocatorSplitIndexInfo(&parent_info);

                /* mark the larger parent block as having been split */
                alloc->SplitIndex[split_info.WordIndex] |= split_info.Mask;

                /* insert two blocks into the free list of the next-smaller level */
                CORE__BuddyAllocatorPushFreeOffset(alloc, parent_offset                               , check_idx+1);
                CORE__BuddyAllocatorPushFreeOffset(alloc, parent_offset + (parent_info.BlockSize >> 1), check_idx+1);
                check_idx++;
            }

            /* return a block from level_idx, which is known to have at least one free block */
            return_offset = CORE__BuddyAllocatorPopFreeOffset(alloc, level_idx);
            CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, return_offset, level_idx);
            merge_info = CORE__BuddyAllocatorMergeIndexInfo(&block_info);

            /* toggle the buddy pair bit representing the pair allocation state.
             * if both blocks are allocated, the bit will be set to 0.
             * if only the block at return_offset is allocated, the bit will be set to 1. */
            alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;
            block->SizeInBytes   = block_info.BlockSize;
            block->BlockOffset   = return_offset;
            block->HostAddress   = alloc->AllocatorType == CORE_MEMORY_ALLOCATOR_TYPE_HOST ? ((uint8_t*) alloc->MemoryStart + return_offset) : NULL;
            block->AllocatorType = alloc->AllocatorType;
            return 0;
        }
        if (check_idx != 0)
        {   /* check the next larger level to see if there are any free blocks */
            check_idx--;
        }
        else
        {   /* there are no free blocks that can satisfy the request */
            ZeroMemory(block, sizeof(CORE_MEMORY_BLOCK));
            return -1;
        }
    }
}

CORE_API(void*)
CORE_MemoryAllocateHost
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    size_t                  size, 
    size_t             alignment, 
    CORE_MEMORY_BLOCK     *block
)
{
    CORE_MemoryAllocate(alloc, size, alignment, block);
    return block->HostAddress;
}

CORE_API(int)
CORE_MemoryReallocate
(
    CORE_MEMORY_ALLOCATOR *                   alloc, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT  existing, 
    size_t                                 new_size, 
    size_t                                alignment, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT new_block
)
{
    size_t            i, n;
    uint32_t    offset_u32;
    uint32_t  *free_offset;
    uint32_t pow2_size_old;
    uint32_t pow2_size_new;
    uint32_t level_idx_old;
    uint32_t level_idx_new;
    uint32_t  merge_offset;
    uint32_t  buddy_offset;
    unsigned long bit_index_old;
    unsigned long bit_index_new;
    CORE__BUDDY_BLOCK_INFO       block_info;
    CORE__BUDDY_BLOCK_MERGE_INFO merge_info;
    CORE__BUDDY_BLOCK_SPLIT_INFO split_info;

    if (existing->SizeInBytes == 0)
    {   /* there is no existing allocation, so forward to the base allocation routine */
        return CORE_MemoryAllocate(alloc, new_size, alignment, new_block);
    }
    if (new_size < alignment)
    {   /* round up to the requested alignment */
        new_size = alignment;
    }
    if (new_size < alloc->AllocationSizeMin)
    {   /* round up to the minimum possible block size */
        new_size =(size_t) alloc->AllocationSizeMin;
    }
    if (alignment > alloc->AllocationSizeMin)
    {   assert(alignment <= alloc->AllocationSizeMin);
        ZeroMemory(new_block, sizeof(CORE_MEMORY_BLOCK));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* there are four scenarios this routine has to account for:
     * 1. The new_size still fits in the same block. No re-allocation is performed.
     * 2. The new_size is larger than the old size, but fits in a block one level larger, and the buddy block is free. The existing block is promoted to the next-larger level.
     * 3. The new_size is smaller than the old size by one or more levels. The existing block is demoted to a smaller block.
     * 4. The new_size is larger than the old size by more than one level, or the buddy was not free. A new, larger block is allocated and the existing block is freed.
     */
    if ((pow2_size_new =(uint32_t) CORE__MemoryNextPow2GreaterOrEqual(new_size)) > alloc->AllocationSizeMax)
    {   assert(pow2_size_new <= alloc->AllocationSizeMax);
        ZeroMemory(new_block, sizeof(CORE_MEMORY_BLOCK));
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    offset_u32    = (uint32_t) existing->BlockOffset;
    pow2_size_old = (uint32_t) existing->SizeInBytes;
    _BitScanReverse(&bit_index_old, pow2_size_old);
    _BitScanReverse(&bit_index_new, pow2_size_new);
    level_idx_old = alloc->LevelBits[0] - bit_index_old;
    level_idx_new = alloc->LevelBits[0] - bit_index_new;

    if (level_idx_new == level_idx_old)
    {   /* case 1: the new_size fits in the same block. don't do anything. */
        CopyMemory(new_block, existing, sizeof(CORE_MEMORY_BLOCK));
        return 0;
    }
    if (level_idx_new ==(level_idx_old-1))
    {   /* case 2: if the buddy is free, promote the existing block */
        CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, offset_u32, level_idx_old);
        merge_info =  CORE__BuddyAllocatorMergeIndexInfo(&block_info);
        if ((alloc->MergeIndex[merge_info.WordIndex] & merge_info.Mask) != 0)
        {   /* the buddy block is free - merge it with the existing block.
               toggle the status bit to 0 - both blocks are currently allocated. */
            alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;

            /* scan the free list to locate the buddy block offset, and remove it */
            merge_offset  = (block_info.LeftAbsoluteIndex  - block_info.IndexOffset) * block_info.BlockSize;
            buddy_offset  = (block_info.BuddyAbsoluteIndex - block_info.IndexOffset) * block_info.BlockSize;
            free_offset   =  alloc->FreeLists[level_idx_old];
            for (i = 0, n =  alloc->FreeCount[level_idx_old]; i < n; ++i)
            {
                if (free_offset[i] == buddy_offset)
                {   /* found the matching offset. swap the last item into its place. */
                    alloc->FreeCount[level_idx_old]--;
                    free_offset[i] = free_offset[n-1];
                    break;
                }
            }

            /* retrieve the attributes of the parent block */
            CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, merge_offset, level_idx_new);
            merge_info  = CORE__BuddyAllocatorMergeIndexInfo(&block_info);
            split_info  = CORE__BuddyAllocatorSplitIndexInfo(&block_info);

            /* mark the parent block as allocated, and clear its split status */
            alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;
            alloc->SplitIndex[split_info.WordIndex] &=~split_info.Mask;

            /* return the merged block */
            new_block->SizeInBytes   = block_info.BlockSize;
            new_block->BlockOffset   = merge_offset;
            new_block->HostAddress   = alloc->AllocatorType == CORE_MEMORY_ALLOCATOR_TYPE_HOST ? ((uint8_t*) alloc->MemoryStart + merge_offset) : NULL;
            new_block->AllocatorType = alloc->AllocatorType;
            return 0;
        }
    }
    if (level_idx_new > level_idx_old)
    {   /* case 3: demote the existing block to a smaller size; no copy required */
        CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, offset_u32, level_idx_old);
        merge_info  = CORE__BuddyAllocatorMergeIndexInfo(&block_info);

        /* mark the current block as being free */
        alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;

        /* perform splits down to the necessary block size */
        while (level_idx_old < level_idx_new)
        {   /* update the split index to mark the parent block as having been split */
            split_info = CORE__BuddyAllocatorSplitIndexInfo(&block_info);
            alloc->SplitIndex[split_info.WordIndex] |= split_info.Mask;
            CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, offset_u32, block_info.LevelIndex+1);
            CORE__BuddyAllocatorPushFreeOffset(alloc, offset_u32 + block_info.BlockSize, block_info.LevelIndex);
            level_idx_old++;
        }

        /* mark the new smaller block as allocated in the merge index */
        merge_info = CORE__BuddyAllocatorMergeIndexInfo(&block_info);
        alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;
        new_block->SizeInBytes   = block_info.BlockSize;
        new_block->BlockOffset   = existing->BlockOffset;
        new_block->HostAddress   = existing->HostAddress;
        new_block->AllocatorType = alloc->AllocatorType;
        return 0;
    }
    /* case 4: no choice but to allocate a new block, copy data, and free the old block */
    if (CORE_MemoryAllocate(alloc, new_size, alignment, new_block) < 0)
    {   /* allocation of the new block failed */
        return -1;
    }
    /* mark the old block as being free */
    CORE_MemoryFree(alloc, existing);
    return 0;
}

CORE_API(void*)
CORE_MemoryReallocateHost
(
    CORE_MEMORY_ALLOCATOR *                   alloc, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT  existing, 
    size_t                                 new_size, 
    size_t                                alignment, 
    CORE_MEMORY_BLOCK     * CORE_RESTRICT new_block
)
{
    if (CORE_MemoryReallocate(alloc, existing, new_size, alignment, new_block) == 0)
    {   /* the reallocation request was successful. does the data need to be moved? */
        if (new_block->HostAddress != existing->HostAddress)
        {   /* the memory location changed; the data needs to be copied */
            CopyMemory(new_block->HostAddress, existing->HostAddress, (size_t) new_block->SizeInBytes);
        }
        return new_block->HostAddress;
    }
    else return NULL;
}

CORE_API(void)
CORE_MemoryFree
(
    CORE_MEMORY_ALLOCATOR *alloc, 
    CORE_MEMORY_BLOCK  *existing
)
{
    if (existing->SizeInBytes >= alloc->AllocationSizeMin)
    {
        uint32_t                           i, n;
        uint32_t                     offset_u32 = (uint32_t) existing->BlockOffset;
        uint32_t                      pow2_size = (uint32_t) existing->SizeInBytes;
        uint32_t                      level_idx =  0;
        unsigned long                 bit_index =  0;
        uint32_t                   merge_offset;
        uint32_t                   buddy_offset;
        uint32_t                   *free_offset;
        CORE__BUDDY_BLOCK_INFO       block_info;
        CORE__BUDDY_BLOCK_MERGE_INFO merge_info;
        CORE__BUDDY_BLOCK_SPLIT_INFO split_info;

        /* convert the size into a level index */
        _BitScanReverse(&bit_index, pow2_size);
        level_idx = alloc->LevelBits[0] - bit_index;

        /* mark the block as free in the merge index */
        CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, offset_u32, level_idx);
        merge_info  = CORE__BuddyAllocatorMergeIndexInfo(&block_info);
        alloc->MergeIndex[merge_info.WordIndex] ^= merge_info.Mask;
        do
        {   /* if the new state is 0, the block and its buddy can be merged.
               if the new state is 1, only the block is free and we're done. */
            if ((block_info.LevelIndex == 0) || ((alloc->MergeIndex[merge_info.WordIndex] & merge_info.Mask) != 0))
            {   /* no additional merging can be performed */
                break;
            }

            /* perform a merge operation between the buddy pair.
             * remove the buddy block from the free list.
             * the merged block is the lowest offset of the pair. */
            merge_offset  =(block_info.LeftAbsoluteIndex  - block_info.IndexOffset) * block_info.BlockSize;
            buddy_offset  =(block_info.BuddyAbsoluteIndex - block_info.IndexOffset) * block_info.BlockSize;
            free_offset   = alloc->FreeLists[block_info.LevelIndex];
            for (i = 0, n = alloc->FreeCount[block_info.LevelIndex]; i < n; ++i)
            {
                if (free_offset[i] == buddy_offset)
                {   /* found the matching offset. swap the last item into its place */
                    alloc->FreeCount[block_info.LevelIndex] = n-1;
                    free_offset[i] = free_offset[n-1];
                    break;
                }
            }

            /* clear the split status for the parent block */
            CORE__QueryBuddyAllocatorBlockInfoWithKnownLevel(&block_info, alloc, merge_offset, block_info.LevelIndex-1);
            merge_info  = CORE__BuddyAllocatorMergeIndexInfo(&block_info);
            split_info  = CORE__BuddyAllocatorSplitIndexInfo(&block_info);
            alloc->SplitIndex[split_info.WordIndex] &= ~split_info.Mask;

            /* continue trying to merge into larger blocks */
            offset_u32 = merge_offset;
        } while (block_info.LevelIndex != 0);
        /* return the possibly merged block back to the free list for the level it was allocated from */
        CORE__BuddyAllocatorPushFreeOffset(alloc, offset_u32, block_info.LevelIndex);
    }
}

CORE_API(void)
CORE_MemoryAllocatorReset
(
    CORE_MEMORY_ALLOCATOR *alloc
)
{   /* return the merge and split indexes to their initial state.
     * zero out all of the free list entries.
     * return level 0 block 0 to the free list. */
    ZeroMemory(alloc->MergeIndex, (1 << (alloc->LevelCount-1)) / 8);
    ZeroMemory(alloc->SplitIndex, (1 << (alloc->LevelCount-1)) / 8);
    ZeroMemory(alloc->FreeCount , CORE_BUDDY_ALLOCATOR_MAX_LEVELS * sizeof(uint32_t));
    CORE__BuddyAllocatorPushFreeOffset(alloc, 0, 0);
    if (alloc->BytesReserved > 0)
    {   /* allocate small blocks until BytesReserved is met. 
           allocating the smallest block size ensures the least amount of waste.
           contiguous blocks will be allocated, starting from offset 0. */
        uint32_t  level_size = 1 << alloc->LevelBits[alloc->LevelCount-1];
        uint32_t block_count =(uint32_t)((alloc->BytesReserved + level_size) / level_size);
        uint32_t block_index;
        CORE_MEMORY_BLOCK  b;
        for (block_index = 0; block_index < block_count; ++block_index)
        {
            (void) CORE_MemoryAllocate(alloc, level_size, level_size, &b);
        }
    }
}

#endif /* CORE_MEMORY_IMPLEMENTATION */

