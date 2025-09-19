#ifndef CBUFFER_H
#define CBUFFER_H

#include <stddef.h>

typedef struct {
    int *buffer;     
    int head;        
    int tail;        
    int max;         
    int full;        
    size_t item_size;       // 每个元素的大小
} LingxinCircularBuffer;

// 初始化循环缓冲区
LingxinCircularBuffer* lingxin_cbuffer_init(int size, size_t item_size);

// 销毁循环缓冲区
void lingxin_cbuffer_free(LingxinCircularBuffer *cb);

// 向循环缓冲区添加一个元素
void lingxin_cbuffer_put(LingxinCircularBuffer *cb, const void *item);

// 从循环缓冲区读取一个元素
int lingxin_cbuffer_get(LingxinCircularBuffer *cb, void *item);

// 检查缓冲区是否为空
int lingxin_cbuffer_empty(LingxinCircularBuffer *cb);

// 检查缓冲区是否已满
int lingxin_cbuffer_full(LingxinCircularBuffer *cb);

// 获取缓冲区中的元素数量
int lingxin_cbuffer_size(LingxinCircularBuffer *cb);

#endif // CBUFFER_H
