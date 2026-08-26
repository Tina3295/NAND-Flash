#include "dv.h"

#include "fcl.h"
#include "ftl.h"
#include "nand_flash.h"
#include "storage_sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定義測試函數類型
typedef int (*DvTestFn)(void);
// 定義測試案例結構
typedef struct DvTestCase {
    const char *name;
    DvTestFn fn;
} DvTestCase;

// 測試：連續寫入是否正常
static int test_sequential_write(void)
{
    uint8_t data[PAGE_SIZE];
    int lba;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    for (lba = 0; lba < 32; lba++) {
        memset(data, lba, sizeof(data));
        if (storage_write((uint64_t)lba, data, sizeof(data)) != STORAGE_OK) {
            return 0;
        }
    }
    return 1;
}

// 測試：連續讀取是否正常(LBA 0~31)
static int test_sequential_read(void)
{
    uint8_t write_data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];
    int lba;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    for (lba = 0; lba < 32; lba++) {
        memset(write_data, lba, sizeof(write_data));
        if (storage_write((uint64_t)lba, write_data, sizeof(write_data)) != STORAGE_OK) {
            return 0;
        }
    }
    for (lba = 0; lba < 32; lba++) {
        memset(write_data, lba, sizeof(write_data));
        if (storage_read((uint64_t)lba, read_data, sizeof(read_data)) != STORAGE_OK) {
            return 0;
        }
        if (memcmp(write_data, read_data, sizeof(read_data)) != 0) {
            return 0;
        }
    }
    return 1;
}

static int test_random_write(void)
{
    uint8_t data[PAGE_SIZE];
    int i;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    srand(7);
    for (i = 0; i < 64; i++) {
        uint64_t lba = (uint64_t)(rand() % 128);
        memset(data, i, sizeof(data));
        if (storage_write(lba, data, sizeof(data)) != STORAGE_OK) {
            return 0;
        }
    }
    return 1;
}

static int test_random_read(void)
{
    uint8_t write_data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];
    uint8_t expected[128];
    int valid[128];
    int i;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }
    memset(valid, 0, sizeof(valid));

    srand(9);
    for (i = 0; i < 128; i++) {
        uint64_t lba = (uint64_t)(rand() % 128);
        expected[lba] = (uint8_t)(i + 1);
        valid[lba] = 1;
        memset(write_data, expected[lba], sizeof(write_data));
        if (storage_write(lba, write_data, sizeof(write_data)) != STORAGE_OK) {
            return 0;
        }
    }

    for (i = 0; i < 128; i++) {
        if (!valid[i]) {
            continue;
        }
        if (storage_read((uint64_t)i, read_data, sizeof(read_data)) != STORAGE_OK) {
            return 0;
        }
        if (read_data[0] != expected[i]) {
            return 0;
        }
    }
    return 1;
}

static int test_overwrite(void)
{
    uint8_t data_a[PAGE_SIZE];
    uint8_t data_b[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    memset(data_a, 0x11, sizeof(data_a));
    memset(data_b, 0x22, sizeof(data_b));
    if (storage_write(100, data_a, sizeof(data_a)) != STORAGE_OK) {
        return 0;
    }
    if (storage_write(100, data_b, sizeof(data_b)) != STORAGE_OK) {
        return 0;
    }
    if (storage_read(100, read_data, sizeof(read_data)) != STORAGE_OK) {
        return 0;
    }
    return memcmp(data_b, read_data, sizeof(read_data)) == 0;
}

static int test_garbage_collection(void)
{
    uint8_t data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];
    FtlStats stats;
    int lba;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    for (lba = 0; lba < 16; lba++) {
        memset(data, lba + 1, sizeof(data));
        if (storage_write((uint64_t)lba, data, sizeof(data)) != STORAGE_OK) {
            return 0;
        }
    }
    for (lba = 0; lba < 16; lba += 2) {
        memset(data, 0xA0 + lba, sizeof(data));
        if (storage_write((uint64_t)lba, data, sizeof(data)) != STORAGE_OK) {
            return 0;
        }
    }

    if (ftl_garbage_collect() != FTL_OK) {
        return 0;
    }
    if (ftl_get_stats(&stats) != FTL_OK) {
        return 0;
    }
    if (stats.gc_count == 0 || stats.valid_pages_copied == 0) {
        return 0;
    }

    if (storage_read(0, read_data, sizeof(read_data)) != STORAGE_OK || read_data[0] != 0xA0) {
        return 0;
    }
    return 1;
}

// 測試：模擬電源故障後，資料是否仍然正確(尚未實作
static int test_power_failure_simulated(void)
{
    uint8_t data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    memset(data, 0x5A, sizeof(data));
    if (storage_write(10, data, sizeof(data)) != STORAGE_OK) {
        return 0;
    }
    if (storage_flush() != STORAGE_OK) {
        return 0;
    }
    if (storage_read(10, read_data, sizeof(read_data)) != STORAGE_OK) {
        return 0;
    }
    return memcmp(data, read_data, sizeof(read_data)) == 0;
}

static int test_bad_block(void)
{
    uint8_t data[PAGE_SIZE];
    int b;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    for (b = 0; b < 3; b++) {
        if (nand_mark_bad_block(b) != NAND_OK) {
            return 0;
        }
        if (fcl_is_bad_block(b) != 1) {
            return 0;
        }
    }

    memset(data, 0x77, sizeof(data));
    return storage_write(0, data, sizeof(data)) == STORAGE_OK;
}

static int test_full_disk(void)
{
    uint8_t data[PAGE_SIZE];
    int lba;

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    memset(data, 0xEE, sizeof(data));
    for (lba = 0; lba < MAX_LBA; lba++) {
        if (storage_write((uint64_t)lba, data, sizeof(data)) != STORAGE_OK) {
            return 0;
        }
    }

    return storage_write((uint64_t)MAX_LBA, data, sizeof(data)) != STORAGE_OK;
}

static int test_read_after_write(void)
{
    uint8_t write_data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];

    if (storage_init() != STORAGE_OK) {
        return 0;
    }

    memset(write_data, 0x3C, sizeof(write_data));
    if (storage_write(200, write_data, sizeof(write_data)) != STORAGE_OK) {
        return 0;
    }
    if (storage_read(200, read_data, sizeof(read_data)) != STORAGE_OK) {
        return 0;
    }
    return memcmp(write_data, read_data, sizeof(read_data)) == 0;
}

// 執行所有 DV 測試
int run_dv_tests(void)
{
    DvTestCase tests[] = {
        {"Sequential Write", test_sequential_write},
        {"Sequential Read", test_sequential_read},
        {"Random Write", test_random_write},
        {"Random Read", test_random_read},
        {"Overwrite", test_overwrite},
        {"Garbage Collection", test_garbage_collection},
        {"Power Failure (Simulated)", test_power_failure_simulated},
        {"Bad Block", test_bad_block},
        {"Full Disk", test_full_disk},
        {"Read After Write", test_read_after_write}
    };
    int total;
    int passed;
    int i;

    total = (int)(sizeof(tests) / sizeof(tests[0]));
    passed = 0;

    puts("========== DV TEST ==========");
    puts("");
    for (i = 0; i < total; i++) {
        int ok = tests[i].fn();
        printf("%-24s %s\n", tests[i].name, ok ? "PASS" : "FAIL");
        if (ok) {
            passed++;
        }
    }
    puts("");
    puts("==============================");
    if (passed == total) {
        puts("ALL TESTS PASSED");
        return 0;
    }
    printf("%d/%d tests passed\n", passed, total);
    return 1;
}
