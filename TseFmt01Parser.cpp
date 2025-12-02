#include "TseFmt01Parser.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>

// Constants for parsing
static const int FMT01_LENGTH = 114;
static const uint8_t ESC_BYTE = 0x1B;
static const uint8_t CR_BYTE = 0x0D;
static const uint8_t LF_BYTE = 0x0A;

// Helper: Convert bytes to hex string
static std::string toHex(const uint8_t* p, int n) {
    static const char* hex = "0123456789ABCDEF";
    std::string s; s.resize(n * 2);
    for (int i = 0; i < n; ++i) {
        s[2*i]   = hex[(p[i] >> 4) & 0xF];
        s[2*i+1] = hex[p[i] & 0xF];
    }
    return s;
}

// Parse one message. Checks ESC, Terminal, and Length.
bool TseFmt01Parser::parseOneMSG01(const uint8_t* msg, int len, void* out) const {
    if (!out) return false;
    Tse01Record& row = *static_cast<Tse01Record*>(out);
    
    if (len < FMT01_LENGTH) return false;
    if (msg[0] != ESC_BYTE) return false;
    if (!(msg[len-2] == CR_BYTE && msg[len-1] == LF_BYTE)) return false;

    // ===== Header =====
    row.esc = msg[0];
    
    // Byte 2-3: Message Length (PACK-BCD 2 bytes) -> "0114"
    {
        std::string lenStr = bcdToDigitString(&msg[1], 2);
        row.msgLen = (lenStr.empty() ? 0 : std::stoi(lenStr));
    }
    
    // Byte 4: Business Type (PACK-BCD 1 byte)
    row.bizType = bcdToDigitString(&msg[3], 1);
    
    // Byte 5: Format Code (PACK-BCD 1 byte)
    row.fmtCode = bcdToDigitString(&msg[4], 1);
    
    // Byte 6: Format Version (PACK-BCD 1 byte)
    row.fmtVer  = bcdToDigitString(&msg[5], 1);
    
    // Byte 7-10: Sequence Number (PACK-BCD 4 bytes)
    row.seq     = bcdToDigitString(&msg[6], 4);

    // ===== Body 3.1: Stock Info =====
    // Stock ID: Byte 11-16 (ASCII 6 bytes)
    row.stockId   = asciiField(&msg[10], 6);
    
    // Stock Name: Byte 17-32 (ASCII 16 bytes, likely Big5)
    {
        std::string rawName(reinterpret_cast<const char*>(&msg[16]), 16);
        row.stockName = big5ToUtf8(rawName.c_str(), rawName.size());
    }
    
    // Industry, Security Type, Trade Note (ASCII)
    row.industry  = asciiField(&msg[32], 2);  // 33-34
    row.secType   = asciiField(&msg[34], 2);  // 35-36
    row.tradeNote = asciiField(&msg[36], 2);  // 37-38

    // Abnormal Code (PACK-BCD 1 byte)
    {
        std::string s = bcdToDigitString(&msg[38], 1); // 39
        row.abnCode = s.empty() ? 0 : std::stoi(s);
    }
    
    // Board (ASCII 1 byte)
    row.board = asciiField(&msg[39], 1); // 40

    // Prices: Ref, Up, Down (PACK-BCD 5 bytes) -> 9(6)V9(4)
    if (!parsePrice_fromBCD5(&msg[40], row.refPrice)) return false;
    if (!parsePrice_fromBCD5(&msg[45], row.upPrice )) return false;
    if (!parsePrice_fromBCD5(&msg[50], row.dnPrice )) return false;

    // Flags (ASCII 1 byte each)
    row.non10Par     = asciiField(&msg[55], 1); // 56
    row.abnPromo     = asciiField(&msg[56], 1); // 57
    row.specialAbn   = asciiField(&msg[57], 1); // 58
    row.dayTradeCash = asciiField(&msg[58], 1); // 59
    row.exemptSSR    = asciiField(&msg[59], 1); // 60
    row.exemptSBL    = asciiField(&msg[60], 1); // 61

    // Match Cycle Seconds (PACK-BCD 3 bytes) @ 62-64
    {
        std::string s = bcdToDigitString(&msg[61], 3);
        row.matchCycleSec = s.empty() ? 0 : std::stoi(s);
    }

    // ===== 3.2 Warrant Info (39 bytes) @ 65-103 =====
    row.warrantRawHex = toHex(&msg[64], 39);

    // ===== 3.3 Other Info (7 bytes) @ 104-110 =====
    row.otherRawHex = toHex(&msg[103], 7);

    // ===== 3.4 Line Note (PACK-BCD 1 byte) @ 111 =====
    row.lineNote = bcdToDigitString(&msg[110], 1);

    // Checksum (Byte 112)
    row.checksum = msg[111];

    // culculate XOR for checksum verification
    uint8_t x = 0 ;
    for (int i = 1; i <= 110; ++i) {
        x ^= msg[i];
    }
    row.calculateXor = x ;
    row.checksumOK = (row.calculateXor == row.checksum) ;

    return true;
}

std::string TseFmt01Parser::recToCsv01(const void* rec) const {
    if (!rec) return "";
    const Tse01Record& r = *static_cast<const Tse01Record*>(rec);
    
    std::ostringstream oss;
    oss << r.stockId << ","
        << csvEscape(r.stockName) << ","
        << std::fixed << std::setprecision(4)
        << r.refPrice << ","
        << r.upPrice  << ","
        << r.dnPrice;
    return oss.str();
}
