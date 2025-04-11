#include "volc_fileio.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "volc_type.h"

uint32_t volc_file_read(const char* path, bool bin_mode, uint8_t* buffer, uint64_t* size) {
    uint64_t file_len = 0;
    uint32_t ret = VOLC_STATUS_SUCCESS;
    FILE* fp = NULL;

    VOLC_CHK(path != NULL && size != NULL, VOLC_STATUS_NULL_ARG);

    fp = fopen(path, bin_mode ? "rb" : "r");

    VOLC_CHK(fp != NULL, VOLC_STATUS_OPEN_FILE_FAILED);

    // Get the size of the file
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);

    if (buffer == NULL) {
        // requested the length - set and early return
        *size = file_len;
        VOLC_CHK(0, VOLC_STATUS_SUCCESS);
    }

    // Validate the buffer size
    VOLC_CHK(file_len <= *size, VOLC_STATUS_BUFFER_TOO_SMALL);

    // Read the file into memory buffer
    fseek(fp, 0, SEEK_SET);
    VOLC_CHK((fread(buffer, (size_t) file_len, 1, fp) == 1), VOLC_STATUS_READ_FILE_FAILED);

err_out_label:
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    return ret;
}

uint32_t volc_file_read_segment(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size) {
    uint64_t file_len;
    uint32_t ret = VOLC_STATUS_SUCCESS;
    FILE* fp = NULL;
    int32_t result = 0;

    VOLC_CHK(path != NULL && buffer != NULL && size != 0, VOLC_STATUS_NULL_ARG);

    fp = fopen(path, bin_mode ? "rb" : "r");

    VOLC_CHK(fp != NULL, VOLC_STATUS_OPEN_FILE_FAILED);

    // Get the size of the file
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);

    // Check if we are trying to read past the end of the file
    VOLC_CHK(offset + size <= file_len, VOLC_STATUS_READ_FILE_FAILED);

    // Set the offset and read the file content
    result = fseek(fp, (uint32_t) offset, SEEK_SET);
    VOLC_CHK(result == 0 && (fread(buffer, (size_t) size, 1, fp) == 1), VOLC_STATUS_READ_FILE_FAILED);

err_out_label:

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

uint32_t volc_file_write(const char* path, bool bin_mode, bool append, uint8_t* buffer, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    FILE* fp = NULL;

    VOLC_CHK(path != NULL && buffer != NULL, VOLC_STATUS_NULL_ARG);

    fp = fopen(path, bin_mode ? (append ? "ab" : "wb") : (append ? "a" : "w"));

    VOLC_CHK(fp != NULL, VOLC_STATUS_OPEN_FILE_FAILED);

    // Write the buffer to the file
    VOLC_CHK(fwrite(buffer, (size_t) size, 1, fp) == 1, VOLC_STATUS_WRITE_TO_FILE_FAILED);

err_out_label:

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

uint32_t volc_file_update(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    FILE* fp = NULL;
    uint32_t i;
    uint8_t* p_cur_ptr;

    VOLC_CHK(path != NULL && buffer != NULL, VOLC_STATUS_NULL_ARG);

    fp = fopen(path, bin_mode ? "rb+" : "r+");

    VOLC_CHK(fp != NULL, VOLC_STATUS_OPEN_FILE_FAILED);

    VOLC_CHK(0 == fseek(fp, (uint32_t) offset, SEEK_SET), VOLC_STATUS_INVALID_OPERATION);

    for (i = 0, p_cur_ptr = buffer + offset; i < size; i++, p_cur_ptr++) {
        VOLC_CHK(EOF != fputc(*p_cur_ptr, fp), VOLC_STATUS_WRITE_TO_FILE_FAILED);
    }

err_out_label:

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

uint32_t volc_file_get_length(const char* path, uint64_t* p_length) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    VOLC_CHK_STATUS(volc_file_read(path, 1, NULL, p_length));
err_out_label:

    return ret;
}

uint32_t volc_file_set_length(const char* path, uint64_t length) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int32_t ret_val, err_code, file_desc;

    VOLC_CHK(path != NULL, VOLC_STATUS_NULL_ARG);

    VOLC_UNUSED_PARAM(file_desc);

    ret_val = truncate(path, length);

    err_code = errno;

    if (ret_val == -1) {
        switch (err_code) {
            case EACCES:
                ret = VOLC_STATUS_DIRECTORY_ACCESS_DENIED;
                break;

            case ENOENT:
                ret = VOLC_STATUS_DIRECTORY_MISSING_PATH;
                break;

            case EINVAL:
                ret = VOLC_STATUS_INVALID_ARG_LEN;
                break;

            case EISDIR:
            case EBADF:
                ret = VOLC_STATUS_INVALID_ARG;
                break;

            case ENOSPC:
                ret = VOLC_STATUS_NOT_ENOUGH_MEMORY;
                break;

            default:
                ret = VOLC_STATUS_INVALID_OPERATION;
        }
    }

err_out_label:

    return ret;
}

uint32_t volc_file_exists(const char* path, bool* p_exists) {
    if (path == NULL || p_exists == NULL) {
        return VOLC_STATUS_NULL_ARG;
    }

    struct stat st;
    int32_t result = stat(path, &st);
    *p_exists = (result == 0);

    return VOLC_STATUS_SUCCESS;
}

uint32_t volc_file_create(const char* path, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    FILE* fp = NULL;

    VOLC_CHK(path != NULL, VOLC_STATUS_NULL_ARG);

    fp = fopen(path, "w+b");
    VOLC_CHK(fp != NULL, VOLC_STATUS_OPEN_FILE_FAILED);

    if (size != 0) {
        VOLC_CHK(0 == fseek(fp, (uint32_t)(size - 1), SEEK_SET), VOLC_STATUS_INVALID_OPERATION);
        VOLC_CHK(0 == fputc(0, fp), VOLC_STATUS_INVALID_OPERATION);
    }

err_out_label:

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

uint32_t volc_file_delete(const char* path) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    unlink(path);
err_out_label:
    return ret;
}