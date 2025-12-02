#include "TseFmt06Parser.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::string;

namespace {
    constexpr uint8_t ESC_BYTE = 0x1B;
    constexpr uint8_t CR_BYTE  = 0x0D;
    constexpr uint8_t LF_BYTE  = 0x0A;
    inline bool inRange(int pos, int end, int need) { return pos + need <= end; }
}

// 映射 06 → CSV 格式（選擇把 lastPx 映射作為示範）
static std::string map06ToCSV(const Tse06Record& src) {
    std::ostringstream oss;
    oss << src.stockId << ","
        << "" << ","  // 06 沒有股票名稱
        << std::fixed << std::setprecision(4)
        << src.lastPx << ","
        << 0.0 << ","  // upPrice 不適用
        << 0.0;        // dnPrice 不適用
    return oss.str();
}

bool TseFmt06Parser::parseOneMSG06(const uint8_t* msg, int len, void* out) const {
    if (!msg || !out || len < 32) return false;
    if (msg[0] != ESC_BYTE) return false;
    if (!(msg[len-2] == CR_BYTE && msg[len-1] == LF_BYTE)) return false;

    Tse06Record& r = *static_cast<Tse06Record*>(out);

    // 1) Header：宣告長度需等於實際長度
    string sLen = bcdToDigitString(&msg[1], 2); // Byte2-3
    int declared = 0;
    try { declared = std::stoi(sLen); } catch (...) { return false; }
    if (declared != len) return false;

    r.esc     = msg[0];
    r.msgLen  = declared;
    r.bizType = bcdToDigitString(&msg[3], 1); // "01"
    r.fmtCode = bcdToDigitString(&msg[4], 1); // "06"
    r.fmtVer  = bcdToDigitString(&msg[5], 1); // "04"
    r.seq     = bcdToDigitString(&msg[6], 4); // 4B

    // 2) Body 固定區
    r.stockId = asciiField(&msg[10], 6);                // [11-16]
    if (!parseMatchTime_fromBCD6(&msg[16], r.matchTime))// [17-22]
        return false;
    r.itemBitmap  = msg[22];                            // [23]
    r.limitBitmap = msg[23];                            // [24]
    r.stateBitmap = msg[24];                            // [25]
    if (!parseQty_fromBCD4(&msg[25], r.cumQty))         // [26-29]
        return false;

    // 3) 檢查碼（XOR Byte2..最後一個 BODY byte，與 msg[len-3] 比對）
    r.checksum = msg[len - 3];
    {
        uint8_t x = 0;
        for (int i = 1; i <= len - 4; ++i) x ^= msg[i];
        r.calcXor  = x;
        r.checksumOK = (x == r.checksum);
    }

    // 4) 變動區：成交 → 買 N → 賣 M
    const int payloadEnd = len - 3; // checksum 位置
    int pos = 29;                   // 已讀到 msg[28]

    r.lastPx = 0.0; r.lastQty = 0;
    r.bidPx.fill(0.0);  r.bidQty.fill(0);
    r.askPx.fill(0.0);  r.askQty.fill(0);

    const bool hasTrade   = (r.itemBitmap & 0b1000'0000) != 0;       // Bit7
    const int  bidLvls    = (r.itemBitmap >> 4) & 0b0000'0111;       // 0..5
    const int  askLvls    = (r.itemBitmap >> 1) & 0b0000'0111;       // 0..5
    const bool onlyTrade  = (r.itemBitmap & 0b0000'0001) != 0;       // Bit0
    const int  trendBits  = (r.limitBitmap & 0b0000'0011);
    const bool isDeferred = (trendBits == 0b01 || trendBits == 0b10);// 暫緩撮合

    // (a) 成交價量
    if (hasTrade) {
        if (!inRange(pos, payloadEnd, 5)) { return true; }
        (void)parsePrice_fromBCD5(&msg[pos], r.lastPx); pos += 5;

        if (!inRange(pos, payloadEnd, 4)) { return true; }
        uint32_t q = 0;
        if (parseQty_fromBCD4(&msg[pos], q)) r.lastQty = (isDeferred ? 0u : q);
        pos += 4;
    }

    // (b) 僅成交 或 暫緩撮合 → 不解析五檔
    if (onlyTrade || isDeferred) { return true; }

    // (c) 買 N 檔
    {
        const int nb = std::min(std::max(bidLvls, 0), 5);
        for (int i = 0; i < nb; ++i) {
            if (!inRange(pos, payloadEnd, 5)) { return true; }
            (void)parsePrice_fromBCD5(&msg[pos], r.bidPx[i]); pos += 5;

            if (!inRange(pos, payloadEnd, 4)) { return true; }
            (void)parseQty_fromBCD4(&msg[pos], r.bidQty[i]);  pos += 4;
        }
    }

    // (d) 賣 M 檔
    {
        const int na = std::min(std::max(askLvls, 0), 5);
        for (int i = 0; i < na; ++i) {
            if (!inRange(pos, payloadEnd, 5)) { return true; }
            (void)parsePrice_fromBCD5(&msg[pos], r.askPx[i]); pos += 5;

            if (!inRange(pos, payloadEnd, 4)) { return true; }
            (void)parseQty_fromBCD4(&msg[pos], r.askQty[i]);  pos += 4;
        }
    }

    return true;
}

// 先維持與 01 相同的 5 欄 CSV。之後要 24 欄可在此擴充 header/輸出。
std::string TseFmt06Parser::recToCsv06(const void* rec) const {
    if (!rec) return "";
    const Tse06Record& r = *static_cast<const Tse06Record*>(rec);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << std::left;
    
    // 股票代號
    oss << std::setw(10) << r.stockId << " ";
    
    // 買進 1~5 檔：價格、數量
    for (int i = 0; i < 5; ++i) {
        oss << std::setw(10) << r.bidPx[i] << " " 
            << std::setw(12) << r.bidQty[i] << " ";
    }
    
    // 賣出 1~5 檔：價格、數量
    for (int i = 0; i < 5; ++i) {
        oss << std::setw(10) << r.askPx[i] << " " 
            << std::setw(12) << r.askQty[i] << " ";
    }
    
    // 成交價、數量、搓合時間
    oss << std::setw(10) << r.lastPx << " " 
        << std::setw(12) << r.lastQty << " " 
        << std::setw(16) << r.matchTime;
    
    return oss.str();
}
