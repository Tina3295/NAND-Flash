#ifndef STORAGE_SDK_H
#define STORAGE_SDK_H

#include <stddef.h> // for size_t 表示資料有多大？
#include <stdint.h> // for uint8_t, uint64_t

//定義錯誤碼
#define STORAGE_OK            0
#define STORAGE_ERR_INVALID  -200
#define STORAGE_ERR_UNMAPPED -201
#define STORAGE_ERR_IO       -202

int storage_init(void);
                              // 指向 byte 資料的指標。把data寫入LBA，size表示資料有多大
int storage_write(uint64_t lba, const uint8_t *data, size_t size);
int storage_read(uint64_t lba, uint8_t *data, size_t size);
int storage_flush(void);
// 標記 LBA 為無效，以便收垃圾
int storage_trim(uint64_t lba);

#endif
