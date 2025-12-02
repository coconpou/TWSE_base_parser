#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <iomanip>
#include <set>

#include "TseBaseParser.h"
#include "TseFmt01Parser.h"
#include "TseFmt06Parser.h"
#include "Utils.h"
#include "StreamFramer.h"

using namespace std;


// ====================================================================
// main: Reads Tse.bin, writes to out_fmt01.csv and out_fmt06.csv (UTF-8)
// ====================================================================
int main(int argc, char* argv[]) {
    const char* inPath  = (argc > 1 ? argv[1] : "Tse.bin");
    const char* outPath01 = "out_fmt01.csv";
    const char* outPath06 = "out_fmt06.csv";
    const size_t CHUNK  = 2048;     // Read 2 KB at a time
    const int MAX_OUT   = 100000;  // Max output rows

    StreamFramer framer;
    int outCount01 = 0, outCount06 = 0;
    set<string> unsupportVersions;  // Track not supported versions
    
    // Output files
    ofstream fout01(outPath01, ios::binary);
    ofstream fout06(outPath06, ios::binary);
    if (!fout01) { cerr << "[ERROR] " << outPath01 << " cannot create.\n"; return 1; }
    if (!fout06) { cerr << "[ERROR] " << outPath06 << " cannot create.\n"; return 1; }

    // Register parsers
    unordered_map<string, unique_ptr<TseBaseParser>> parsers;
    bool header01Wrote = false;
    bool header06Wrote = false;
    // Open input file
    ifstream fin(inPath, ios::binary);
    if (!fin) {
        cerr << "Cannot open input file: " << inPath << "\n";
        return 1;
    }    

    // Callback for processing each message
    auto onMessage = [&](const vector<uint8_t>& msg, const string& version) {
        if (outCount01 >= MAX_OUT && outCount06 >= MAX_OUT) return;

        // get parser
        auto it = parsers.find(version);
        if( version != "01" && version != "06") {
            if( unsupportVersions.find(version) == unsupportVersions.end() ) {
                //cout << "[Detected Unsupported format] version: " << version << "\n";
                unsupportVersions.insert(version);
            }
            return;
        }

        // first time see this format, create parser
        if (it == parsers.end()) {
            auto parser = TseBaseParser::create(version);
            if (!parser) {
                // format not supported
                cout << "[Error] version: " << version << " parser create fail." << "\n";
                return;
            }
            it = parsers.emplace(version, std::move(parser)).first;
        }

        // Parse into appropriate record type
        if( version == "01") {
            if (outCount01 >= MAX_OUT) return;
            
            Tse01Record rec01;
            if (it->second->parseOneMSG01(msg.data(), (int)msg.size(), &rec01)) {
                if( !header01Wrote ) {
                    fout01 << it->second->csvHeader() << "\n";
                    header01Wrote = true;
                } 
                fout01 << it->second->recToCsv01(&rec01) << "\n";
                outCount01++;
                
                // Progress output every 100000 records
                if( outCount01 % 100000 == 0 ) {
                    cout << "[FMT01] Processed " << outCount01 << " records\n";
                }
            }

            // check XOR and checksum is equal
            // if not, print error to screen
            if (!rec01.checksumOK ) {
                cout << "[ERROR] format: " << version
                     << " seq: " << rec01.seq
                     << " stockId: " << rec01.stockId
                     << " calculateXor: 0x" << std::uppercase << std::hex
                     << std::setw(2) << std::setfill('0') << (unsigned) rec01.calculateXor
                     << " field=0x" << std::setw(2) << std::setfill('0') << (unsigned) rec01.checksum
                     << std::dec << "\n";
                return;
            }
        }
        else if( version == "06") {
            if (outCount06 >= MAX_OUT) return;
            
            Tse06Record rec06;
            if (it->second->parseOneMSG06(msg.data(), (int)msg.size(), &rec06)) {
                if( !header06Wrote ) {
                    fout06 << it->second->csvHeader() << "\n";
                    header06Wrote = true;
                } 
                fout06 << it->second->recToCsv06(&rec06) << "\n";
                outCount06++;
                
                // Progress output every 100000 records
                if( outCount06 % 100000 == 0 ) {
                    cout << "[FMT06] Processed " << outCount06 << " records\n";
                }
            }

            // check XOR and checksum is equal
            // if not, print error to screen
            if (!rec06.checksumOK ) {
                cout << "[ERROR] format: " << version
                     << " seq: " << rec06.seq
                     << " stockId: " << rec06.stockId
                     << " calculateXor: 0x" << std::uppercase << std::hex
                     << std::setw(2) << std::setfill('0') << (unsigned) rec06.calcXor
                     << " field=0x" << std::setw(2) << std::setfill('0') << (unsigned) rec06.checksum
                     << std::dec << "\n";
                return;
            }
        }
    };

    vector<uint8_t> chunk(CHUNK);

    // Main loop: read chunk, feed to framer
    while (outCount01 < MAX_OUT || outCount06 < MAX_OUT) {
        fin.read((char*)chunk.data(), CHUNK);
        streamsize got = fin.gcount();

        if (got <= 0) break;

        framer.feed(chunk.data(), (size_t)got, onMessage);

        // If we read less than CHUNK, we reached EOF
        if (got < (streamsize)CHUNK)
            break;
    }

    if (!fin && fin.eof()) {
        cerr << "Warning: Incomplete record at EOF ignored.\n";
    }

    cout << "Done.\n";
    cout << "Output " << outCount01 << " rows to " << outPath01 << "\n";
    cout << "Output " << outCount06 << " rows to " << outPath06 << "\n";
    return 0;
}
