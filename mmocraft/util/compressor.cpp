#include "pch.h"
#include "util/compressor.h"

namespace util
{
    std::unique_ptr<std::byte[]> gzip_compress(std::string_view source, unsigned& compressed_size)
    {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.next_in = reinterpret_cast<z_const Bytef*>(const_cast<char*>(source.data()));
        strm.avail_in = ::uInt(source.size());

        if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 | MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            return nullptr;

        // allocate the output buffer.
        compressed_size = deflateBound(&strm, strm.avail_in);
        auto dest = std::unique_ptr<std::byte[]>(new std::byte[compressed_size]);

        strm.next_out = reinterpret_cast<z_const Bytef*>(dest.get());
        strm.avail_out = compressed_size;
        if (deflate(&strm, Z_FINISH) != Z_STREAM_END || deflateEnd(&strm) != Z_OK)
            return nullptr;

        compressed_size = compressed_size - strm.avail_out;
        return dest;
    }

    Compressor::Compressor(char* block_data, unsigned block_data_size)
    {
        zstm.zalloc = Z_NULL;
        zstm.zfree = Z_NULL;
        zstm.opaque = Z_NULL;
        zstm.next_in = reinterpret_cast<z_const Bytef*>(block_data);
        zstm.avail_in = ::uInt(block_data_size);

        if (deflateInit2(&zstm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 | MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            return;

        max_compressed_size = deflateBound(&zstm, zstm.avail_in);
    }

    unsigned Compressor::deflate_n(std::byte* dest, unsigned avail_size)
    {
        zstm.next_out = reinterpret_cast<z_const Bytef*>(dest);
        zstm.avail_out = avail_size;
        deflate(&zstm, Z_FINISH);
        return zstm.avail_out;
    }
}