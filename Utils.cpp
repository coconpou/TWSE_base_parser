#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <windows.h> // For MultiByteToWideChar

#ifndef CP_BIG5
#define CP_BIG5 950
#endif

using namespace std;

// Remove spaces from string
string removeSpaceBack(const string& s) {
    string ret = s;
    ret.erase(remove(ret.begin(), ret.end(), ' '), ret.end());
    return ret;
}

// Convert PACK-BCD to digit string
string bcdToDigitString(const uint8_t* bcd, int nBytes) {
    string s;
    s.reserve(nBytes * 2);
    for (int i = 0; i < nBytes; ++i) {
        uint8_t val = bcd[i];
        // High nibble
        s.push_back('0' + ((val >> 4) & 0xF));
        // Low nibble
        s.push_back('0' + (val & 0xF));
    }
    return s;
}

// Parse Price from 5-byte BCD (9(6)V9(4))
// Total 10 digits: 6 integer digits, 4 decimal digits.
bool parsePrice_fromBCD5(const uint8_t* bcd5, double& out) {
    string s = bcdToDigitString(bcd5, 5);
    if (s.size() != 10) return false;
    
    // First 6 digits are integer part
    string sInt = s.substr(0, 6);
    // Last 4 digits are decimal part
    string sDec = s.substr(6, 4);
    
    try {
        double vInt = stod(sInt);
        double vDec = stod(sDec);
        out = vInt + (vDec / 10000.0);
        return true;
    } catch(...) {
        cout << "[Error] parsing price from BCD5: " << s << endl;
        return false;
    }
}

// Parse Qty from 4-byte BCD (8 digits)
bool parseQty_fromBCD4(const uint8_t* bcd4, uint32_t& out) {
    std::string s = bcdToDigitString(bcd4, 4); // 8 digits
    if (s.size() != 8) return false;
    try {
        out = static_cast<uint32_t>(std::stoul(s)); 
        return true;
    } catch (...) {
        cout << "[Error] parsing qty from BCD4: " << s << endl;
        return false;
    }
}

// Parse MatchTime from 6-byte BCD -> "HH:MM:SS.mmmuuu"
bool parseMatchTime_fromBCD6(const uint8_t* bcd6, std::string& out) {
    // hhmmssmmmuuu ¡÷ 12 digits
    string s = bcdToDigitString(bcd6, 6);
    if (s.size() != 12) return false;

    out.clear();
    out.reserve(15);
    out.append(s.substr(0,2)).append(":")   // HH
       .append(s.substr(2,2)).append(":")   // MM
       .append(s.substr(4,2)).append(".")   // SS.
       .append(s.substr(6,3))               // mmm
       .append(s.substr(9,3));              // uuu
    return true;
}

// Escape string for CSV
string csvEscape(const string& s) {
    // If contains comma, quote, or newline, wrap in quotes and escape quotes
    if (s.find_first_of(",\"\r\n") == string::npos) {
        return s;
    }
    string ret = "\"";
    for (char c : s) {
        if (c == '\"') ret += "\"\"";
        else ret += c;
    }
    ret += "\"";
    return ret;
}

// Find next ESC byte (0x1B)
int findNextESC(const vector<uint8_t>& buf, int start) {
    for (size_t i = start; i < buf.size(); ++i) {
        if (buf[i] == 0x1B) return (int)i;
    }
    return -1;
}

// Peek Header: Length, Format, Version
// Header structure:
// Byte 0: ESC
// Byte 1-2: Length (BCD)
// Byte 3: Business Type
// Byte 4: Format Code
// Byte 5: Format Version
// Byte 6-9: Seq
bool peekHeaderLenFmtVer(const vector<uint8_t>& buf, int escPos,
                         int& outLen, string& outFmt, string& outVer) 
{
    // Need at least 10 bytes to read header info
    if (escPos + 10 > (int)buf.size()) return false;

    const uint8_t* p = buf.data() + escPos;
    
    // Length (Byte 1-2)
    string sLen = bcdToDigitString(p + 1, 2);
    try {
        outLen = stoi(sLen);
    } catch(...) {
        outLen = 0;
    }

    // Format Code (Byte 4)
    outFmt = bcdToDigitString(p + 4, 1);
    
    // Format Version (Byte 5)
    outVer = bcdToDigitString(p + 5, 1);

    return true;
}

// Extract ASCII field
string asciiField(const uint8_t* p, int n) {
    string s(reinterpret_cast<const char*>(p), n);
    // Trim trailing spaces or nulls if needed, but usually just return as is or trim right
    // Here we just trim nulls if any, though spec says ASCII
    size_t nullPos = s.find('\0');
    if (nullPos != string::npos) s.resize(nullPos);
    return removeSpaceBack(s);
}

// Big5 to UTF-8 conversion (Windows only)
string big5ToUtf8(const char* bytes, size_t len) {
    if (len == 0) return "";
    
    // 1. Big5 -> WideChar (UTF-16)
    int wLen = MultiByteToWideChar(CP_BIG5, 0, bytes, (int)len, NULL, 0);
    if (wLen <= 0) return string(bytes, len); // Fallback to original if fail

    wstring wstr(wLen, 0);
    MultiByteToWideChar(CP_BIG5, 0, bytes, (int)len, &wstr[0], wLen);

    // 2. WideChar -> UTF-8
    int uLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wLen, NULL, 0, NULL, NULL);
    if (uLen <= 0) return string(bytes, len);

    string ret(uLen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wLen, &ret[0], uLen, NULL, NULL);
    
    return ret;
}
