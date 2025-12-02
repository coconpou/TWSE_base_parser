#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <cstdint>
#include <vector>

using namespace std;

string removeSpaceBack(const string& s);

// 將 PACK-BCD 轉為十進位字串
string bcdToDigitString(const uint8_t* bcd, int nBytes);

// 將 PACK-BCD（5 byte, 10 digit）轉為price
bool parsePrice_fromBCD5(const uint8_t* bcd5, double& out);

// 將 PACK-BCD（4 byte, 8 digit）轉為數量（uint32）
bool parseQty_fromBCD4(const uint8_t* bcd4, uint32_t& out);

// 將 PACK-BCD（6 byte, 12 digit）撮合時間 → "HH:MM:SS.mmmuuu"
bool parseMatchTime_fromBCD6(const uint8_t* bcd6, std::string& out);

// CSV 欄位
string csvEscape(const string& s);

// 在 byte 串中尋找下一個 0x1B(ESC), 沒找到回傳 -1
int findNextESC(const vector<uint8_t>& buf, int start);

// 以 ESC + Header 解析出 訊息長度.格式代碼.版別 若成功回傳 true
bool peekHeaderLenFmtVer(const vector<uint8_t>& buf, int escPos,
                         int& outLen, string& outFmt, string& outVer);


// 安全取字串（不轉碼 只去尾端空白/0）
string asciiField(const uint8_t* p, int n);

// Big5 -> UTF-8（Windows API 非 Windows 則原樣回傳）
string big5ToUtf8(const char* bytes, size_t len);

#endif
