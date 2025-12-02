#ifndef STREAM_FRAMER_H
#define STREAM_FRAMER_H

#include <vector>
#include <string>
#include <functional>
#include <cstdint>

class StreamFramer {
private:
    std::vector<uint8_t> _buf;
    const size_t MAX_BUFFER = 1024 * 1024 * 10; // 10MB limit to prevent memory exhaustion
public:
    // Callback function type: (message_data, format_version)
    using Callback = std::function<void(const std::vector<uint8_t>&, const std::string&)>;

    // Feed data into the framer. It will buffer data and call the callback
    // for each complete message found.
    void feed(const uint8_t* data, size_t n, Callback cb);

};

#endif // STREAM_FRAMER_H
