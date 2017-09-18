/*
 * CORE_data.h: A single-file library for working with data-oriented objects, 
 * where objects are uniquely identified by a handle, and the data comprising 
 * the objects are spread across one or more tables. This module also includes
 * support for string management via string tables, and fast random-access and 
 * sequential access to data via table indices.
 *
 * This software is dual-licensed to the public domain and under the following 
 * license: You are hereby granted a perpetual, irrevocable license to copy, 
 * modify, publish and distribute this file as you see fit.
 *
 */
#ifndef __CORE_DATA_H__
#define __CORE_DATA_H__

// What the handle table really needs is to be straightforward.
// Indices i are recycled using an array-based FIFO. This requires Nmax dwords.
// Two additional arrays, sparse and dense are needed. This requires 2 * Nmax dwords.
// Finally, a generation needs to be kept, which requires Nmax bytes.
//
// CREATE does:
// i = pop_freelist(); // i is an index in [0, MAX_OBJECTS)
// n = item_count; // n is an index in [0, MAX_OBJECTS)
// generation[n] = (generation[n] + GEN_PLUS_ONE) & GENERATION_MASK;
// dense[n] = i;
// sparse[i] = n;
// handle = MakeHandle(type, generation[n], i);
// n++;
// return handle;
//
// IS_MEMBER(h) does:
// i = (h & INDEX_MASK)      >> INDEX_SHIFT;
// g = (h & GENERATION_MASK) >> GENERATION_SHIFT;
// n = item_count;
// return true if sparse[i] < n && dense[sparse[i]] == i && generation[sparse[i]] == g
//
// DELETE(h) does:
// i = (h & INDEX_MASK)      >> INDEX_SHIFT;
// g = (h & GENERATION_MASK) >> GENERATION_SHIFT;
// n = item_count;
// j = dense[n-1];
// dense[sparse[i]] = j;
// generation[sparse[i]] = (generation[sparse[i]] + GEN_PLUS_ONE) & GENERATION_MASK; // so IS_MEMBER(h) will return false
// sparse[j] = sparse[i];
// item_count--;
// push_freelist(i)

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* @summary Define various constants associated with the system. The maxmimum 
 * number of unique type identifiers is 32 [0, 31] and the maximum number of 
 * objects of any one type is around 4 million. If these need to be adjusted 
 * for your application, you can either decrease the maximum number of objects
 * or decrease the maximum generation. The number of generations is currently 
 * 31 before the same handle values can reappear.
 * CORE_DATA_MIN_TYPEID : The smallest valid object type identifier.
 * CORE_DATA_MAX_TYPEID : The largest valid object type identifier.
 * CORE_DATA_MIN_OBJECTS: The minimum number of object IDs that can be returned by a handle table.
 * CORE_DATA_MAX_OBJECTS: The maximum number of object IDs that can be returned by a handle table.
 */
#ifndef CORE_DATA_CONSTANTS
#define CORE_DATA_CONSTANTS
#define CORE_DATA_MIN_TYPEID              0
#define CORE_DATA_MAX_TYPEID              31
#define CORE_DATA_MIN_OBJECTS             2
#define CORE_DATA_MAX_OBJECTS             4194303UL
#define CORE_DATA_ID_INDEX_MASK           0x003FFFFFUL
#define CORE_DATA_ID_INDEX_SHIFT          0
#define CORE_DATA_ID_GENERATION_MASK      0x07C00000UL
#define CORE_DATA_ID_GENERATION_SHIFT     22
#define CORE_DATA_ID_TYPEID_MASK          0xF8000000UL
#define CORE_DATA_ID_TYPEID_SHIFT         27
#define CORE_DATA_ID_OBJECTID_MASK       (CORE_DATA_ID_INDEX_MASK | CORE_DATA_ID_GENERATION_MASK)
#define CORE_DATA_ID_NEW_OBJECT_ADD      (1UL << CORE_DATA_ID_GENERATION_SHIFT)
#define CORE_DATA_TYPEID_COUNT           (CORE_DATA_MAX_TYPEID+1)
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

/* @summary Construct an object handle given a type ID and object ID.
 * @param _typeid The type identifier associated with the handle table.
 * @param _objectid The combined generation and index value for the object within the handle table.
 */
#ifndef CORE_MakeHandle
#define CORE_MakeHandle(_typeid, _objectid)                                    \
    ((((_typeid) & 0x1FUL) << CORE_DATA_HANDLE_TYPEID_SHIFT) | (((_objectid) & CORE_DATA_HANDLE_OBJECTID_MASK)))
#endif

/* @summary Extract the object identifier from an object handle.
 * @param _handle The CORE_DATA_HANDLE to examine.
 * @return The object identifier (generation + index) of the object within the handle table.
 */
#ifndef CORE_GetObjectId
#define CORE_GetObjectId(_handle)                                              \
    ((_handle) & CORE_DATA_HANDLE_OBJECTID_MASK)
#endif

/* @summary Extract the object type identifier from an object handle.
 * @param _handle The CORE_DATA_HANDLE to examine.
 * @return The object type identifier for the object.
 */
#ifndef CORE_GetObjectType
#define CORE_GetObjectType(_handle)                                            \
    (((_handle) & CORE_DATA_HANDLE_TYPEID_MASK) >> CORE_DATA_HANDLE_TYPEID_SHIFT)
#endif

/* Forward-declare types exported by the library */
struct _CORE_DATA_HANDLE_TABLE;
struct _CORE_DATA_HANDLE_TABLE_INIT;
struct _CORE_DATA_HANDLE_INDEX4;
struct _CORE_DATA_HANDLE_INDEX8;
struct _CORE_DATA_HANDLE_INDEX;
struct _CORE_DATA_HANDLE_INDEX_INIT;
struct _CORE_DATA_STRING_TABLE;
struct _CORE_DATA_STRING_TABLE_INIT;

/* @summary Object identifiers are opaque 32-bit integers.
 */
typedef uint32_t CORE_DATA_HANDLE;

/* @summary Define the data used to configure a handle table.
 */
typedef struct _CORE_DATA_HANDLE_TABLE_INIT {
    uint32_t                        TypeId;          /* The object type identifier for all objects returned by the handle table. */
    uint32_t                        MaxObjects;      /* The maximum number of live objects of the given type at any one time. */
    void                           *MemoryStart;     /* Pointer to the start of the memory block used for the table data. */
    uint64_t                        MemorySize;      /* The number of bytes in the memory block used for the table data. */
} CORE_DATA_HANDLE_TABLE_INIT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Compute the minimum number of bytes required to initialize a handle table with the given capacity.
 * @param max_objects The maximum number of live objects within the table at any given time.
 * @return The minimum number of bytes required for the memory block used to store the table data.
 */
CORE_API(size_t)
CORE_QueryHandleTableMemorySize
(
    uint32_t max_objects
);

/* @summary Initialize an object handle table to empty using memory supplied by the caller.
 * @param table On return, this location points to the newly initialized handle table.
 * @param init Data used to configure the handle table object.
 * @return Zero if the table is successfully created, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateHandleTable
(
    struct _CORE_DATA_HANDLE_TABLE **table, 
    CORE_DATA_HANDLE_TABLE_INIT      *init
);

/* @summary Reset a handle table to empty, invalidating all live objects.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The object handle table to reset.
 */
CORE_API(void)
CORE_ResetHandleTable
(
    struct _CORE_DATA_HANDLE_TABLE *table
);

/* @summary Allocate identifiers for one or more objects. 
 * The operation fails if the table cannot allocate the requested number of objects.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The table from which the object identifiers will be allocated.
 * @param handles The array of count handles to populate with allocated object identifiers.
 * @param count The number of object identifiers to allocate.
 * @return Zero if count object identifiers were successfully allocated, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_HANDLE             *handles, 
    size_t                          count
);

/* @summary Delete identifiers for one or more objects.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The table from which the object identifiers were allocated.
 * @param handles The array of count handles specifying the object identifiers to delete.
 * @param count The number of object identifiers to delete.
 */
CORE_API(void)
CORE_DeleteObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_HANDLE             *handles, 
    size_t                          count
);

/* @summary Query an object handle table for a set of identifiers of live objects.
 * This operation acquires a shared lock on the table - read operations may proceed concurrently.
 * @param table The handle table to query.
 * @param handles An array of max_handles locations where live object handles will be written, starting from index 0.
 * @param max_handles The maximum number of live object handles to write to the handles array.
 * @param num_handles On return, this location stores the number of object handles written to the handles array.
 * @param start_index The index into the table at which to start reading live object handles.
 * @param num_remaining On return, this location stores the number of live object handles remanining in the table.
 * @return Non-zero if at least one live object handle was returned, or zero otherwise.
 */
CORE_API(int)
CORE_QueryObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_HANDLE             *handles, 
    size_t                    max_handles, 
    size_t                   *num_handles, 
    size_t                    start_index, 
    size_t                 *num_remaining
);

/* @summary Given a set of object handles, filter the set into two lists - handles of objects that are still alive, and handles of objects that are dead.
 * This operation acquires a shared lock on the table - read operations may proceed concurrently.
 * @param table The handle table to query.
 * @param result_handles An array of check_count values used for storing the results of the filter operation.
 * @param live_objects On return, this location points to the first live object handle in result_handles.
 * @param live_count On return, this location specifies the number of live object handles in result_handles.
 * @param dead_objects On return, this location points to the first dead object handle in result_handles.
 * @param dead_count On return, this location specifies the number of dead object handles in result_handles.
 * @param check_handles The set of check_count object handles to check.
 * @param check_count The number of object handles to check.
 */
CORE_API(void)
CORE_FilterObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_HANDLE      *result_handles,
    CORE_DATA_HANDLE       **live_objects,
    size_t                    *live_count,
    CORE_DATA_HANDLE       **dead_objects, 
    size_t                    *dead_count, 
    CORE_DATA_HANDLE       *check_handles, 
    size_t                    check_count
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __CORE_DATA_H__ */

#ifdef CORE_DATA_IMPLEMENTATION

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an instance of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 */
#ifndef CORE__DataAllocationSizeType
#define CORE__DataAllocationSizeType(_type)                                    \
    ((sizeof(_type)) + (__alignof(_type)-1))
#endif

/* @summary For a given type, calculate the maximum number of bytes that will need to be allocated for an array of instances of that type, accounting for the padding required for proper alignment.
 * @param _type A typename, such as int, specifying the type whose allocation size is being queried.
 * @param _count The number of elements in the array.
 */
#ifndef CORE__DataAllocationSizeArray
#define CORE__DataAllocationSizeArray(_type, _count)                           \
    ((sizeof(_type) * (_count)) + (__alignof(_type)-1))
#endif

/* @summary Allocate host memory with the correct size and alignment for an instance of a given type from a memory arena.
 * @param _arena The CORE__DATA_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__DataMemoryArenaAllocateType
#define CORE__DataMemoryArenaAllocateType(_arena, _type)                       \
    ((_type*) CORE__DataMemoryArenaAllocateHost((_arena), sizeof(_type), __alignof(_type)))
#endif

/* @summary Allocate memory with the correct size and alignment for an array of instance of a given type from a memory arena.
 * @param _arena The CORE__DATA_ARENA from which the allocation is being made.
 * @param _type A typename, such as int, specifying the type being allocated.
 * @param _count The number of elements in the array.
 * @return A pointer to the start of the allocated memory block, or NULL.
 */
#ifndef CORE__DataMemoryArenaAllocateArray
#define CORE__DataMemoryArenaAllocateArray(_arena, _type, _count)              \
    ((_type*) CORE__DataMemoryArenaAllocateHost((_arena), sizeof(_type) * (_count), __alignof(_type)))
#endif

/* @summary Define the data associated with an internal memory arena allocator.
 */
typedef struct _CORE__DATA_ARENA {
    uint8_t *BaseAddress;            /* The base address of the memory range. */
    size_t   MemorySize;             /* The size of the memory block, in bytes. */
    size_t   NextOffset;             /* The offset of the next available address. */
} CORE__DATA_ARENA;

/* @summary Define the data stored with an object index, used by the handle table.
 * Object index data is stored in a sparse array within the handle table.
 */
typedef struct _CORE__DATA_INDEX {
    uint32_t          ObjectId;      /* The packed generation and dense array index for the object. */
    uint32_t          Next;          /* The index of the next item in the sparse array free list. */
} CORE__DATA_INDEX;

/* @summary Define the data associated with a handle table. The handle table maintains two arrays, Index, which is sparse, and Store, which is dense.
 * Object slots are re-used in FIFO order to prevent duplicate handles from being returned too quickly.
 */
typedef struct _CORE_DATA_HANDLE_TABLE {
    SRWLOCK           RWLock;        /* The reader-writer lock protecting the table. */
    CORE__DATA_INDEX *Index;         /* The sparse index array, which points into the dense object array. */
    uint32_t         *Store;         /* The dense object array, which stores the ObjectId for all live objects. */
    uint32_t          ObjectType;    /* The object type identifier for the objects managed by this table. */
    uint32_t          ObjectCount;   /* The number of live objects in the table. Store indices [0, ObjectCount) are valid. */
    uint32_t          FreelistTail;  /* The zero-based index into the Index array of the most recently freed object. */
    uint32_t          FreelistHead;  /* The zero-based index into the Index array of the oldest free object. */
    uint32_t          TableCapacity; /* The maximum number of objects that can be live at any one time. */
    uint32_t          Reserved;      /* Reserved for future use. Set to 0. */
    void             *MemoryStart;   /* The address of the start of the memory block reserved for table data. */
    uint64_t          MemorySize;    /* The number of bytes in the memory block reserved for table data. */
} CORE__DATA_HANDLE_TABLE;

/* @summary Initialize a memory arena allocator around an externally-managed memory block.
 * @param arena The memory arena allocator to initialize.
 * @param memory A pointer to the start of the memory block to sub-allocate from.
 * @param memory_size The size of the memory block, in bytes.
 */
static void
CORE__DataInitMemoryArena
(
    CORE__DATA_ARENA *arena, 
    void            *memory,
    size_t      memory_size
)
{
    arena->BaseAddress =(uint8_t*) memory;
    arena->MemorySize  = memory_size;
    arena->NextOffset  = 0;
}

/* @summary Sub-allocate memory from an arena.
 * @param arena The memory arena from which the memory will be allocated.
 * @param size The minimum number of bytes to allocate.
 * @param alignment The required alignment of the returned address, in bytes. This value must be zero, or a power-of-two.
 * @return A pointer to the start of a memory block of at least size bytes, or NULL.
 */
static void*
CORE__DataMemoryArenaAllocateHost
(
    CORE__DATA_ARENA *arena, 
    size_t             size, 
    size_t        alignment
)
{
    uintptr_t    base_address = (uintptr_t) arena->BaseAddress + arena->NextOffset;
    uintptr_t aligned_address =  CORE_AlignUp(base_address, alignment);
    size_t        align_bytes =  aligned_address - base_address;
    size_t        alloc_bytes =  size + align_bytes;
    size_t         new_offset =  arena->NextOffset + alloc_bytes;
    if (new_offset < arena->MemorySize)
    {   /* the arena can satisfy the allocation */
        arena->NextOffset = new_offset;
        return (void*) aligned_address;
    }
    else
    {   /* the arena cannot satisfy the allocation */
        return NULL;
    }
}

/* @summary Invalidate all allocations made from a memory arena.
 * @param arena The memory arena allocator to roll back.
 */
static void
CORE__DataResetMemoryArena
(
    CORE__DATA_ARENA *arena
)
{
    arena->NextOffset = 0;
}

CORE_API(size_t)
CORE_QueryHandleTableMemorySize
(
    uint32_t max_objects
)
{
    size_t required_size = 0;
    /* calculate the amount of memory required for the data stored directly in _CORE_DATA_HANDLE_TABLE.
     * this includes the size of the structure itself, since all data is private.
     */
    required_size += CORE__DataAllocationSizeType (CORE__DATA_HANDLE_TABLE);
    required_size += CORE__DataAllocationSizeArray(CORE__DATA_INDEX, max_objects+1);
    required_size += CORE__DataAllocationSizeArray(uint32_t        , max_objects+1);
    return required_size;
}

CORE_API(int)
CORE_CreateHandleTable
(
    struct _CORE_DATA_HANDLE_TABLE **table, 
    CORE_DATA_HANDLE_TABLE_INIT      *init
)
{
    CORE__DATA_ARENA  arena;
    CORE__DATA_HANDLE_TABLE *stor = NULL;
    size_t          required_size = 0;
    uint32_t                 i, n;

    if (init->TypeId > CORE_DATA_MAX_TYPEID)
    {   /* the object type identifier is invalid */
        assert(init->TypeId <= CORE_DATA_MAX_TYPEID);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MaxObjects < CORE_DATA_MIN_OBJECTS || 
        init->MaxObjects > CORE_DATA_MAX_OBJECTS)
    {   /* the table maximum object count is invalid */
        assert(init->MaxObjects >= CORE_DATA_MIN_OBJECTS);
        assert(init->MaxObjects <= CORE_DATA_MAX_OBJECTS);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MemoryStart == NULL || init->MemorySize == 0)
    {   /* the caller must supply memory for the table data */
        assert(init->MemoryStart != NULL);
        assert(init->MemorySize > 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    required_size = CORE_QueryHandleTableMemorySize(init->MaxObjects);
    if (init->MemorySize < required_size)
    {   /* the caller must supply sufficient memory for the table */
        assert(init->MemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* zero-initialize the entire memory block and allocate the base structure */
    ZeroMemory(init->MemoryStart, (size_t) init->MemorySize);
    CORE__DataInitMemoryArena(&arena, init->MemoryStart, (size_t) init->MemorySize);
    stor                = CORE__DataMemoryArenaAllocateType (&arena, CORE__DATA_HANDLE_TABLE);
    stor->RWLock        = SRWLOCK_INIT;
    stor->Index         = CORE__DataMemoryArenaAllocateArray(&arena, CORE__DATA_INDEX, init->MaxObjects+1);
    stor->Store         = CORE__DataMemoryArenaAllocateArray(&arena, uint32_t        , init->MaxObjects+1);
    stor->ObjectType    = init->TypeId;
    stor->ObjectCount   = 0;
    stor->FreeListTail  = init->MaxObjects;
    stor->FreeListHead  = 0;
    stor->TableCapacity = init->MaxObjects + 1;
    stor->Reserved      = 0;
    stor->MemoryStart   = init->MemoryStart;
    stor->MemorySize    = init->MemorySize;
    for (i = 0, n = init->MaxObjects + 1; i < n; ++i)
    {
        stor->Index[i].ObjectId = i; /* generation = 0, dense index = i */
        stor->Index[i].Next = i + 1; /* next free sparse index */
    }
   *table = stor;
    return 0;
}

CORE_API(void)
CORE_ResetHandleTable
(
    struct _CORE_DATA_HANDLE_TABLE *table
)
{
    AcquireSRWLockExclusive(&table->RWLock);
    {
        uint32_t i, n;
        for (i = 0, n = table->TableCapacity; i < n; ++i)
        {
            table->Index[i].ObjectId = i; /* generation = 0, dense index = i */
            table->Index[i].Next = i + 1; /* next free sparse index */
        }
        table->ObjectCount = 0;
    }
    ReleaseSRWLockExclusive(&table->RWLock);
}

CORE_API(int)
CORE_CreateObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_ID                 *handles, 
    size_t                          count
)
{
    int       res = -1;
    AcquireSRWLockExclusive(&table->RWLock);
    {
        uint32_t  type = table->ObjectType;
        uint32_t  nmax = table->TableCapacity-1;
        uint32_t  ncur = table->ObjectCount;
        uint32_t avail = nmax - ncur;
        uint32_t i, id;
        if (avail >= count)
        {
            for (i = 0; i < count; ++i)
            {   /* pop an item from the free list */
                CORE__DATA_INDEX *index = table->Index[table->FreelistHead];
                /* build two ids - the dense ID and the sparse ID.
                 * the dense ID is stored in the sparse index table, and consists of generation+dense array index.
                 * the sparse ID is the type + generation + sparse table index, and is used as the external object ID.
                 */
                table->FreelistHead  = index->Next;
                id =(index->ObjectId + CORE_DATA_HANDLE_NEW_OBJECT_ADD + ncur) & CORE_DATA_HANDLE_OBJECTID_MASK;
                handles[i] = CORE_MakeHandle(type, id);
                table->Store[ncur++] = id;
                index->ObjectId = id;
            }
            table->ObjectCount = ncur;
            res = 0;
        }
    }
    ReleaseSRWLockExclusive(&table->RWLock);
    return res;
}

CORE_API(void)
CORE_DeleteObjects
(
    struct _CORE_DATA_HANDLE_TABLE *table, 
    CORE_DATA_HANDLE             *handles, 
    size_t                          count
)
{
    AcquireSRWLockExclusive(&table->RWLock);
    {
        uint32_t   ncur = table->ObjectCount;
        uint32_t i, idx;
        for (i = 0; i < count; ++i)
        {   /* idx is the index, in table->Index, of the item to delete */
            idx = handles[i] & CORE_DATA_HANDLE_INDEX_MASK;
        }
    }
    ReleaseSRWLockExclusive(&table->RWLock);
}

#endif /* CORE_DATA_IMPLEMENTATION */

