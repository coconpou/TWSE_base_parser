#ifndef TSE_FMT01_PARSER_H
#define TSE_FMT01_PARSER_H

#include "TseBaseParser.h"
#include <string>

using namespace std;

// 格式一完整欄位存儲（Header + Body）
struct Tse01Record {
    // Header
    uint8_t esc{};      // Byte 1
    int     msgLen{};   // Byte 2-3 (PACK-BCD → int)
    string  bizType;    // Byte 4  (PACK-BCD)
    string  fmtCode;    // Byte 5  (PACK-BCD)
    string  fmtVer;     // Byte 6  (PACK-BCD)
    string  seq;        // Byte 7-10 (PACK-BCD 4 bytes)

    // Body 3.1 (Byte 11–64)
    string  stockId;        // 11–16 ASCII(6)
    string  stockName;      // 17–32 ASCII(16) 可能 Big5
    string  industry;       // 33–34 ASCII(2)
    string  secType;        // 35–36 ASCII(2)
    string  tradeNote;      // 37–38 ASCII(2)
    int     abnCode{};      // 39 PACK-BCD(1)
    string  board;          // 40 ASCII(1)
    double  refPrice{};     // 41–45 PACK-BCD(5) → 9(6)V9(4)
    double  upPrice{};      // 46–50
    double  dnPrice{};      // 51–55
    string  non10Par;       // 56 ASCII(1)
    string  abnPromo;       // 57 ASCII(1)
    string  specialAbn;     // 58 ASCII(1)
    string  dayTradeCash;   // 59 ASCII(1)
    string  exemptSSR;      // 60 ASCII(1)
    string  exemptSBL;      // 61 ASCII(1)
    int     matchCycleSec{};// 62–64 PACK-BCD(3)

    // 3.2 權證資料 39B (65–103) → 原樣十六進位
    string  warrantRawHex;

    // 3.3 其它資料 7B (104–110) → 原樣十六進位
    string  otherRawHex;

    // 3.4 行情資訊線路註記 1B (111 PACK-BCD)
    string  lineNote;

    // 檢查碼 (112)
    uint8_t checksum{};

    // 檢查XOR值
    uint8_t calculateXor{};
    bool checksumOK{true};
};

class TseFmt01Parser : public TseBaseParser {
public:
    bool parseOneMSG01(const uint8_t* msg, int len, void* out) const override;

    // CSV 表頭（5 欄）
    std::string csvHeader() const override {
        return "Stock Code,Stock Name,Today Ref Price,Up Limit Price,Down Limit Price";
    }

    // CSV 單列（5 欄）
    std::string recToCsv01(const void* rec) const override;
};

#endif
