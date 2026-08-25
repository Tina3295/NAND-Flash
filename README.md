# NAND Flash Simulator (C)

此專案用 C 實作一個 NAND Flash 模擬器，包含：

- NAND 基本操作：Program / Read / Block Erase
- NAND 限制：已寫過的 page 不能直接 overwrite
- Bad Block 機制
- Program / Erase 計數
- 簡化版 FTL（LBA -> PPA mapping）
- FCL（Flash Command Layer）
- Garbage Collection（搬移 valid page + erase victim block）
- GC 統計與 Write Amplification

資料路徑：

`Host -> FTL -> FCL -> NAND`

---

## 專案檔案

- [main.c](</C:/CFile/NAND Flash/main.c>)：整合測試與示範流程
- [nand_flash.h](</C:/CFile/NAND Flash/nand_flash.h>)：NAND 介面與錯誤碼
- [nand_flash.c](</C:/CFile/NAND Flash/nand_flash.c>)：NAND 模擬實作
- [fcl.h](</C:/CFile/NAND Flash/fcl.h>)：FCL 介面與命令定義
- [fcl.c](</C:/CFile/NAND Flash/fcl.c>)：FCL 命令提交與轉發
- [ftl.h](</C:/CFile/NAND Flash/ftl.h>)：FTL 介面與統計結構
- [ftl.c](</C:/CFile/NAND Flash/ftl.c>)：FTL + GC 實作

---

## 目前模擬

## 1) NAND Layer

- `nand_program(block, page, buf)`  
  若目標 page 非 erased（非 `0xFF`）會回 `NAND_ERR_PROGRAM_FAIL`。
- `nand_read(block, page, buf)`  
  從指定實體 page 讀出資料。
- `nand_erase(block)`  
  擦除整個 block（所有 page 變成 `0xFF`）。
- `nand_mark_bad_block(block)`  
  將 block 標記為壞塊；之後 read/program/erase 都會失敗。
- `nand_get_program_count(...)` / `nand_get_erase_count(...)`  
  取得每個 block 的累計次數。

## 2) FTL Layer

- `ftl_write(lba, buf)`  
  將 LBA 寫到新 PPA（out-of-place write），並更新 mapping（透過 FCL 下發 command）。
- `ftl_read(lba, buf)`  
  依 mapping_table 讀取最新資料（透過 FCL 下發 command）。
- `ftl_get_mapping(lba, &block, &page)`  
  查詢 LBA 對應的實體位置。

## 3) FCL Layer

- `fcl_submit(cmd, block, page, buffer)`  
  以命令方式接收 NAND 操作請求（READ / PROGRAM / ERASE），再轉發到底層 NAND API。
- `fcl_read(...) / fcl_program(...) / fcl_erase(...)`  
  FTL 使用的便捷介面，形成 `FTL -> FCL -> NAND` 路徑。

## 4) Garbage Collection

- `ftl_garbage_collect()`  
  選擇含 invalid pages 的 victim block，搬移其中 valid pages 到新位置，最後 erase 舊 block。
- `ftl_get_stats(&stats)`  
  取得 GC 相關統計：
  - `gc_count`
  - `page_migration_count`
  - `valid_pages_copied`
  - `erase_count`
  - `host_write_count`
  - `nand_program_count`
- `ftl_get_write_amplification()`  
  回傳 `nand_program_count / host_write_count`。

---

## 編譯與執行（Windows + GCC）

在專案目錄執行：

```powershell
gcc "C:\CFile\NAND Flash\main.c" "C:\CFile\NAND Flash\nand_flash.c" "C:\CFile\NAND Flash\fcl.c" "C:\CFile\NAND Flash\ftl.c" -o "C:\CFile\NAND Flash\nand_flash.exe"
```

```powershell
& "C:\CFile\NAND Flash\nand_flash.exe"
```

執行成功時會看到 NAND/FTL/GC 檢查與統計輸出，最後顯示：

```text
NAND flash + FTL + GC simulation OK
```
