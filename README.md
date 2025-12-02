# TWSE Quote Parser (Format 1 & 6)

這個專案是用 C++ 實作的「臺灣證券交易所 行情傳輸格式」解析器，目前支援：

- **格式一：集中市場普通股個股基本資料**（固定長度 114 bytes）
- **格式六：集中市場普通股競價交易即時行情資訊**（變動長度 32~131 bytes）

程式會從 TWSE 二進位行情檔中讀取資料流，依照 ESC / 長度 / 結尾 0D 0A 切出完整訊息，  
再依 HEADER 內的格式代碼 (`fmtCode`) 決定使用哪一個 Parser（01 或 06）做解析，  
最後輸出成 CSV 檔案。

---

## 專案結構

```text
TSEParser/
├─ main.cpp              # 入口程式：讀檔、使用 StreamFramer 切包、呼叫 Parser、輸出 CSV
├─ StreamFramer.cpp      # 負責從 byte stream 中找 ESC / 長度 / CRLF，切出完整訊息
├─ Utils.cpp             # BCD 轉字串、價格/數量解析、ASCII 欄位處理等共用工具
├─ TseBaseParser.cpp     # 定義通用 TseRecord 與 ParserFactory（create("01"/"06")）
├─ TseFmt01Parser.cpp    # 格式一解析：基本資料、今日參考價/漲停/跌停
├─ TseFmt06Parser.cpp    # 格式六解析：撮合時間、成交價量、買賣五檔等
├─ ...Other cpp
├─ include/
│  ├─ StreamFramer.h
│  ├─ Utils.h
│  ├─ TseBaseParser.h       # 通用 TseRecord + TseBaseParser 介面
│  ├─ TseFmt01Parser.h
│  ├─ TseFmt06Parser.h
│  └─ ...
├─ data/
│  └─ Tse.bin
├─ .gitignore
└─ README.md
