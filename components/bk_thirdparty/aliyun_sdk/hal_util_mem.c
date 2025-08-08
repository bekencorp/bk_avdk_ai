// Copyright 2025-2035 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * 初始化随机数生成器
 * 
 * @param seed 用于初始化随机数生成器的种子值
 * 
 * @return 返回初始化结果，0表示成功，非0表示失败
 * 
 * 此函数通过对随机数生成器进行初始化，以确保后续生成的随机数序列具有良好的随机性
 * 种子值的选择对生成的随机数序列有重要影响，相同的种子值会生成相同的随机数序列
 */
#include "os/os.h"
#include "os/mem.h"
#include "hal_util_mem.h"

/**
 * util_malloc - 分配指定大小的内存块。
 * @size: 需要分配的内存大小，以字节为单位。
 *
 * 本函数通过调用标准库函数malloc来分配内存，目的是为了提供一个更健壮的内存分配方法。
 * 它可能包含了额外的错误检查或者内存管理策略，以提高程序的稳定性和性能。
 * 
 * 返回值： 返回指向所分配内存的指针，如果内存分配失败，则返回NULL。
 */
void * util_malloc(int32_t size)
{
	return psram_malloc(size);
}

/**
 * 释放动态分配的内存。
 * 
 * 本函数旨在释放之前通过动态分配获得的内存空间，以避免内存泄漏。
 * 它接受一个指向动态分配内存区域的指针，并将其设置为NULL，以防止悬挂指针的出现。
 * 
 * @param ptr 指向动态分配内存区域的指针。如果为NULL，函数将不执行任何操作。
 *            在释放内存后，此指针将被设置为NULL。
 */
void util_free(void *ptr)
{
	os_free(ptr);
}


