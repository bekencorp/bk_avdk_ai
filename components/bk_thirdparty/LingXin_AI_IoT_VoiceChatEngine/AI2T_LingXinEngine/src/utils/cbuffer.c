#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lingxin_cbuffer.h"
#include "lingxin_log.h"
#include "os/mem.h"

// 初始化循环缓冲区
LingxinCircularBuffer* lingxin_cbuffer_init(int size, size_t item_size) {
    LingxinCircularBuffer* cb = psram_malloc(sizeof(LingxinCircularBuffer));
    if (cb == NULL) {
        lingxin_log_error("Error: cbuffer结构体内存malloc失败");
        return NULL;
    }
    cb->buffer = psram_malloc(size * item_size);
    if (cb->buffer == NULL) {
        lingxin_log_error("Error: cbuffer->buffer内存malloc失败");
        free(cb);
        return NULL;
    }
    cb->max = size;
    cb->head = 0;
    cb->tail = 0;
    cb->full = 0;
    cb->item_size = item_size;
    return cb;
}

// 销毁循环缓冲区
void lingxin_cbuffer_free(LingxinCircularBuffer *cb) {
    if (cb == NULL) {
        lingxin_log_warn("Warning: cbuffer为NULL，无需销毁");
        return;
    }
    if (cb->buffer != NULL) {
        free(cb->buffer);
    }
    free(cb);
}

// 向循环缓冲区添加一个元素
void lingxin_cbuffer_put(LingxinCircularBuffer *cb, const void *item) {

    memcpy((char*)cb->buffer + cb->head * cb->item_size, item, cb->item_size);

    if (cb->full) {
        cb->tail = (cb->tail + 1) % cb->max;
    }

    cb->head = (cb->head + 1) % cb->max;
    cb->full = (cb->head == cb->tail);
}

// 从循环缓冲区读取一个元素
int lingxin_cbuffer_get(LingxinCircularBuffer *cb, void *item) {
    int success = 0;

    if (!cb->full && cb->head == cb->tail) {
        success = -1;  // 缓冲区为空
    } else {
        memcpy(item, (char*)cb->buffer + cb->tail * cb->item_size, cb->item_size);
        cb->full = 0;
        cb->tail = (cb->tail + 1) % cb->max;
        success = 0;
    }
    return success;
}

// 检查缓冲区是否为空
int lingxin_cbuffer_empty(LingxinCircularBuffer *cb) {
    int empty = (!cb->full && (cb->head == cb->tail));
    return empty;
}

// 检查缓冲区是否已满
int lingxin_cbuffer_full(LingxinCircularBuffer *cb) {
    int full = cb->full;
    return full;
}

// 获取缓冲区中的元素数量
int lingxin_cbuffer_size(LingxinCircularBuffer *cb) {
    int size = cb->max;

    if (!cb->full) {
        if (cb->head >= cb->tail) {
            size = cb->head - cb->tail;
        } else {
            size = cb->max + cb->head - cb->tail;
        }
    }

    return size;
}
