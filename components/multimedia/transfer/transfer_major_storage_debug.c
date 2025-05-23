// Copyright 2024-2025 Beken
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


#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_posix.h"
#include "driver/flash_partition.h"
#if CONFIG_LITTLEFS_USE_LITTLEFS_PARTITION
#include "vendor_flash_partition.h"
#endif


#define TRANSFER_MAJOR_STORAGE_TAG "tms_image"
#define LOGI(...) BK_LOGI(TRANSFER_MAJOR_STORAGE_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TRANSFER_MAJOR_STORAGE_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TRANSFER_MAJOR_STORAGE_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TRANSFER_MAJOR_STORAGE_TAG, ##__VA_ARGS__)


#define IMAGE_NUM_MAX   (1)

#define MOUNT_ENABLE


#ifdef MOUNT_ENABLE
int transfer_major_storage_mount(void)
{
    int ret = BK_FAIL;

    LOGI("%s\n", __func__);

#if (CONFIG_FATFS)
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;

    fs_name = "fatfs";
    partition.part_type = FATFS_DEVICE;
#if (CONFIG_SDCARD)
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
#else
    partition.part_dev.device_name = FATFS_DEV_FLASH;
#endif
    partition.mount_path = "/";

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
#endif

#if (CONFIG_LITTLEFS)

    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
#ifdef BK_PARTITION_LITTLEFS_USER
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_LITTLEFS_USER);
#else
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
#endif

    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = pt->partition_start_addr;
    partition.part_flash.size = pt->partition_length;
    partition.mount_path = "/";

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
#endif

    if (ret != BK_OK)
    {
        LOGE("%s, %d, mount fail, ret: %d\n", __FUNCTION__, __LINE__, ret);
        return BK_FAIL;
    }
    else
    {
        LOGI("mount success\n");
        return BK_OK;
    }
}

int transfer_major_storage_unmount(void)
{
    bk_err_t ret = BK_FAIL;

    LOGI("%s\n", __func__);

    ret = umount("/");
    if (BK_OK != ret)
    {
        LOGE("%s, %d, unmount fail:%d\n", __FUNCTION__, __LINE__, ret);
    }
    else
    {
        LOGI("unmount success\n");
    }

    return ret;
}
#endif

void transfer_major_storage_image(uint8_t *frame, uint32_t len)
{
    //char image_url[] = "/%image.h264";
    char image_url[20];
    static uint32_t image_index = 1;

    LOGI("%s\n", __func__);

    if (image_index > IMAGE_NUM_MAX)
    {
        image_index = 1;
    }
#if (CONFIG_VIDEO_ENGINE_JPEG_FORMAT)
    os_snprintf(image_url, 20, "/%d.jpeg", image_index);
#endif

#if (CONFIG_VIDEO_ENGINE_H264_FORMAT)
    os_snprintf(image_url, 20, "/%d.h264", image_index);
#endif

    LOGI("%s, url: %s\n", __func__, image_url);

    int fd = open(image_url, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        LOGE("%s, open file fail\n", __func__);
        return;
    }

    uint32_t wt_size = write(fd, frame, len);
    if (wt_size != len)
    {
        LOGE("%s, write size: %d != %d\n", __func__, wt_size, len);
    }

    close(fd);
    fd = -1;

    image_index++;

    LOGI("storage image complete\n");

    return;
}

