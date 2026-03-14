#ifndef hm_MALLOC_H
#define hm_MALLOC_H

#include <stddef.h>

void *hm_malloc(size_t size);
void hm_free(void *ptr);
void print_free_list(void);

#endif