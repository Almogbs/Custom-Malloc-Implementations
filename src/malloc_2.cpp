/**
 * @file malloc_2.cpp
 * @author Art Vandelay
 * @version 1
 * @date 2022-01-13
 * @copyright Copyright (c) 2022
 */



// includes
#include <unistd.h>
#include <assert.h>
#include <string.h>




// constants
#define MAX_MALLOC_2_SIZE 100000000
#define SBRK_FAIL -1
#define NDEBUG



/**
 * @struct: malloc_metadata_t
 * @brief:  Struct to hold the Metadata of the Allocations.
 * 
 * @members:
 *     - size_t size:           numbers of bytes in the allocate (include metadata).
 *     - bool is_free:          true if block was free'd before.
 *     - MallocMetadata* next:  pointer to next alloc block (nullptr if last).
 *     - MallocMetadata* prev:  pointer to previous alloc block (nullptr if first).
 */
struct malloc_metadata_t {
    size_t size;
    bool is_free;
    malloc_metadata_t* next;
    malloc_metadata_t* prev;
};

typedef struct malloc_metadata_t MallocMetadata;


/**
 * @macro: MMIN(A,B)
 * @brief: returns the minimum of A and B.
 */
#define MMIN(A,B) (((A) < (B)) ? (A) : (B))



/**
 * @macro: GET_METADATA_FROM_PTR(alloc_ptr)
 * @brief: get the metadata of the alloc pointer.
 */
#define GET_METADATA_FROM_PTR(alloc_ptr)    ((MallocMetadata*)((intptr_t)alloc_ptr - sizeof(malloc_metadata_t)))


/**
 * @macro: GET_METADATA_SIZE(metadata)
 * @brief: get the block size from the metadata.
 */
#define GET_METADATA_SIZE(metadata)         (((MallocMetadata*)metadata)->size)


/**
 * @macro: GET_METADATA_IS_FREE(metadata)
 * @brief: get the block is_Free from the metadata.
 */
#define GET_METADATA_IS_FREE(metadata)      (((MallocMetadata*)metadata)->is_free)


/**
 * @macro: GET_METADATA_NEXT(metadata)
 * @brief: get the next block from the metadata.
 */
#define GET_METADATA_NEXT(metadata)         (((MallocMetadata*)metadata)->next)


/**
 * @macro: GET_METADATA_PREV(metadata)  
 * @brief: get the prev block from the metadata.
 */
#define GET_METADATA_PREV(metadata)         (((MallocMetadata*)metadata)->prev)


/**
 * @macro: GET_PTR_FROM_METADATA(metadata)
 * @brief: get the actual pointer from the metadata.
 */
#define GET_PTR_FROM_METADATA(metadata)     ((void*)((intptr_t)metadata + sizeof(malloc_metadata_t)))


/**
 * @macro: METADATA_FOR_EACH(block, mt_head)
 * @brief: iterate over the list's block, from mt_head until nullptr(end).
 */
#define METADATA_FOR_EACH(block, mt_head)   for(MallocMetadata* block = mt_head; block != nullptr; block = block->next)


/**
 * @macro: INIT_METADATA(metadata, size, is_free, next, prev)
 * @brief: initialize the metadata's members.
 */
#define INIT_METADATA(metadata, _size, _is_free, _next, _prev) \
        MallocMetadata* tm = (MallocMetadata*)metadata;        \
        tm->size    = _size;                                   \
        tm->is_free = _is_free;                                \
        tm->next    = _next;                                   \
        tm->prev    = _prev;                                  



// global head of the alloction list
static MallocMetadata* metadata_head = nullptr;



/**
 * @function:   static void* get_free_metadata_block(size_t size)
 * @brief:      search for suitable block from the alloc list (when block.size >= size).
 * 
 * @arguments:
 *     - size_t size: # of bytes to find in blocks.
 * 
 * @returns:
 *     - Success: a pointer to the matching block.
 *
 *     - Failure:
 *          If no suitable block was found, returns nullptr.
 */
static MallocMetadata* get_free_metadata_block(size_t size)
{
    METADATA_FOR_EACH(block, metadata_head)
    {
        if (block->is_free && block->size >= size)
            return block;
    }
    return nullptr;
}



/**
 * @function:   static MallocMetadata* get_last_metadata_block()
 * @brief:      search for the last block in alloc list.
 * 
 * @returns:
 *     - Success: a pointer to the last block.
 *
 *     - Failure:
 *          Never Fail (motivation loll)
 */

static MallocMetadata* get_last_metadata_block()
{
    if (metadata_head  == nullptr) return nullptr;
    METADATA_FOR_EACH(block, metadata_head)
    {
        if (block->next == nullptr)
        {
            return block;
        }
            
    }
    return nullptr;
}

/**
 * @function:   void* smalloc(size_t size)
 * @brief:  Searches for a free block with up to ???size??? bytes or allocates (sbrk())
 *          one if none are found.
 * 
 * @arguments:
 *     - size_t size: # of bytes to allocate.
 * 
 * @returns:
 *     - Success: a pointer to the first allocated byte within the allocated block.
 *                (excluding the meta-data)
 *
 *     - Failure:
 *          If ???size??? is 0 returns nullptr.
 *          If ???size??? is more than 10^8 , return nullptr. 
 *          If sbrk fails, return nullptr.
 */
void* smalloc(size_t size)
{
    if (size > MAX_MALLOC_2_SIZE || size == 0)
    {
        return nullptr;
    }

    // if this is the first allocation
    if (metadata_head == nullptr)
    {
        void* ret = sbrk(size + sizeof(malloc_metadata_t));
        if ((intptr_t)ret == SBRK_FAIL)
        {
            return nullptr;
        }
        
        INIT_METADATA(ret, size, false, nullptr, nullptr)
        metadata_head = (MallocMetadata*)ret;
        return GET_PTR_FROM_METADATA(ret);
    }
    
    // try to use free'd block
    MallocMetadata* freed = get_free_metadata_block(size);
    if (freed != nullptr)
    {
        freed->is_free = false;
        return GET_PTR_FROM_METADATA(freed);
    }

    // no matching block -> create new one
    void* ret = sbrk(size + sizeof(malloc_metadata_t));
    if ((intptr_t)ret == SBRK_FAIL)
    {
        return nullptr;
    }
    MallocMetadata* last = get_last_metadata_block();
    last->next = (MallocMetadata*)ret;
    INIT_METADATA(ret, size, false, nullptr, last);
    return GET_PTR_FROM_METADATA(ret);
}


/**
 * @function:   void* scalloc(size_t num, size_t size)
 * @brief:  Searches for a free block of up to ???num??? elements, each ???size??? bytes that 
 *          are all set to 0 or allocates if none are found.
 * 
 * @arguments:
 *     - size_t num: # of elements to allocate.
 *     - size_t size: size (bytes) of each element.
 * 
 * @returns:
 *     - Success: - returns pointer to the first byte in the allocated block.
 *
 *     - Failure:
 *          If ???size??? is 0 returns nullptr.
 *          If ???size??? is more than 10^8 , return nullptr. 
 *          If sbrk fails, return nullptr.
 */
void* scalloc(size_t num, size_t size)
{
    void* res = smalloc(num * size);
    if (res != nullptr)
    {
        memset(res, 0, num * size);
        return res;
    }
    return nullptr;
}



/**
 * @function:   void sfree(void* p)
 * @brief:  Releases the usage of the block that starts with the pointer ???p???. 
 * 
 * @arguments:
 *     - void* p: pointer to allocated block to free.
 *                If ???p??? is nullptr or already released, simply returns.
 *                Presume that all pointers ???p??? truly points to the beginning of an allocated block.
 * 
 * @returns:
 *     None
 */
void sfree(void* p)
{
    if (p == nullptr)
    {
        return;
    }
    MallocMetadata* to_free =  GET_METADATA_FROM_PTR(p);
    to_free->is_free = true;
    return;
}



/**
 * @function:   void* srealloc(void* oldp, size_t size)
 * @brief:  If ???size??? is smaller than the current block???s size, reuses the same block.
 *          Otherwise, finds/allocates ???size??? bytes for a new space, copies content of oldp
 *          into the new allocated space and frees the oldp.
 * 
 * @arguments:
 *     - void* oldp: pointer to block-to-copy.
 *     - size_t size: # of bytes to allocate.
 * 
 * @returns:
 *     - Success:
 *          Returns pointer to the first byte in the (newly) allocated space.
 *          If ???oldp??? is nullptr, allocates space for ???size??? bytes and returns a pointer to it.
 *
 *     - Failure:
 *          If ???size??? is 0 returns nullptr.
 *          If ???size??? is more than 10^8 , return nullptr. 
 *          If sbrk fails, return nullptr.
 */
void* srealloc(void* oldp, size_t size)
{
    if (size > MAX_MALLOC_2_SIZE || size == 0)
    {
        return nullptr;
    }
    assert(size > 0);

    if(oldp != nullptr)
    {
        MallocMetadata* old_ptr = GET_METADATA_FROM_PTR(oldp);
        if (old_ptr->size >= size)
        {
            old_ptr->is_free = false;
            return GET_PTR_FROM_METADATA(old_ptr);
        }
    }

    void* res = smalloc(size);
    if (res != nullptr)
    {
        if(oldp != nullptr)
        {
            MallocMetadata* old_ptr = GET_METADATA_FROM_PTR(oldp);
            memcpy(res, oldp, MMIN(old_ptr->size, size));
            sfree(oldp);
        }
        return res;
    }

    return nullptr;
}



/**
 * @function:   size_t _num_free_blocks()
 *
 * @returns:
 *     Returns the number of allocated blocks in the heap that are currently free.
 */
size_t _num_free_blocks()
{
    size_t result = 0;
    METADATA_FOR_EACH(block, metadata_head)
    {
        if (block->is_free) result++;
    }
    return result;
}



/**
 * @function:  size_t _num_free_bytes()
 *
 * @returns:
 *     Returns the number of bytes in all allocated blocks in the heap that are 
 *     currently free, excluding the bytes used by the meta-data structs.
 */
size_t _num_free_bytes()
{
    size_t result = 0;
    METADATA_FOR_EACH(block, metadata_head)
    {
        if (block->is_free) 
            result += (block->size);
    }
    return result;
}


/**
 * @function:   size_t _num_allocated_blocks()
 *
 * @returns:
 *      Returns the overall (free and used) number of allocated blocks in the heap.
 */
size_t _num_allocated_blocks()
{
    size_t result = 0;
    METADATA_FOR_EACH(block, metadata_head)
    { 
        result++;
    }
    return result;
}



/**
 * @function:  size_t _num_allocated_bytes()
 *
 * @returns:
 *      Returns the overall number (free and used) of allocated bytes in the heap, 
 *      excluding the bytes used by the meta-data structs.
 */
size_t _num_allocated_bytes()
{
    size_t result = 0;
    METADATA_FOR_EACH(block, metadata_head)
    {
        result += (block->size);
    }
    return result;
}



/**
 * @function:   size_t _num_meta_data_bytes()
 *
 * @returns:
 *     Returns the overall number of meta-data bytes currently in the heap.
 */
size_t _num_meta_data_bytes()
{
    size_t result = 0;
    METADATA_FOR_EACH(block, metadata_head)
    {
        result += (sizeof(malloc_metadata_t));
    }
    return result;
}



/**
 * @function:   size_t _size_meta_data()
 *
 * @returns:
 *     Returns the number of bytes of a single meta-data structure in your system.
 */
size_t _size_meta_data()
{
    return sizeof(malloc_metadata_t);
}