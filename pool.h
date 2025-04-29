#include <unistd.h>
#define BIN_COUNT 64
typedef struct chk_t {
	size_t size;
} chk_t;
typedef struct free_t {
	size_t size;
	struct free_t *next;
} free_t;
typedef struct {
    void *original_brk;
    void *start;
    size_t size;
    size_t used;
	free_t *free_chunk;
} Pool;
extern Pool *pool;
void *alloc(size_t size);
Pool *pool_create(size_t size);
void pool_destroy(Pool *pool);
void pool_reuse(Pool *pool);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
size_t get_chunk_size(void *chunk);



