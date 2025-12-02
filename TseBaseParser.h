#ifndef TSE_BASE_PARSER_H
#define TSE_BASE_PARSER_H

#include <string>
#include <cstdint>
#include <memory>

class TseBaseParser {
public:
    virtual ~TseBaseParser() = default;

    // Pure virtual methods to be implemented by subclasses
    virtual bool parseOneMSG01(const uint8_t* msg, int len, void* out) const {
        return false;
    }

    virtual bool parseOneMSG06(const uint8_t* msg, int len, void* out) const {
        return false;
    }

    virtual std::string csvHeader() const {
        return "Stock Code,Stock Name,Today Ref Price,Up Limit Price,Down Limit Price";
    }

    virtual std::string recToCsv01(const void* rec) const {
        return "";
    }

    virtual std::string recToCsv06(const void* rec) const {
        return "";
    }

    // Factory by fmtCode: "01"/"06"
    static std::unique_ptr<TseBaseParser> create(const std::string& fmt);
};

#endif
