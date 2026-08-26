#include "storage_sdk.h"

#include "ftl.h"
#include "nand_flash.h"

// 檢查 LBA 是否超出範圍
static int storage_valid_lba(uint64_t lba)
{
    return lba < (uint64_t)MAX_LBA;
}

int storage_init(void)
{
    ftl_init();
    return STORAGE_OK;
}

int storage_write(uint64_t lba, const uint8_t *data, size_t size)
{
    int rc;

    if (!data || size != PAGE_SIZE || !storage_valid_lba(lba)) {
        return STORAGE_ERR_INVALID;
    }

    // Error Translation
    rc = ftl_write((int)lba, data);
    if (rc == FTL_OK) {
        return STORAGE_OK;
    }
    if (rc == FTL_ERR_INVALID) {
        return STORAGE_ERR_INVALID;
    }
    return STORAGE_ERR_IO;
}

int storage_read(uint64_t lba, uint8_t *data, size_t size)
{
    int rc;

    if (!data || size != PAGE_SIZE || !storage_valid_lba(lba)) {
        return STORAGE_ERR_INVALID;
    }

    rc = ftl_read((int)lba, data);
    if (rc == FTL_OK) {
        return STORAGE_OK;
    }
    if (rc == FTL_ERR_UNMAPPED) {
        return STORAGE_ERR_UNMAPPED;
    }
    if (rc == FTL_ERR_INVALID) {
        return STORAGE_ERR_INVALID;
    }
    return STORAGE_ERR_IO;
}

int storage_flush(void)
{
    return STORAGE_OK;
}

int storage_trim(uint64_t lba)
{
    int rc;

    if (!storage_valid_lba(lba)) {
        return STORAGE_ERR_INVALID;
    }

    rc = ftl_trim((int)lba);
    if (rc == FTL_OK) {
        return STORAGE_OK;
    }
    if (rc == FTL_ERR_UNMAPPED) {
        return STORAGE_ERR_UNMAPPED;
    }
    if (rc == FTL_ERR_INVALID) {
        return STORAGE_ERR_INVALID;
    }
    return STORAGE_ERR_IO;
}
