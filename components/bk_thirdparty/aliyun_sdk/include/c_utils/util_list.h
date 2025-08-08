/*
 * Copyright 2025 Alibaba Group Holding Ltd.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http: *www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _UTIL_LIST_H_
#define _UTIL_LIST_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

#define LIST_MAX_COUNT      (0x7FFFFFFE)
#define LIST_INVALID_INDEX  (0xFFFFFFFE)

typedef struct _util_list_node_t {
    void *data;
    struct _util_list_node_t *next;
} util_list_node_t;

typedef struct _util_list_t {
    util_list_node_t *head;
    util_list_node_t *tail;
    int32_t node_count;

    int32_t max_count;
} util_list_t;

#define util_list_foreach(list, node) for((node) = (list)->head; (list) && (node); (node) = (node)->next)

typedef void (*util_list_destory_data)(void *data);

/*
 * 初始化参数 max_count 用于设置链表最大节点数，当设置为0时，默认为LIST_MAX_NODE
 */
util_list_t * util_list_init(int32_t max_count);
void util_list_destory(util_list_t *list);
void util_list_destory_with_data(util_list_t *list, util_list_destory_data destory_func);
void * util_list_get(util_list_t *list);
int32_t util_list_add(util_list_t *list, void *data);
int32_t util_list_remove(util_list_t *list, void *data);
int32_t util_list_get_count(util_list_t *list);
void * util_list_get_node_by_index(util_list_t *list, int32_t idx);
bool util_list_found(util_list_t *list, void *data);

#ifdef __cplusplus
}
#endif

#endif
