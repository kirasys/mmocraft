#pragma once
#include <memory>
#include <utility>
#include <string_view>
#include <zlib.h>

namespace util
{
    std::unique_ptr<std::byte[]> gzip_compress(std::string_view source, unsigned& compressd_size);

    class Compressor
    {
    public:
        Compressor(char* data, unsigned data_size);

        std::size_t deflate_bound() const
        {
            return max_compressed_size;
        }

        unsigned deflate_n(std::byte* source, unsigned avail_size);

    private:
        z_stream zstm;
        std::size_t max_compressed_size = 0;
    };
}