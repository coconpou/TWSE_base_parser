#include "StreamFramer.h"
#include "Utils.h"
#include <iostream>

// Constants for protocol
static const uint8_t ESC_BYTE = 0x1B;
static const uint8_t CR_BYTE = 0x0D;
static const uint8_t LF_BYTE = 0x0A;
static const int FORMAT_01_LEN = 114;
static const int MAX_MSG_LEN = 4096;

void StreamFramer::feed(const uint8_t* data, size_t n, Callback cb) {
    // Safety check: prevent buffer from growing indefinitely
    if (_buf.size() > MAX_BUFFER) {
        _buf.clear();
    }
    
    _buf.insert(_buf.end(), data, data + n);

    while (true) {
        // 1. Find the start of the message (ESC)
        int escPos = findNextESC(_buf, 0);
        if (escPos < 0) {
            // if ESC not found, clear buffer to save space 
            // cause can't have a valid message start
            _buf.clear();
            return;
        }

        // Discard garbage before ESC
        if (escPos > 0) {
            _buf.erase(_buf.begin(), _buf.begin() + escPos);
        }

        // 2. Peek header to get message length and format
        int msgLen = 0;
        std::string fmt, ver;
        
        // need at least enough bytes to parse the header
        if (!peekHeaderLenFmtVer(_buf, 0, msgLen, fmt, ver)) {
            // header incomplete.
            // if have a lot of data but still can't parse header, it maybe garbage.
            // for now, just wait for more data.

            // if have > 10 bytes (header size) and still fail, 
            // it maybe invalid data starting with ESC. 
            
            // if size >= 10 and still failed, drop one byte and retry.
            if ((int)_buf.size() >= 10) {
                _buf.erase(_buf.begin());
                continue;
            }
            return; // Wait for more data
        }

        // Format "01" which is fixed length 114 bytes
        if (fmt == "01" && msgLen != FORMAT_01_LEN) {
            msgLen = FORMAT_01_LEN;
        }

        // check on message length
        if (msgLen <= 0 || msgLen > MAX_MSG_LEN) {
            // Invalid length, discard and try next
            _buf.erase(_buf.begin());
            continue;
        }

        // Check if have the full message
        if ((int)_buf.size() < msgLen) {
            return; // Wait for more data
        }

        // 3. Extract the message
        std::vector<uint8_t> one(_buf.begin(), _buf.begin() + msgLen);

        // 4. Verify terminator (CRLF == 0A0D)
        if (!(one.size() >= 2 && 
              one[one.size() - 2] == CR_BYTE && 
              one[one.size() - 1] == LF_BYTE)) {
            // invalid terminator, this not a valid message.
            // discard the ESC and continue searching.
            _buf.erase(_buf.begin()); 
            continue;
        }

        // 5. Remove the processed message from buffer
        _buf.erase(_buf.begin(), _buf.begin() + msgLen);

        // 6. Dispatch
        cb(one, fmt);
    }
}
