# Custom-Malloc-Implementations

## Four Different Implementations of Malloc/Free

### Malloc_1 - Malloc Level 1:
Just expanding the program break (without any optimization).

Functions:
- void* smalloc(size_t size): Tries to allocate ‘size’ bytes

### Malloc_2 - Malloc Level 2:
Saves Meta-Data for every allocation and store them in ordered linked list.
Expanding the program break only if there are no allocations that were free'd before.

Functions:
- void* smalloc(size_t size): Searches for a free block with up to ‘size’ bytes 
        or allocates (sbrk()) one if none are found
- void* scalloc(size_t num, size_t size): Searches for a free block of up to ‘num’ elements,
        each ‘size’ bytes that are all set to 0 or allocates if none are found
- void* srealloc(void* oldp, size_t size): If ‘size’ is smaller than the current block’s size,
        reuses the same block. Otherwise, finds/allocates ‘size’ bytes for a new space, copies
        content of oldp into the new allocated space and frees the oldp
- void sfree(void* p): Releases the usage of the block that starts with the pointer ‘p’
### Malloc_3 - Malloc Level 3:
### Malloc_4 - Malloc Level 4:
## See Code For More Details

## Download:
    git clone https://github.com/Almogbs/Custom-Malloc-Implementations.git

## Compile:
    cd Custom-Malloc-Implementations
    ./build.sh <ARG>
    when:
        ARG = 1 - compile malloc_1.cpp
        ARG = 2 - compile malloc_2.cpp
        ARG = 3 - compile malloc_3.cpp
        ARG = 4 - compile malloc_4.cpp
        ARG = "all" - compile malloc_N.cpp for N = 1, 2, 3, 4
    or use the Makefile
