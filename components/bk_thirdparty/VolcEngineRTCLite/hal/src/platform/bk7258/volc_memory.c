#include "volc_memory.h"

#include <stdlib.h>
#include <esp_heap_caps.h>

void* volc_malloc(size_t size) {
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
}

void* volc_align_alloc(size_t size, size_t alignment) {
    return heap_caps_aligned_alloc(alignment,size,MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
}

void* volc_calloc(size_t num, size_t size) {
    return heap_caps_calloc(num,size,MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
}

void* volc_realloc(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr,new_size,MALLOC_CAP_SPIRAM | MALLOC_CAP_DEFAULT);
}

void volc_free(void* ptr) {
    heap_caps_free(ptr);
}

bool volc_memory_check(void* ptr, uint8_t val, size_t size) {
    uint8_t* p_buf = (uint8_t *)ptr;
    
    if (NULL == p_buf) {
        return false;
    }

    for (int i = 0; i < size; p_buf++, i++) {
        if (*p_buf != val) {
            return false;
        }
    }

    return true;
}