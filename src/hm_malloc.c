#include "hm_malloc.h"
#include <stdio.h>
#include <unistd.h>

#define ALIGN 8
#define ALIGN_SIZE(size) (((size) + (ALIGN - 1)) & ~(ALIGN - 1))

#define SET_ALLOCATED(block) ((block)->size |= 1)
#define SET_FREE(block) ((block)->size &= ~1)
#define IS_ALLOCATED(block) ((block)->size & 1)
#define GET_SIZE(block) ((block)->size & ~1)
#define GET_SIZE_FROM_FOOTER(footer) ((footer) & ~1)

typedef struct mem_block_header {
    size_t size;
    struct mem_block_header *prev;
    struct mem_block_header *next;
} block_t;

static block_t *free_list_head = NULL;
static void *heap_start = NULL;

static void list_insert_head(block_t *node);
static void list_remove(block_t *node);
static block_t *get_next_adjacent(block_t *block);
static block_t *get_prev_adjacent(block_t *block);
static int is_heap_end(block_t *block);
static void update_footer(block_t *block);
static block_t *split_block(block_t *block, size_t size);
static block_t *coalesce(block_t *block);

static void list_insert_head(block_t *node) {
    node->prev = NULL;
    node->next = free_list_head;
    if (free_list_head != NULL) {
        free_list_head->prev = node;
    }
    free_list_head = node;
}

static void list_remove(block_t *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        free_list_head = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    node->next = NULL;
    node->prev = NULL;
}

static block_t *get_next_adjacent(block_t *block) {
    size_t size = GET_SIZE(block);
    return (block_t *)((unsigned char *)(block + 1) + size + sizeof(size_t));
}

static block_t *get_prev_adjacent(block_t *block) {
    if ((void *)block == heap_start) {
        return NULL;
    }
    size_t *prev_footer = (size_t *)((unsigned char *)block - sizeof(size_t));
    size_t prev_size = GET_SIZE_FROM_FOOTER(*prev_footer);
    return (block_t *)((unsigned char *)block - sizeof(block_t) - prev_size - sizeof(size_t));
}

static int is_heap_end(block_t *block) {
    void *current_brk = sbrk(0);
    void *block_end = (unsigned char *)(block + 1) + GET_SIZE(block) + sizeof(size_t);
    return (block_end >= current_brk);
}

static void update_footer(block_t *block) {
    size_t *footer = (size_t *)((unsigned char *)(block + 1) + GET_SIZE(block));
    *footer = block->size;
}

static block_t *split_block(block_t *block, size_t size) {
    size_t current_size = GET_SIZE(block);
    size_t remaining_space = current_size - size;
    size_t min_new_block = sizeof(block_t) + sizeof(size_t);
    
    if (remaining_space <= min_new_block) {
        SET_ALLOCATED(block);
        update_footer(block);
        return NULL;
    }
    
    size_t new_block_size = remaining_space - sizeof(block_t) - sizeof(size_t);
    
    block_t *new_block = (block_t *)((unsigned char *)(block + 1) + size + sizeof(size_t));
    new_block->size = new_block_size;
    SET_FREE(new_block);
    update_footer(new_block);
    
    block->size = size;
    SET_ALLOCATED(block);
    update_footer(block);
    
    return new_block;
}

static block_t *coalesce(block_t *block) {
    if (!is_heap_end(block)) {
        block_t *next_block = get_next_adjacent(block);
        if (!IS_ALLOCATED(next_block)) {
            list_remove(next_block);
            block->size = GET_SIZE(block) + sizeof(block_t) + GET_SIZE(next_block) + sizeof(size_t);
            SET_FREE(block);
            update_footer(block);
        }
    }
    
    block_t *prev_block = get_prev_adjacent(block);
    if (prev_block != NULL && !IS_ALLOCATED(prev_block)) {
        list_remove(prev_block);
        prev_block->size = GET_SIZE(prev_block) + sizeof(block_t) + GET_SIZE(block) + sizeof(size_t);
        SET_FREE(prev_block);
        update_footer(prev_block);
        block = prev_block;
    }
    
    return block;
}

void *hm_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size = ALIGN_SIZE(size);
    
    block_t *current = free_list_head;
    while (current != NULL) {
        if (GET_SIZE(current) >= size) {
            list_remove(current);
            
            block_t *remainder = split_block(current, size);
            if (remainder != NULL) {
                list_insert_head(remainder);
            }
            
            return (void *)(current + 1);
        }
        current = current->next;
    }

    current = sbrk(sizeof(block_t) + size + sizeof(size_t));
    if (current == (void *)-1) {
        return NULL;
    }
    
    if (heap_start == NULL) {
        heap_start = current;
    }
    
    current->size = size;
    SET_ALLOCATED(current);
    update_footer(current);
    
    return (void *)(current + 1);
}

void hm_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    block_t *block = (block_t *)ptr - 1;
    SET_FREE(block);
    update_footer(block);
    
    block = coalesce(block);
    list_insert_head(block);
}

void print_free_list(void) {
    printf("\n=== Free List ===\n");
    if (free_list_head == NULL) {
        printf("Empty\n");
        printf("=================\n\n");
        return;
    }
    
    block_t *current = free_list_head;
    int i = 0;
    while (current != NULL) {
        printf("Block %d: addr=%p, size=%zu, allocated=%d\n", 
               i, (void*)current, GET_SIZE(current), IS_ALLOCATED(current));
        current = current->next;
        i++;
    }
    printf("=================\n\n");
}