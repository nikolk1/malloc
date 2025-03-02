#include "malloc.h"

#define ALIGN 8

const int META_SIZE = sizeof(block_meta);
void *global_base = NULL;

block_meta *get_block_ptr(void *ptr)
{
    return (block_meta *)ptr - 1;
}

block_meta *find_free_block(block_meta **last, size_t size)
{
    block_meta *current = global_base;
    while (current && !(current->free && current->size >= size))
    {
        *last = current;
        current = current->next;
    }
    return current;
}

block_meta *request_space(block_meta *last, size_t size)
{
    struct block_meta *block;
    block = sbrk(0);
    void *request = sbrk(size + META_SIZE);
    assert((void *)block == request); // Not thread safe.
    if (request == (void *)-1)
    {
        return NULL; // sbrk failed.
    }

    if (last)
    { // NULL on first request.
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->magic = 0x12345678;
    return block;
}

void split_block(size_t req_size, block_meta *block)
{
    block_meta *leftovers;
    size_t left_size = block->size - (req_size + META_SIZE);

    if (left_size <= 0)
    {
        printf("Left size is below 0, something is wrong with the code");
        exit(1);
    }

    block_meta *next_block = block->next;

    block->free = 0;
    block->magic = 0x77777777;
    block->size = req_size + META_SIZE;
    block->next = get_block_ptr(block + block->size + 1); // problem here!!!

    leftovers = block->next;

    leftovers->free = 1;
    leftovers->magic = 0x55555555;
    leftovers->size = left_size;
    leftovers->next = next_block;

    merge_consecutive_blocks();
}

void *malloc(size_t size)
{
    block_meta *block;
    unsigned int aligned_size = align_block_size(size + META_SIZE);

    if (aligned_size <= 0)
    {
        return NULL;
    }

    if (!global_base)
    { // First call.
        block = request_space(NULL, aligned_size);
        if (!block)
        {
            return NULL;
        }
        global_base = block;
    }
    else
    {
        block_meta *last = global_base;
        block = find_free_block(&last, aligned_size);
        if (!block)
        { // Failed to find free block.
            block = request_space(last, aligned_size);
            if (!block)
            {
                return NULL;
            }
        }
        else
        { // Found free block
            split_block(aligned_size, block);
            block->free = 0;
            block->magic = 0x77777777;
        }
    }

    return (block + 1);
}

void free(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    // TODO: consider merging blocks once splitting blocks is implemented.
    block_meta *block_ptr = get_block_ptr(ptr);
    assert(block_ptr->free == 0);
    assert(block_ptr->magic == 0x77777777 || block_ptr->magic == 0x12345678);
    block_ptr->free = 1;
    block_ptr->magic = 0x55555555;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
    {
        // NULL ptr. realloc should act like malloc.
        return malloc(size);
    }

    struct block_meta *block_ptr = get_block_ptr(ptr);
    if (block_ptr->size >= size)
    {
        // We have enough space. Could free some once we implement split.
        return ptr;
    }

    // Need to really realloc. Malloc new space and free old space.
    // Then copy old data to new space.
    void *new_ptr;
    new_ptr = malloc(size);
    if (!new_ptr)
    {
        return NULL; // TODO: set errno on failure.
    }
    memcpy(new_ptr, ptr, block_ptr->size);
    free(ptr);
    return new_ptr;
}

void *calloc(size_t nelem, size_t elsize)
{
    size_t size = nelem * elsize; // TODO: check for overflow.
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

unsigned int align_block_size(size_t block_size)
{
    return ((((block_size + META_SIZE) / ALIGN) + 1) * ALIGN) - META_SIZE;
}

void merge_consecutive_blocks()
{
    block_meta *current = global_base;
    block_meta *next = current->next;
    while (next)
    {
        if (current->free == 1 && next->free == 1)
        {
            current->size += next->size + META_SIZE;
            current->next = next->next;

            next = next->next;
        }
        else
        {
            current = next;
            next = next->next;
        }
    }
}