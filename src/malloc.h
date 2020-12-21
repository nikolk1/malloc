#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct block_meta
{
    size_t size;
    struct block_meta *next;
    int free;
    int magic; // For debugging only. TODO: remove
};
typedef struct block_meta block_meta;

block_meta *get_block_ptr(void *ptr);
block_meta *find_free_block(block_meta **last, size_t size);
block_meta *request_space(block_meta *last, size_t size);
void split_block(size_t req_size, block_meta *block);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nelem, size_t elsize);