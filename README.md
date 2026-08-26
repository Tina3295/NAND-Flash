# NAND Flash Simulator (C)

此專案實作了一個簡化的 SSD Software Stack：

`Host -> SDK -> FTL -> FCL -> NAND`

包含：

- NAND 基本操作與限制（不可直接重寫已寫入 page）
- Bad block、Program/Erase 計數
- FTL（LBA->PPA mapping, out-of-place write）
- GC（搬移 valid page + erase victim block）
- FCL（Flash Command Layer）
- SDK API（給上層呼叫）
- DV 測試框架

## 檔案說明

- [nand_flash.h](</C:/CFile/NAND Flash/nand_flash.h>) / [nand_flash.c](</C:/CFile/NAND Flash/nand_flash.c>): NAND layer
- [fcl.h](</C:/CFile/NAND Flash/fcl.h>) / [fcl.c](</C:/CFile/NAND Flash/fcl.c>): FCL layer
- [ftl.h](</C:/CFile/NAND Flash/ftl.h>) / [ftl.c](</C:/CFile/NAND Flash/ftl.c>): FTL + GC
- [storage_sdk.h](</C:/CFile/NAND Flash/storage_sdk.h>) / [storage_sdk.c](</C:/CFile/NAND Flash/storage_sdk.c>): SDK API
- [dv.h](</C:/CFile/NAND Flash/dv.h>) / [dv.c](</C:/CFile/NAND Flash/dv.c>): DV tests
- [main.c](</C:/CFile/NAND Flash/main.c>): SDK demo + DV test runner

## SDK API

```c
int storage_init(void);
int storage_write(uint64_t lba, const uint8_t *data, size_t size);
int storage_read(uint64_t lba, uint8_t *data, size_t size);
int storage_flush(void);
int storage_trim(uint64_t lba);
```

目前每次 write/read 以單一 page（`PAGE_SIZE`）為單位。

## DV 測試項目

- Sequential Write
- Sequential Read
- Random Write
- Random Read
- Overwrite
- Garbage Collection
- Power Failure (Simulated)
- Bad Block
- Full Disk
- Read After Write

## 編譯與執行（Windows + GCC）

```powershell
gcc "C:\CFile\NAND Flash\main.c" "C:\CFile\NAND Flash\nand_flash.c" "C:\CFile\NAND Flash\fcl.c" "C:\CFile\NAND Flash\ftl.c" "C:\CFile\NAND Flash\storage_sdk.c" "C:\CFile\NAND Flash\dv.c" -o "C:\CFile\NAND Flash\nand_flash.exe"
```

```powershell
& "C:\CFile\NAND Flash\nand_flash.exe"
```
