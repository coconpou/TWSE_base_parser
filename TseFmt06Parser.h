#ifndef TSE_FMT06_PARSER_H
#define TSE_FMT06_PARSER_H

#include "TseBaseParser.h"
#include <string>
#include <cstdint>
#include <array>

// 06 專用的完整欄位結構
struct Tse06Record {
    // Header
    uint8_t esc{};
    int     msgLen{};
    std::string bizType, fmtCode, fmtVer, seq;

    // Body 固定區
    std::string stockId;     // ASCII(6)
    std::string matchTime;   // 6B BCD → "HH:MM:SS.mmmuuu"
    uint8_t  itemBitmap{};
    uint8_t  limitBitmap{};
    uint8_t  stateBitmap{};
    uint32_t cumQty{};

    // 變動區：成交與最佳五檔
    double      lastPx{};
    uint32_t    lastQty{};
    std::array<double, 5>   bidPx{};
    std::array<uint32_t, 5> bidQty{};
    std::array<double, 5>   askPx{};
    std::array<uint32_t, 5> askQty{};

    // 檢查碼
    uint8_t checksum{};
    uint8_t calcXor{};
    bool    checksumOK{true};
};

class TseFmt06Parser : public TseBaseParser {
public:
    bool parseOneMSG06(const uint8_t* msg, int len, void* out) const override;
    
    // CSV 表頭（對齊格式）
    std::string csvHeader() const override {
        return "Stock ID   "
               "Bid1 Price   Bid1 Qty     "
               "Bid2 Price   Bid2 Qty     "
               "Bid3 Price   Bid3 Qty     "
               "Bid4 Price   Bid4 Qty     "
               "Bid5 Price   Bid5 Qty     "
               "Ask1 Price   Ask1 Qty     "
               "Ask2 Price   Ask2 Qty     "
               "Ask3 Price   Ask3 Qty     "
               "Ask4 Price   Ask4 Qty     "
               "Ask5 Price   Ask5 Qty     "
               "Last Trade Price Last Trade Qty Last Match Time";
    }
    
    std::string recToCsv06(const void* rec) const override;
};

#endif
