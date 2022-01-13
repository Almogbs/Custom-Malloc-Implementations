/**
 * @file malloc_1.cpp
 * @author Art Vandelay
 * @version 0.1
 * @date 2022-01-13
 * @copyright Copyright (c) 2022
 */


#include <unistd.h>
#include <assert.h>

#define MAX_MALLOC_1_SIZE 100000000
#define SBRK_FAIL -1
#define NDEBUG


/**
 * @function:   void* smalloc(size_t size)
 * @brief:  Tries to allocate ‘size’ bytes.
 * 
 * @arguments:
 *     - size_t size: # of bytes to allocate.
 * 
 * @returns:
 *     - Success: a pointer to the first allocated byte within the allocated block.
 *     - Failure:
 *          If ‘size’ is 0 returns NULL.
 *          If ‘size’ is more than 10^8 , return NULL. 
 *          If sbrk fails, return NULL.
 */
void* smalloc(size_t size)
{
    if (size > MAX_MALLOC_1_SIZE || size == 0)
    {
        return NULL;
    }
    assert(size > 0);

    void* ret = sbrk(size);
    if ((intptr_t)ret == SBRK_FAIL)
    {
        return NULL;
    }
    return ret;
}