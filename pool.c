
#include "pool.h"
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define ALIGN 8
#define MINIMUM ALIGN + sizeof(chk_t)

#define ALIGN_UP(size, align)  (((size) + (align) - 1) & ~((align) - 1))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
Pool *pool = NULL;
// Internal helpers


static void insert_bin(void *ptr) {
    free_t *chk = (free_t *)((chk_t *)ptr - 1);

    if (pool->free_chunk == NULL || chk < pool->free_chunk) {
        chk->next = pool->free_chunk;
        pool->free_chunk = chk;
    } else {
        free_t *tmp = pool->free_chunk;

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
	free_t *tmp = pool->free_chunk;
	free_t *prev = NULL;
	while (tmp != NULL) {
		size_t chk_size = tmp->size;
		if (chk_size == size) {
			if (prev == NULL)
				pool->free_chunk = tmp->next;
			else
				prev->next = tmp->next;
			tmp->next = NULL;
			return (chk_t *)tmp + 1; 
		} else if (chk_size > size) {
			if (chk_size - size > MINIMUM) {
				tmp->size = size;
				size_t rest_size = chk_size - size;
				free_t *rest_chk = (void *)tmp + size;
				rest_chk->size = rest_size;
				if (prev == NULL) {
					pool->free_chunk = rest_chk;
					pool->free_chunk->next = tmp->next;
				} else {
					prev->next = rest_chk;
					prev->next->next = tmp->next;
				}
				tmp->next = NULL;
				return (chk_t *)tmp + 1;
			}
		}
		prev = tmp;
		tmp = tmp->next;
	}
	return NULL;
}
void colapse(Pool *pool) {
	free_t *tmp = pool->free_chunk;
	free_t *prev = NULL;
	if (!tmp) return;
	while (tmp) {
		if ((void *)tmp + tmp->size == (void *)tmp->next){
			tmp->size = tmp->size + tmp->next->size;
			tmp->next = tmp->next->next;
			continue;
		}
		if ((void *)tmp + tmp->size == pool->start + pool->used) {
			pool->used -= tmp->size;
			if (prev == NULL) 
				pool->free_chunk = 0;
			else 
				prev->next = 0;
		};
		prev = tmp;
		tmp = tmp->next;
	}
}

void *alloc(size_t size) {
    if (pool == NULL) pool = pool_create(0x10000);
	if (pool->used < 0){
		perror("pool->used < 0 !!!");
		_exit(1);
	};
	colapse(pool);
	if (size == 0) return NULL;
    size = ALIGN_UP(size > ALIGN ? size : ALIGN, ALIGN) + sizeof(chk_t);

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

	chk_t *new = pool->start + pool->used;
	free_t *free_form = (free_t *)new;
	free_form->next = 0;
	pool->used += size;
	new->size  = size;
    return ((chk_t *)new + 1);
}

void dealloc(void *ptr) {
	if (ptr && ptr > pool->start && ptr < pool->start + pool->used)
		insert_bin(ptr);
}

void *realloc_mem(void *ptr, size_t size) {
    if (ptr == NULL) return alloc(size);
    if (size == 0) {
        dealloc(ptr);
        return NULL;
    }

    free_t *chunk = (free_t *)((chk_t *)ptr - 1);
    if (chunk->size >= size + sizeof(chk_t)) {
        return ptr;  
    }

    void *new_ptr = alloc(size);
    memcpy(new_ptr, ptr, chunk->size);
    dealloc(ptr);
    return new_ptr;
}
#if MYHEAP

void *malloc(size_t size) {
    return alloc(size);
}

void free(void *ptr) {
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
	p->free_chunk = NULL;
}

