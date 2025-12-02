#include "TseBaseParser.h"
#include "TseFmt01Parser.h"
#include "TseFmt06Parser.h"

std::unique_ptr<TseBaseParser> TseBaseParser::create(const std::string& fmt) {
    if (fmt == "01") return std::unique_ptr<TseBaseParser>(new TseFmt01Parser());
    if (fmt == "06") return std::unique_ptr<TseBaseParser>(new TseFmt06Parser());
    return nullptr;
}
