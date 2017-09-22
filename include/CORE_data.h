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

/* #define CORE_STATIC to make all function declarations and definitions static.     */
/* This is useful if the library needs to be included multiple times in the project. */
#ifdef  CORE_STATIC
#define CORE_API(_rt)                     static _rt
#else
#define CORE_API(_rt)                     extern _rt
#endif

/* @summary Define various constants associated with the system, specifically 
 * dealing with the layout of an object ID. An object identifier looks like:
 * 31......|.....|...................0 bit index
 *  TTTTTTT|GGGGG|IIIIIIIIIIIIIIIIIIII bit usage
 * Where T bits correspond to the object type identifier, 
 * Where G bits correspond to the handle generation, and 
 * Where I bits correspond to the object STATE index in the ID table.
 * In the current system, you have 128 possible type values, and the same STATE 
 * array slot can be used 32 times before duplicate handles start appearing.
 * The system supports up to 2^20 (=1,048,576) objects of a given type.
 * If these values do not work well for your application, you can adjust them 
 * by #define'ing values that do work well for CORE_DATA_ID_INDEX_BITS, 
 * CORE_DATA_ID_GENERATION_BITS and CORE_DATA_ID_TYPE_BITS prior to including 
 * this file. The various masks and shifts will adjust themselves automatically.
 */
#ifndef CORE_DATA_ID_INDEX_BITS
#define CORE_DATA_ID_INDEX_BITS                20
#endif

#ifndef CORE_DATA_ID_GENERATION_BITS
#define CORE_DATA_ID_GENERATION_BITS           5
#endif

#ifndef CORE_DATA_ID_TYPE_BITS
#define CORE_DATA_ID_TYPE_BITS                 7
#endif

#ifndef CORE_DATA_ID_CONSTANTS
#define CORE_DATA_ID_CONSTANTS
#define CORE_DATA_MIN_TYPEID                   0
#define CORE_DATA_MIN_OBJECT_COUNT             1
#define CORE_DATA_ID_INDEX_SHIFT               0
#define CORE_DATA_ID_INDEX_MASK                ((1UL << CORE_DATA_ID_INDEX_BITS) - 1)
#define CORE_DATA_ID_INDEX_MASK_PACKED         (CORE_DATA_ID_INDEX_MASK << CORE_DATA_ID_INDEX_SHIFT)
#define CORE_DATA_ID_GENERATION_SHIFT          (CORE_DATA_ID_INDEX_SHIFT + CORE_DATA_ID_INDEX_BITS)
#define CORE_DATA_ID_GENERATION_ADD_PACKED     ((1UL << CORE_DATA_ID_GENERATION_SHIFT))
#define CORE_DATA_ID_GENERATION_MASK           ((1UL << CORE_DATA_ID_GENERATION_BITS) - 1)
#define CORE_DATA_ID_GENERATION_MASK_PACKED    (CORE_DATA_ID_GENERATION_MASK << CORE_DATA_ID_GENERATION_SHIFT)
#define CORE_DATA_ID_TYPE_SHIFT                (CORE_DATA_ID_GENERATION_SHIFT + CORE_DATA_ID_GENERATION_BITS)
#define CORE_DATA_ID_TYPE_MASK                 ((1UL << CORE_DATA_ID_TYPE_BITS) - 1)
#define CORE_DATA_ID_TYPE_MASK_PACKED          (CORE_DATA_ID_TYPE_MASK << CORE_DATA_ID_TYPE_SHIFT)
#define CORE_DATA_MAX_OBJECT_COUNT             ((1UL << CORE_DATA_ID_INDEX_BITS))
#define CORE_DATA_MAX_TYPEID_COUNT             ((1UL << CORE_DATA_ID_TYPE_BITS ))
#define CORE_DATA_MAX_TYPEID                   ((CORE_DATA_MIN_TYPEID + CORE_DATA_MAX_TYPEID_COUNT) - 1)
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

/* @summary Extract the object type identifier from an object handle.
 * @param _objid The CORE_DATA_OBJECT_ID to examine.
 * @return The object type identifier for the object.
 */
#ifndef CORE_GetObjectType
#define CORE_GetObjectType(_objid)                                             \
    (((_objid) & CORE_DATA_ID_TYPE_MASK_PACKED) >> CORE_DATA_ID_TYPE_SHIFT)
#endif

/* Forward-declare types exported by the library */
struct _CORE_DATA_OBJECT_ID_TABLE;       /* O(n) space, O(1) time management of object identifiers    */
struct _CORE_DATA_OBJECT_ID_TABLE_INIT;
struct _CORE_DATA_INDEX4;                /* O(n) space, O(1) time lookup of object ID/4 byte ID => 4 byte value */
struct _CORE_DATA_INDEX8;                /* O(n) space, O(1) time lookup of object ID/4 byte ID => 8 byte value */
struct _CORE_DATA_INDEX;                 /* O(n) space, O(1) time lookup of object ID/4 byte ID => N byte value */
struct _CORE_DATA_INDEX_INIT;
struct _CORE_DATA_STRING_TABLE;          /* O(1) time lookup of string attributes, storage of unique string data */
struct _CORE_DATA_STRING_TABLE_INIT;

/* @summary Object identifiers are opaque 32-bit integers.
 */
typedef uint32_t CORE_DATA_OBJECT_ID;

/* @summary Define the data associated with a table of object identifiers. 
 * The table maintains two arrays, State, which is sparse, and Dense, which is dense.
 * The low CORE_DATA_ID_INDEX_BITS bits in a State value store the index of the object ID in Dense.
 * The next CORE_DATA_ID_GENERATION_BITS in a State value store the slot generation, used to detect expired handles.
 * The high bit of each 32-bit value in State indicates whether the slot is in use - that is, whether the object is alive.
 * The low CORE_DATA_ID_INDEX_BITS bits in a Dense or CORE_DATA_OBJECT_ID value specify the index of the corresponding slot in the State array.
 * The table can generate a new object ID as an O(1) operation.
 * The table can delete an existing object ID as an O(1) operation.
 * The table can check whether an object ID represents a live object as an O(1) operation.
 * The set of object IDs for live objects are the first [0, ObjectCount) items in the Dense array.
 * The object ID table requires N * 8 bytes of storage, where N is the maximum number of live objects (TableCapacity).
 * A table with TableCapacity = CORE_DATA_MAX_OBJECT_COUNT therefore requires approximately 8MB of storage.
 */
typedef struct _CORE_DATA_OBJECT_ID_TABLE {
    SRWLOCK               RWLock;                 /* The reader-writer lock protecting the table. */
    uint32_t             *State;                  /* The sparse object state array, which points into the dense object ID array. */
    CORE_DATA_OBJECT_ID  *Dense;                  /* The dense object ID array, which stores the object ID handles for all live objects. */
    uint32_t              ObjectType;             /* The object type identifier for the objects managed by this table. */
    uint32_t              ObjectCount;            /* The number of live objects in the table. Dense indices [0, ObjectCount) are valid. */
    uint32_t              TableCapacity;          /* The maximum number of objects that can be live at any one time. */
    uint32_t              Reserved;               /* Reserved for future use. Set to 0. */
    void                 *MemoryStart;            /* The address of the start of the memory block reserved for table data. */
    uint64_t              MemorySize;             /* The number of bytes in the memory block reserved for table data. */
    uint32_t              Pad0;                   /* Reserved for future use. Set to 0. */
    uint32_t              Pad1;                   /* Reserved for future use. Set to 0. */
} CORE_DATA_OBJECT_ID_TABLE;

/* @summary Define the data used to configure an object ID table.
 */
typedef struct _CORE_DATA_OBJECT_ID_TABLE_INIT {
    uint32_t              TypeId;                 /* The object type identifier for all objects returned by the handle table. */
    uint32_t              MaxObjects;             /* The maximum number of live objects of the given type at any one time. */
    void                 *MemoryStart;            /* Pointer to the start of the memory block used for the table data. */
    uint64_t              MemorySize;             /* The number of bytes in the memory block used for the table data. */
} CORE_DATA_OBJECT_ID_TABLE_INIT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* @summary Compute the minimum number of bytes required to initialize an object ID table with the given capacity.
 * @param max_objects The maximum number of live objects within the table at any given time.
 * @return The minimum number of bytes required for the memory block used to store the table data.
 */
CORE_API(size_t)
CORE_QueryObjectIdTableMemorySize
(
    uint32_t max_objects
);

/* @summary Initialize an object ID table to empty using memory supplied by the caller.
 * This operation is O(n) where n is the maximum number of live objects in the table.
 * @param table On return, this location points to the newly initialized ID table.
 * @param init Data used to configure the object ID table.
 * @return Zero if the table is successfully created, or -1 if an error occurred.
 */
CORE_API(int)
CORE_InitObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE     *table, 
    CORE_DATA_OBJECT_ID_TABLE_INIT *init
);

/* @summary Reset an object ID table to empty, invalidating all live objects.
 * This operation is O(n) where n is the maximum number of live objects in the table.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The object ID table to reset.
 */
CORE_API(void)
CORE_ResetObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
);

/* @summary Print the contents of an object ID table to stdout.
 * State values are formatted as LL|GG|IIIIIII (L=Live 0/1, G=Generation, I=Dense Index).
 * Dense values are formatted as TT|GG|IIIIIII (T=Type ID, G=Generation, I=State Index).
 * The width of the Index values is determined by the table capacity.
 */
CORE_API(void)
CORE_PrintObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
);

/* @summary Perform a consistency check on the data within an object ID table.
 * Verify that the generation in State matches the generation in Dense.
 * Verify that the index in State points to the correct Dense slot.
 * Verify that the live bit in State is set for any live items.
 * Count the unused items in State, and verify that a corresponding number of items are available in the free list.
 * Assert if any of these checks fail in a debug build.
 * @param table The object ID table to verify.
 * @return Non-zero if the table state appears to be valid, or zero if one of the consistency checks failed.
 */
CORE_API(int)
CORE_VerifyObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
);

/* @summary Allocate identifiers for one or more objects.
 * The operation fails if the table cannot allocate the requested number of object IDs.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The table from which the object identifiers will be allocated.
 * @param handles The array of count handles to populate with allocated object identifiers.
 * @param count The number of object identifiers to allocate.
 * @return Zero if count object identifiers were successfully allocated, or -1 if an error occurred.
 */
CORE_API(int)
CORE_CreateObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t                     count
);

/* @summary Delete identifiers for one or more objects.
 * This operation acquires an exclusive lock on the table - no read or write operations can proceed concurrently.
 * @param table The table from which the object identifiers were allocated.
 * @param handles The array of count handles specifying the object identifiers to delete.
 * @param count The number of object identifiers to delete.
 */
CORE_API(void)
CORE_DeleteObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t                     count
);

/* @summary Query an object ID table for a set of identifiers of live objects.
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
CORE_QueryLiveObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t               max_handles, 
    size_t              *num_handles, 
    size_t               start_index, 
    size_t            *num_remaining
);

/* @summary Given a set of object IDs, filter the set into two lists - handles of objects that are still alive, and handles of objects that are dead.
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
CORE_FilterObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE    *table, 
    CORE_DATA_OBJECT_ID *result_handles,
    CORE_DATA_OBJECT_ID  **live_objects,
    size_t                  *live_count,
    CORE_DATA_OBJECT_ID  **dead_objects, 
    size_t                  *dead_count, 
    CORE_DATA_OBJECT_ID  *check_handles, 
    size_t                  check_count
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
    uint8_t          *BaseAddress;                /* The base address of the memory range. */
    size_t            MemorySize;                 /* The size of the memory block, in bytes. */
    size_t            NextOffset;                 /* The offset of the next available address. */
} CORE__DATA_ARENA;

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
CORE_QueryObjectIdTableMemorySize
(
    uint32_t max_objects
)
{
    size_t required_size = 0;
    required_size += CORE__DataAllocationSizeArray(uint32_t           , max_objects+1); /* State */
    required_size += CORE__DataAllocationSizeArray(CORE_DATA_OBJECT_ID, max_objects+1); /* Dense */
    return required_size;
}

CORE_API(int)
CORE_InitObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE     *table, 
    CORE_DATA_OBJECT_ID_TABLE_INIT *init
)
{
    CORE__DATA_ARENA  arena;
    size_t    required_size = 0;
    uint32_t           i, n;

    if (init->TypeId < CORE_DATA_MIN_TYPEID || 
        init->TypeId > CORE_DATA_MAX_TYPEID)
    {   /* the object type identifier is invalid */
        assert(init->TypeId >= CORE_DATA_MIN_TYPEID);
        assert(init->TypeId <= CORE_DATA_MAX_TYPEID);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }
    if (init->MaxObjects < CORE_DATA_MIN_OBJECT_COUNT || 
        init->MaxObjects > CORE_DATA_MAX_OBJECT_COUNT)
    {   /* the table maximum object count is invalid */
        assert(init->MaxObjects >= CORE_DATA_MIN_OBJECT_COUNT);
        assert(init->MaxObjects <= CORE_DATA_MAX_OBJECT_COUNT);
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

    required_size = CORE_QueryObjectIdTableMemorySize(init->MaxObjects);
    if (init->MemorySize < required_size)
    {   /* the caller must supply sufficient memory for the table */
        assert(init->MemorySize >= required_size);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* zero-initialize the entire memory block and allocate the base structure */
    ZeroMemory(init->MemoryStart, (size_t) init->MemorySize);
    CORE__DataInitMemoryArena(&arena, init->MemoryStart, (size_t) init->MemorySize);
    InitializeSRWLock(&table->RWLock);
    table->State        = CORE__DataMemoryArenaAllocateArray(&arena, uint32_t           , init->MaxObjects);
    table->Dense        = CORE__DataMemoryArenaAllocateArray(&arena, CORE_DATA_OBJECT_ID, init->MaxObjects);
    table->ObjectType   = init->TypeId;
    table->ObjectCount  = 0;
    table->TableCapacity= init->MaxObjects;
    table->Reserved     = 0;
    table->MemoryStart  = init->MemoryStart;
    table->MemorySize   = init->MemorySize;
    table->Pad0         = 0;
    table->Pad1         = 0;
    /* initialize the free list in the Dense vector */
    for (i = 0, n = init->MaxObjects; i < n; ++i)
    {
        table->Dense[i] = i;
    }
    return 0;
}

CORE_API(void)
CORE_ResetObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
)
{
    AcquireSRWLockExclusive(&table->RWLock);
    {
        uint32_t i, n;
        /* zero out the State vector, clearing the live bit and generation */
        ZeroMemory(table->State, table->TableCapacity * sizeof(uint32_t));
        /* initialize the free list in the Dense vector */
        for (i = 0, n = table->TableCapacity; i < n; ++i)
        {
            table->Dense[i] = i;
        }
        table->ObjectCount = 0;
    }
    ReleaseSRWLockExclusive(&table->RWLock);
}

CORE_API(void)
CORE_PrintObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
)
{
    uint32_t i, n;
    int max_width;
    if      (table->TableCapacity >= 1000000) max_width = 7;
    else if (table->TableCapacity >= 100000 ) max_width = 6;
    else if (table->TableCapacity >= 10000  ) max_width = 5;
    else if (table->TableCapacity >= 1000   ) max_width = 4;
    else if (table->TableCapacity >= 100    ) max_width = 3;
    else if (table->TableCapacity >= 10     ) max_width = 2;
    else max_width = 1;
    printf("OID Table: %u/%u\n", table->ObjectCount, table->TableCapacity);
    printf("State: [");
    for (i = 0, n = table->TableCapacity; i < n; ++i)
    {
        uint32_t state_value = table->State[i];
        uint32_t        live =(state_value & 0x80000000UL) >> 31;
        uint32_t  generation =(state_value & CORE_DATA_ID_GENERATION_MASK_PACKED) >> CORE_DATA_ID_GENERATION_SHIFT;
        uint32_t dense_index =(state_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
        printf("%2u|%2u|%*u", live, generation, max_width, dense_index);
        if (i != (n-1)) printf(", ");
    }
    printf("]\n");
    printf("Dense: [");
    for (i = 0, n = table->TableCapacity; i < n; ++i)
    {
        uint32_t dense_value = table->Dense[i];
        uint32_t        type =(dense_value & CORE_DATA_ID_TYPE_MASK_PACKED) >> CORE_DATA_ID_TYPE_SHIFT;
        uint32_t  generation =(dense_value & CORE_DATA_ID_GENERATION_MASK_PACKED) >> CORE_DATA_ID_GENERATION_SHIFT;
        uint32_t state_index =(dense_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
        printf("%2u|%2u|%*u", type, generation, max_width, state_index);
        if (i != (n-1)) printf(", ");
    }
    printf("]\n");
    printf("\n");
}

CORE_API(int)
CORE_VerifyObjectIdTable
(
    CORE_DATA_OBJECT_ID_TABLE *table
)
{
    uint32_t         *s = table->State;
    uint32_t         *d = table->Dense;
    uint32_t live_count = 0;
    uint32_t dead_count = 0;
    uint32_t       i, n;
    /* count the number of values in state with their live bit set/unset */
    for (i = 0, n = table->TableCapacity; i < n; ++i)
    {
        if (s[i] & 0x80000000UL) live_count++;
        else dead_count++;
    }
    if (dead_count + live_count != table->TableCapacity)
    {
        assert((dead_count+live_count) == table->TableCapacity);
        return 0;
    }
    if (live_count != table->ObjectCount)
    {   /* this tells you that a state bit was not cleared on delete */
        assert(live_count == table->ObjectCount);
        return 0;
    }
    /* make sure that all of the live IDs in dense point back to state, and vice-versa */
    for (i = 0, n = table->ObjectCount; i < n; ++i)
    {
        uint32_t dense_value = d[i];
        uint32_t state_index =(dense_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
        uint32_t state_value = s[state_index];
        uint32_t dense_gener =(dense_value & CORE_DATA_ID_GENERATION_MASK_PACKED) >> CORE_DATA_ID_GENERATION_SHIFT;
        uint32_t state_gener =(state_value & CORE_DATA_ID_GENERATION_MASK_PACKED) >> CORE_DATA_ID_GENERATION_SHIFT;
        uint32_t dense_index =(state_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;

        if ((state_value & 0x80000000UL) == 0)
        {   /* Dense[i] points back to a dead State slot */
            assert((state_value & 0x80000000UL) != 0 && "Live bit not set on state slot");
            return 0;
        }
        if (state_gener != dense_gener)
        {   /* the generation values don't match */
            assert(state_gener == dense_gener && "State and Dense generations do not match");
            return 0;
        }
        if (dense_index != i)
        {   /* the State value that Dense[i] points to doesn't point back at Dense[i] */
            assert(dense_index == i && "State does not point back at Dense");
            return 0;
        }
    }
    /* make sure that the free list points to actually free items */
    for (i = table->ObjectCount, n = table->TableCapacity; i < n; ++i)
    {
        uint32_t dense_value = d[i];
        uint32_t state_index =(dense_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
        uint32_t state_value = s[state_index];

        if (state_value & 0x80000000UL)
        {   /* the live bit should NOT be set */
            assert((state_value & 0x80000000UL) == 0 && "Live bit set on free item");
            return 0;
        }
        if (state_value & CORE_DATA_ID_INDEX_MASK_PACKED)
        {   /* the dense index should NOT be set on a free state value */
            assert((state_value & CORE_DATA_ID_INDEX_MASK_PACKED) == 0 && "Dense index set on free state value");
            return 0;
        }
        if (dense_value & CORE_DATA_ID_GENERATION_MASK_PACKED)
        {   /* the generation should NOT be present on a free list value */
            assert((dense_value & CORE_DATA_ID_GENERATION_MASK_PACKED) == 0 && "Generation set on free dense value");
            return 0;
        }
        if (dense_value & CORE_DATA_ID_TYPE_MASK_PACKED)
        {   /* the type should NOT be set on a free list value */
            assert((dense_value & CORE_DATA_ID_TYPE_MASK_PACKED) == 0 && "Type set on free dense value");
            return 0;
        }
    }
    return 1;
}

CORE_API(int)
CORE_CreateObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t                     count
)
{
    uint32_t            *s = table->State;
    CORE_DATA_OBJECT_ID *d = table->Dense;
    int                res = -1;
    AcquireSRWLockExclusive(&table->RWLock);
    {   /* only proceed with ID allocation if enough IDs are available */
        if (table->ObjectCount + count <= table->TableCapacity)
        {
            uint32_t       object_count = table->ObjectCount;
            uint32_t        object_type =(table->ObjectType << CORE_DATA_ID_TYPE_SHIFT);
            uint32_t       dense_index;
            uint32_t       state_index;
            uint32_t       state_value;
            uint32_t        generation;
            CORE_DATA_OBJECT_ID new_id;
            size_t                   i;
            for (i = 0; i < count; ++i)
            {
                dense_index    = object_count;
                state_index    = d[object_count++];
                state_value    = s[state_index];
                generation     = state_value & CORE_DATA_ID_GENERATION_MASK_PACKED;
                new_id         = object_type | generation | state_index;
                s[state_index] = 0x80000000UL| generation | dense_index;
                d[dense_index] = new_id;
                handles[i]     = new_id;
            }
            table->ObjectCount = object_count;
            res = 0;
        }
    }
    ReleaseSRWLockExclusive(&table->RWLock);
    return res;
}

CORE_API(void)
CORE_DeleteObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t                     count
)
{
    uint32_t            *s = table->State;
    CORE_DATA_OBJECT_ID *d = table->Dense;
    AcquireSRWLockExclusive(&table->RWLock);
    {
        uint32_t  object_count = table->ObjectCount;
        uint32_t           put = table->ObjectCount - 1;
        uint32_t        id_gen;
        uint32_t     state_gen;
        uint32_t    state_live;
        uint32_t   state_index;
        uint32_t   state_value;
        uint32_t   dense_index;
        uint32_t   dense_value;
        uint32_t   moved_index;
        uint32_t      moved_id;
        CORE_DATA_OBJECT_ID id;
        size_t               i;
        for (i = 0; i < count; ++i)
        {
            id          = handles[i];
            id_gen      = (id & CORE_DATA_ID_GENERATION_MASK_PACKED);
            state_index = (id & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
            state_value = s[state_index];
            state_live  = state_value & 0x80000000UL;
            state_gen   = state_value & CORE_DATA_ID_GENERATION_MASK_PACKED;
            dense_index =(state_value & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
            if (state_live && state_gen == id_gen)
            {   /* clear the live bit and dense index and increment the generation */
                s[state_index] = (state_value + CORE_DATA_ID_GENERATION_ADD_PACKED) & CORE_DATA_ID_GENERATION_MASK_PACKED;
                if (dense_index != put)
                {   /* swap the dead item into the last live slot in Dense */
                    moved_id       = d[put];
                    moved_index    =(moved_id & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
                    d[dense_index] = moved_id;
                    s[moved_index] = 0x80000000UL | (moved_id & CORE_DATA_ID_GENERATION_MASK_PACKED) | dense_index;
                }
                /* return the state_index to the free list */
                d[put] = state_index;
                object_count = put--;
            }
        }
        table->ObjectCount = object_count;
    }
    ReleaseSRWLockExclusive(&table->RWLock);
}

CORE_API(int)
CORE_QueryLiveObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE *table, 
    CORE_DATA_OBJECT_ID     *handles, 
    size_t               max_handles, 
    size_t              *num_handles, 
    size_t               start_index, 
    size_t            *num_remaining
)
{
    uint32_t const   *d = table->Dense;
    size_t            i, n, m;
    AcquireSRWLockShared(&table->RWLock);
    {
        for (i = start_index, 
             n = table->ObjectCount, 
             m = 0; 
             i < n && m < max_handles; 
           ++i,++m)
        {
            handles[m] = d[i];
        }
    }
    ReleaseSRWLockShared(&table->RWLock);
    if (num_handles) *num_handles = m;
    if (num_remaining) *num_remaining = (n - i);
    return (m > 0);
}

CORE_API(void)
CORE_FilterObjectIds
(
    CORE_DATA_OBJECT_ID_TABLE    *table, 
    CORE_DATA_OBJECT_ID *result_handles,
    CORE_DATA_OBJECT_ID  **live_objects,
    size_t                  *live_count,
    CORE_DATA_OBJECT_ID  **dead_objects, 
    size_t                  *dead_count, 
    CORE_DATA_OBJECT_ID  *check_handles, 
    size_t                  check_count
)
{
    if (check_count > 0)
    {
        uint32_t               *s = table->State;
        CORE_DATA_OBJECT_ID *live = result_handles;
        CORE_DATA_OBJECT_ID *dead = result_handles + (check_count - 1);
        size_t           num_live = 0;
        size_t           num_dead = 0;
        size_t                  i;
        uint32_t       handle_gen;
        uint32_t        state_gen;
        uint32_t        state_idx;
        AcquireSRWLockShared(&table->RWLock);
        {   /* FUTURE: this could easily be unrolled and performed using SIMD */
            for (i = 0; i < check_count; ++i)
            {   /* retrieve the state value for the current handle */
                handle_gen=(check_handles[i] & CORE_DATA_ID_GENERATION_MASK_PACKED);
                state_idx =(check_handles[i] & CORE_DATA_ID_INDEX_MASK_PACKED) >> CORE_DATA_ID_INDEX_SHIFT;
                state_gen = s[state_idx] & CORE_DATA_ID_GENERATION_MASK_PACKED;
                if ((s[state_idx] & 0x80000000UL) && state_gen == handle_gen)
                {   /* this object ID is still valid */
                    live[num_live++] = check_handles[i];
                }
                else
                {   /* this object ID is no longer valid */
                   *dead-- = check_handles[i]; ++num_dead;
                }
            }
        }
        ReleaseSRWLockShared(&table->RWLock);
       *live_objects = live;
       *dead_objects = dead;
       *live_count   = num_live;
       *dead_count   = num_dead;
    }
}

#endif /* CORE_DATA_IMPLEMENTATION */

