/**
 * @file        malloc_4.cpp
 * @author      Art Vandelay
 * @version     1
 * @date        2022-01-13
 * @copyright   Copyright (c) 2022
 */



// includes
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>


size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();



// constants
//#define NDEBUG
#define ADDRESS_SIZE (sizeof(void*))
#define GET_SIZE_WITH_ALIGNMENT(address) ((address) + ((ADDRESS_SIZE - ((address) % ADDRESS_SIZE)) % ADDRESS_SIZE))
#define MAX_MALLOC_4_SIZE 100000000
#define SBRK_FAIL -1
#define BIN_SIZE 128
#define KB 1024
#define MIN_KB_BLOCK 128 * KB



/**
 * @struct: malloc_metadata_t
 * @brief:  Struct to hold the Metadata of the Allocations.
 * 
 * @members:
 *     - size_t size:               numbers of bytes in the allocate (include metadata).
 *     - bool is_free:              true if block was free'd before.
 *     - MallocMetadata* next:      pointer to next alloc block (nullptr if last).
 *     - MallocMetadata* prev:      pointer to previous alloc block (nullptr if first).
 *     - MallocMetadata* bin_next:  pointer to next alloc block in free bin entry (nullptr if last).
 *     - MallocMetadata* bin_prev:  pointer to previous alloc block in free bin entry (nullptr if first).
 */
struct malloc_metadata_t {
    size_t size;
    bool is_free;
    malloc_metadata_t* next;
    malloc_metadata_t* prev;
    malloc_metadata_t* bin_next;
    malloc_metadata_t* bin_prev;
};

typedef struct malloc_metadata_t MallocMetadata;


/**
 * @macro: INT_TO_KB(X)
 * @brief: returns X kb.
 */
#define INT_TO_KB(X) ((X)*KB)



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
#define INIT_METADATA(metadata, _size, _is_free, _next, _prev, _bin_next, _bin_prev)  \
        MallocMetadata* tm = (MallocMetadata*)metadata;          \
        tm->size     = _size;                                    \
        tm->is_free  = _is_free;                                 \
        tm->next     = _next;                                    \
        tm->prev     = _prev;                                    \
        tm->bin_next = _bin_next;                                \
        tm->bin_prev = _bin_prev;                                                                    


/**
 * @macro: GET_BIN_ENTRY(size)
 * @brief: gets the matching bin entry from its size.
 */
#define GET_BIN_ENTRY(size) ((size)/KB)


/**
 * @macro: IS_LARGE_ENOUGH(entire_block_size, needed_size)
 * @brief: returns true if entire_block_size large enough to fit needed_size.
 */
#define IS_LARGE_ENOUGH(entire_block_size, needed_size) \
        ((entire_block_size) >= (needed_size) + sizeof(malloc_metadata_t) + 128)


// global head of the alloction list from sbrk
static MallocMetadata* metadata_head = nullptr;

// global head of the alloction list from mmap
static MallocMetadata* mmap_metadata_head = nullptr;

// global bin of free blocks
static MallocMetadata* free_block_bin[BIN_SIZE] = {};



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
static MallocMetadata* get_last_metadata_block(MallocMetadata* list)
{
    if (!list)
    {
        return nullptr;
    }
    METADATA_FOR_EACH(block, list)
    {
        if (block->next == nullptr)
            return block;
    }
    // assert(true);
    return list;
}



/**
 * @function:   void remove_from_list(MallocMetadata* to_del, MallocMetadata** list)
 * @brief:      delete metadata from the list
 * 
 * @arguments:
 *     - MallocMetadata* to_del: block to delete
 *     - MallocMetadata** list: pointer to list to delete from
 */
static void remove_from_list(MallocMetadata* to_del, MallocMetadata** list)
{
    if (!to_del->next && !to_del->prev)
    {
        *list = nullptr;
    }
    else if (!to_del->prev)
    {
        *list = to_del->next;
        to_del->next->prev = nullptr;
    }
    else if (!to_del->next)
    {
        to_del->prev->next = nullptr;
    }
    else
    {
        to_del->next->prev = to_del->prev;
        to_del->prev->next = to_del->next;
    }
    to_del->next = nullptr;
    to_del->prev = nullptr;
    return;
}




/**
 * @function:   void remove_from_bin(MallocMetadata* to_del)
 * @brief:      delete metadata from the right bin
 * 
 * @arguments:
 *     - MallocMetadata* to_del: block to delete
 */
static void remove_from_bin(MallocMetadata* to_del)
{
    MallocMetadata** list = &free_block_bin[GET_BIN_ENTRY(to_del->size)];
    if (!to_del->bin_next && !to_del->bin_prev)
    {
        *list = nullptr;
    }
    else if (!to_del->bin_prev)
    {
        *list = to_del->bin_next;
        to_del->bin_next->bin_prev = nullptr;
    }
    else
    {
        to_del->bin_prev->bin_next = nullptr;
    }
    to_del->bin_next = nullptr;
    to_del->bin_prev = nullptr;
    return;
}



/**
 * @function:   insert_to_metadata_list(MallocMetadata* new_block, MallocMetadata** list)
 * @brief:      insert metadata into the list
 * 
 * @arguments:
 *     - MallocMetadata* new_block: block to insert
 *     - MallocMetadata** list: pointer to list to append to
 */
static void insert_to_metadata_list(MallocMetadata* new_block, MallocMetadata** list)
{
    // list is empty
    if ((*list) == nullptr)
    {
        new_block->prev = nullptr;
        new_block->next = nullptr;
        *list = new_block;
    }
    else
    {
        MallocMetadata* after = get_last_metadata_block(*list);
        after->next = new_block;
        new_block->prev = after;
        new_block->next = nullptr;
    }
}


/**
 * @function:   void insert_block_to_bin(MallocMetadata* new_block)
 * @brief:      insert metadata into its bin
 * 
 * @arguments:
 *     - MallocMetadata* new_block: block to insert
 */
static void insert_block_to_bin(MallocMetadata* new_block)
{
    // list is empty
    if (free_block_bin[GET_BIN_ENTRY(new_block->size)] == nullptr)
    {
        new_block->bin_prev = nullptr;
        new_block->bin_next = nullptr;
        free_block_bin[GET_BIN_ENTRY(new_block->size)] = new_block;
    }
    else
    {
        MallocMetadata* last;
        for(MallocMetadata* block = free_block_bin[GET_BIN_ENTRY(new_block->size)]; block != nullptr; block = block->bin_next)
        {
            last = block;
            if(block->size >= new_block->size)
            {
                // insert before block
                MallocMetadata* prev = block->bin_prev;
                if (prev == nullptr)
                {
                    block->bin_prev = new_block;
                    new_block->bin_next = block;
                    new_block->bin_prev = nullptr;
                    free_block_bin[GET_BIN_ENTRY(new_block->size)] = new_block;
                }
                else
                {
                    prev->bin_next = new_block;
                    new_block->bin_prev = prev;
                    new_block->bin_next = block;
                    block->bin_prev = new_block;
                }
                return;
            }
        }
        // insert new_block last
        new_block->bin_next = nullptr;
        new_block->bin_prev = last;
        last->bin_next = new_block;
        new_block->is_free = true;
    }
}



/**
 * @function:   void cut_block(MallocMetadata* block, size_t size)
 * @brief:      split the block into two according to the size
 * 
 * @arguments:
 *     - size_t size:           # of bytes to find in blocks.
 *     - MallocMetadata* block: block to split
 */
static void cut_block(MallocMetadata* block, size_t size, bool remove_bin = true)
{
    if (remove_bin)
    {
        remove_from_bin(block);
    }
    int new_block_size = block->size - size;
    MallocMetadata* new_block = (MallocMetadata*)((intptr_t)block + size + sizeof(malloc_metadata_t));
    INIT_METADATA(new_block, new_block_size - sizeof(malloc_metadata_t), true, block->next, block, nullptr, nullptr);
    insert_block_to_bin(new_block);
    if (block->next)
    {
        block->next->prev = new_block;
    }
    block->next = new_block;
    block->is_free  = false;
    block->size     = size;
    block->bin_next = nullptr;
    block->bin_prev = nullptr;
}







/**
 * @function:   static void* get_free_metadata_block(size_t size)
 * @brief:      search for suitable block from the alloc list (when block.size large enough).
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
    for (int i = GET_BIN_ENTRY(size); i < BIN_SIZE; i++)
    {
        if (free_block_bin[i] == nullptr) continue;
        METADATA_FOR_EACH(block, free_block_bin[i])
        {
            if (block && block->size >= size)
            {
                if (IS_LARGE_ENOUGH(block->size, size))
                {
                    cut_block(block, size);
                    block->is_free = false;
                    return block;
                }
                remove_from_bin(block);
                block->is_free = false;
                return block;
            }
        }
    }
    return nullptr;
}




/**
 * @function:   void* smalloc(size_t size)
 * @brief:  Searches for a free block with up to ‘size’ bytes or allocates (sbrk())
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
 *          If ‘size’ is 0 returns nullptr.
 *          If ‘size’ is more than 10^8 , return nullptr. 
 *          If sbrk fails, return nullptr.
 */
void* smalloc(size_t size)
{
    if (size > MAX_MALLOC_4_SIZE || size == 0)
    {
        return nullptr;
    }
    size = GET_SIZE_WITH_ALIGNMENT(size);

    // to big for sbrk, use mmap
    if (size >= MIN_KB_BLOCK)
    {
        void* ret = mmap(nullptr, size + sizeof(malloc_metadata_t), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if (ret == MAP_FAILED)
        {
            return nullptr;
        }
        MallocMetadata* mt = (MallocMetadata*)ret;
        INIT_METADATA(mt, size, false, nullptr, nullptr, nullptr, nullptr);
        insert_to_metadata_list(mt, &mmap_metadata_head);
        return GET_PTR_FROM_METADATA(ret);
    }
    // try to use free'd block
    MallocMetadata* freed = get_free_metadata_block(size);

    if (freed != nullptr)
    {
        return GET_PTR_FROM_METADATA(freed);
    }

    // try to expand the last brk

    MallocMetadata* last = get_last_metadata_block(metadata_head);
    if (last && last->is_free)
    {
        void* ret = sbrk(GET_SIZE_WITH_ALIGNMENT(size) - last->size);
        if ((intptr_t)ret == SBRK_FAIL)
        {
            return nullptr;
        }

        if (last->bin_prev == nullptr)
        {
            free_block_bin[GET_BIN_ENTRY(last->size)] = last->bin_next;
        }
        else
        {
            last->bin_prev->bin_next = last->bin_next;
            if (last->bin_next)
            {
                last->bin_next->bin_prev = last->bin_prev;
            }
        }
        last->is_free = false;
        last->size = size;
        return GET_PTR_FROM_METADATA(last);
    }

    void* ret = sbrk(size + sizeof(malloc_metadata_t));
    if ((intptr_t)ret == SBRK_FAIL)
    {
        return nullptr;
    }
    INIT_METADATA((MallocMetadata*)ret, size, false, nullptr, nullptr, nullptr, nullptr);
    MallocMetadata* mt = (MallocMetadata*)ret;

    insert_to_metadata_list(mt, &metadata_head);
    return GET_PTR_FROM_METADATA(mt);
}





/**
 * @function:   void* scalloc(size_t num, size_t size)
 * @brief:  Searches for a free block of up to ‘num’ elements, each ‘size’ bytes that 
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
 *          If ‘size’ is 0 returns nullptr.
 *          If ‘size’ is more than 10^8 , return nullptr. 
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
 * @brief:  Releases the usage of the block that starts with the pointer ‘p’. 
 * 
 * @arguments:
 *     - void* p: pointer to allocated block to free.
 *                If ‘p’ is nullptr or already released, simply returns.
 *                Presume that all pointers ‘p’ truly points to the beginning of an allocated block.
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
    // mmap block
    if (to_free->size >= MIN_KB_BLOCK)
    {
        remove_from_list(to_free, &mmap_metadata_head);
        munmap((void *)to_free, to_free->size + sizeof(malloc_metadata_t));
    }

    // sbrk block
    else
    {
        insert_block_to_bin(to_free);

        // try to merge with next
        if (to_free->next && to_free->next->is_free)
        {

            MallocMetadata* temp = to_free->next;
            to_free->next = temp->next;
            if (temp->next)
            {
                temp->next->prev = to_free;
            }
            
            remove_from_bin(temp);
            remove_from_bin(to_free);
            to_free->size += (temp->size + sizeof(malloc_metadata_t));
            insert_block_to_bin(to_free);
        }

        // try to merge with prev
        if (to_free->prev && to_free->prev->is_free)
        {
            MallocMetadata* temp = to_free->prev;
            temp->next = to_free->next;
            if (to_free->next)
            {
                to_free->next->prev = temp;
            }
            remove_from_bin(temp);
            remove_from_bin(to_free);
            temp->size += (to_free->size + sizeof(malloc_metadata_t));
            insert_block_to_bin(temp);
        }
        to_free->is_free = true;
    }
    return;
}



/**
 * @function:   void* srealloc(void* oldp, size_t size)
 * @brief:  If ‘size’ is smaller than the current block’s size, reuses the same block.
 *          Otherwise, finds/allocates ‘size’ bytes for a new space, copies content of oldp
 *          into the new allocated space and frees the oldp.
 * 
 * @arguments:
 *     - void* oldp: pointer to block-to-copy.
 *     - size_t size: # of bytes to allocate.
 * 
 * @returns:
 *     - Success:
 *          Returns pointer to the first byte in the (newly) allocated space.
 *          If ‘oldp’ is nullptr, allocates space for ‘size’ bytes and returns a pointer to it.
 *
 *     - Failure:
 *          If ‘size’ is 0 returns nullptr.
 *          If ‘size’ is more than 10^8 , return nullptr. 
 *          If sbrk fails, return nullptr.
 */
void* srealloc(void* oldp, size_t size)
{
    if (size > MAX_MALLOC_4_SIZE || size == 0)
    {
        return nullptr;
    }
    size = GET_SIZE_WITH_ALIGNMENT(size);

    if (oldp == nullptr)
    {
        return smalloc(size);
    }

    MallocMetadata* old_ptr = GET_METADATA_FROM_PTR(oldp);

    // ** oldp is mmap **
    if (old_ptr->size >= MIN_KB_BLOCK)
    {
        void* ret = smalloc(size);
        if (!ret) return nullptr;
        memmove(ret, oldp, MMIN(old_ptr->size, size));
        sfree(oldp);
        return ret;
    }

    // ** oldp is sbrk **
    
    // Try to reuse the current block without any merging
    if (old_ptr->size >= size)
    {
        if (IS_LARGE_ENOUGH(old_ptr->size, size))
        {
            cut_block(old_ptr, size);
            if (old_ptr->next && old_ptr->next->is_free && old_ptr->next->next && old_ptr->next->next->is_free)
            {
                MallocMetadata* base = old_ptr->next;
                MallocMetadata* base_next = old_ptr->next->next;
                remove_from_bin(base);
                remove_from_bin(base_next);
                base->next = base_next->next;
                if (base_next->next)
                {
                    base_next->next->prev = base;
                }
                base->size += sizeof(malloc_metadata_t) + base_next->size;
                insert_block_to_bin(base);
            }
        }
        return oldp;
    }

    // Try to merge with the adjacent block with the lower address.
    else if (old_ptr->prev && old_ptr->prev->is_free &&
             old_ptr->prev->size + old_ptr->size >= size)
    {
        MallocMetadata* ret = old_ptr->prev;
        remove_from_bin(ret);
        ret->is_free = false;
        ret->next = old_ptr->next;
        if (old_ptr->next)
        {
            old_ptr->next->prev = ret;
        }
        ret->size += old_ptr->size + sizeof(malloc_metadata_t);
        memmove(GET_PTR_FROM_METADATA(ret), oldp, MMIN(size, old_ptr->size));
        if (IS_LARGE_ENOUGH(ret->size, size))
        {
            cut_block(ret, size, false);
            //merge old_ptr_next with old_ptr_next_next
            if (ret->next && ret->next->is_free && ret->next->next && ret->next->next->is_free)
            {
                MallocMetadata* base = ret->next;
                MallocMetadata* base_next = ret->next->next;
                remove_from_bin(base);
                remove_from_bin(base_next);
                base->next = base_next->next;
                if (base_next->next)
                {
                    base_next->next->prev = base;
                }
                base->size += sizeof(malloc_metadata_t) + base_next->size;
                insert_block_to_bin(base);
            }
        }

        return GET_PTR_FROM_METADATA(ret);
    }

    // Try to merge with the adjacent block with the higher address.
    else if (old_ptr->next && old_ptr->next->is_free &&
             old_ptr->next->size + old_ptr->size >= size)
    {
        MallocMetadata* to_merge = old_ptr->next;
        remove_from_bin(to_merge);
        old_ptr->is_free = false;
        old_ptr->next = to_merge->next;
        if (to_merge->next)
        {
            to_merge->next->prev = old_ptr;
        }
        old_ptr->size += to_merge->size + sizeof(malloc_metadata_t);
        if (IS_LARGE_ENOUGH(old_ptr->size, size))
        {
            cut_block(old_ptr, size, false);
            if (old_ptr->next && old_ptr->next->is_free && old_ptr->next->next && old_ptr->next->next->is_free)
            {
                MallocMetadata* base = old_ptr->next;
                MallocMetadata* base_next = old_ptr->next->next;
                remove_from_bin(base);
                remove_from_bin(base_next);
                base->next = base_next->next;
                if (base_next->next)
                {
                    base_next->next->prev = base;
                }
                base->size += sizeof(malloc_metadata_t) + base_next->size;
                insert_block_to_bin(base);
            }
        }
        return GET_PTR_FROM_METADATA(old_ptr);
    }

    // Try to merge all those three adjacent blocks together
    else if (old_ptr->prev && old_ptr->prev->is_free &&
             old_ptr->next && old_ptr->next->is_free &&
             old_ptr->prev->size + old_ptr->next->size + old_ptr->size >= size)
    {
        MallocMetadata* prev = old_ptr->prev;
        MallocMetadata* next = old_ptr->next;
        remove_from_bin(prev);
        remove_from_bin(next);
        prev->is_free = false;
        prev->next = next->next;
        if (next->next)
        {
            next->next->prev = prev;
        }
        prev->size += old_ptr->size + next->size + 2*sizeof(malloc_metadata_t);
        memmove(GET_PTR_FROM_METADATA(prev), oldp, MMIN(size, old_ptr->size));
        if (IS_LARGE_ENOUGH(prev->size, size))
        {
            cut_block(prev, size, false);
            if (prev->next && prev->next->is_free && prev->next->next && prev->next->next->is_free)
            {
                MallocMetadata* base = prev->next;
                MallocMetadata* base_next = prev->next->next;
                remove_from_bin(base);
                remove_from_bin(base_next);
                base->next = base_next->next;
                if (base_next->next)
                {
                    base_next->next->prev = base;
                }
                base->size += sizeof(malloc_metadata_t) + base_next->size;
                insert_block_to_bin(base);
            }
        }
        return GET_PTR_FROM_METADATA(prev);
    }

    // Try to expand the last block (if free)
    else if (old_ptr->next == nullptr)
    {
        // take the free space before also
        if (old_ptr->prev && old_ptr->prev->is_free)
        {
            MallocMetadata* ret = old_ptr->prev;
            remove_from_bin(ret);
            ret->is_free = false;
            ret->next = old_ptr->next;
            if (old_ptr->next)
            {
                old_ptr->next->prev = ret;
            }
            ret->size += old_ptr->size + sizeof(malloc_metadata_t);
            void* mem = sbrk(size - ret->size);
            if ((intptr_t)mem == SBRK_FAIL)
            {
                return nullptr;
            }
            ret->size = size;
            memmove(GET_PTR_FROM_METADATA(ret), oldp, old_ptr->size);
            return GET_PTR_FROM_METADATA(ret);
        }
        
        // default case
        void* ret = sbrk(size - old_ptr->size);
        if ((intptr_t)ret == SBRK_FAIL)
        {
            return nullptr;
        }
        old_ptr->size = size;
        return oldp;
    }

    // Allocate a new block with sbrk()
    else
    {

        void* ret = smalloc(size);
        memmove(ret, oldp, MMIN(size, old_ptr->size));
        sfree(oldp);
        return ret;
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
    METADATA_FOR_EACH(block, mmap_metadata_head)
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

    METADATA_FOR_EACH(block, mmap_metadata_head)
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

    METADATA_FOR_EACH(block, mmap_metadata_head)
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