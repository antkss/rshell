
#include "pool.h"
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define ALIGN 8
#define HEADER_SIZE 8
#define HEADER(chunk, size) chunk[0] = size;
#define MINIMUM 0x10

#define ALIGN_UP(size, align)  (((size) + (align) - 1) & ~((align) - 1))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
Pool *pool = NULL;
// Internal helpers


static void insert_bin(void *ptr) {
    Chunk *chk = (Chunk *)ptr;

    if (pool->free_chunk == NULL || chk < pool->free_chunk) {
        chk->next = pool->free_chunk;
        pool->free_chunk = chk;
    } else {
        Chunk *tmp = pool->free_chunk;

        while (tmp->next != NULL && tmp->next < chk) {
            tmp = tmp->next;
        }

        chk->next = tmp->next;
        tmp->next = chk;
    }
}


// Pool creation

Pool *pool_create(size_t size) {
    void *base = sbrk(0);
    if (sbrk(size + sizeof(Pool)) == (void *)-1) {
        perror("sbrk failed");
        return NULL;
    }
    Pool *p = base;
    p->original_brk = base;
    p->start = (char *)base + sizeof(Pool);
    p->size = size;
    p->used = 0;
	p->free_chunk = 0;
    return p;
}

void *bin_alloc(size_t size) {
	if (!pool->free_chunk) return NULL;
	size = ALIGN_UP(size, ALIGN);
	Chunk *tmp = pool->free_chunk;
	Chunk *prev = NULL;
	while (tmp != NULL) {
		size_t chk_size = get_chunk_size(tmp);
		if (chk_size == size) {
			if (prev == NULL)
				pool->free_chunk = tmp->next ;
			else
				prev->next = tmp->next;
			tmp->next = NULL;
			return tmp; 
		} else if (chk_size > size) {
			if (chk_size - size > MINIMUM) {
				__int64_t *ptr = (__int64_t *)tmp;
				ptr[-1] = size;
				size_t rest_size = chk_size - size;
				__int64_t *rest_chk = (__int64_t *)((__int64_t)tmp + size);
				rest_chk[-1] = rest_size;
				if (prev == NULL) {
					pool->free_chunk = (Chunk *)rest_chk;
					pool->free_chunk->next = tmp->next;
				} else {
					prev->next = (Chunk *)rest_chk;
					prev->next->next = tmp->next;
				}
				tmp->next = NULL;
				return tmp;
			}
		}
		prev = tmp;
		tmp = tmp->next;
	}
	tmp = pool->free_chunk;
	return NULL;
}
void colapse(Pool *pool) {
	Chunk *tmp = pool->free_chunk;
	if (!tmp) return;
	while (tmp) {
		if ((__int64_t)tmp + get_chunk_size(tmp) == (__int64_t)tmp->next){
			__int64_t *rp = (__int64_t *)tmp;
			rp[-1] = get_chunk_size(tmp) + get_chunk_size(tmp->next);
			tmp->next = tmp->next->next;
			continue;
		};
		tmp = tmp->next;
	}
	tmp = pool->free_chunk;
	Chunk *prev = NULL;
	while (tmp && tmp->next) {
		prev = tmp;
		tmp = tmp->next;
	}
	size_t chk_size = get_chunk_size(tmp);
	if ((__int64_t)tmp + chk_size - 8 == (__int64_t)(pool->start + pool->used)) { // minus 8 because of the ptr
		pool->used -= chk_size;
		if (prev == NULL)
			pool->free_chunk = 0;
		else
			prev->next = 0;
	}
}
size_t get_chunk_size(void *chunk) {
    return *((size_t*) ((uintptr_t)chunk - sizeof(size_t))); 
}

void *alloc(size_t size) {
    if (pool == NULL) pool = pool_create(0x10000);
	if (pool->used < 0){
		perror("pool->used < 0 !!!");
		_exit(1);
	};
	colapse(pool);
	// now size will be calculated by this include HEADER_SIZE
	if (size == 0) return NULL;
    size = ALIGN_UP(size > 8 ? size : 8, ALIGN) + HEADER_SIZE;

	void *bin_chk = bin_alloc(size);
	if (bin_chk) return bin_chk;
	// check grow
	if (pool->size < size + pool->used) {
		if (sbrk(size) == (void *)-1) {
			perror("sbrk failed");
			return NULL;
		}
		pool->size += size;
	}

	__int64_t *new = pool->start + pool->used;
	HEADER(new, size)
	pool->used = pool->used + size;
	new[1] = 0;
    return &new[1];
}

void dealloc(void *ptr) {
    if (!ptr) return;
    insert_bin(ptr);
}

void *realloc_mem(void *ptr, size_t size) {
    if (ptr == NULL) return alloc(size);
    if (size == 0) {
        dealloc(ptr);
        return NULL;
    }

    __int64_t *chunk = ptr;
	size_t chk_size = get_chunk_size(chunk);
    if (chk_size >= size + HEADER_SIZE) {
        return ptr;  // existing chunk is big enough
    }

    void *new_ptr = alloc(size);
    memcpy(new_ptr, ptr, chk_size);
    dealloc(ptr);
    return new_ptr;
}
#if MYHEAP

void *malloc(size_t size) {
    return alloc(size);
}

void free(void *ptr) {
	size_t region = (__int64_t)ptr - (__int64_t)pool->start;
	if (region > pool->used) return;
    dealloc(ptr);
}

void *realloc(void *ptr, size_t size) {
    return realloc_mem(ptr, size);
}
void *calloc(size_t num, size_t size) {
	size_t total = num * size;
	return alloc(total);
}
#endif

void pool_destroy(Pool *p) {
    if (brk(p->original_brk) == -1) {
        perror("brk reset failed");
    }
    pool = NULL;
}

void pool_reuse(Pool *p) {
    p->used = 0;
    memset(p->bins, 0, sizeof(p->bins));
}

