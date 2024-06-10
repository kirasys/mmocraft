#include "pch.h"
#include "util/compressor.h"

#include <zlib.h>

namespace util
{
    bool gzip_compress(std::string_view source, std::string& dest)
    {
        ::uInt compressed_size = 0;

        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.next_in = reinterpret_cast<z_const Bytef*>(const_cast<char*>(source.data()));
        strm.avail_in = ::uInt(source.size());

        if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 | MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
            return false;

        // allocate the output buffer.
        compressed_size = deflateBound(&strm, strm.avail_in);
        dest.resize(compressed_size + 1);

        strm.next_out = reinterpret_cast<z_const Bytef*>(dest.data());
        strm.avail_out = compressed_size;
        if (deflate(&strm, Z_FINISH) != Z_STREAM_END || deflateEnd(&strm) != Z_OK)
            return false;

        dest.resize(compressed_size - strm.avail_out);
        return true;
    }
}